#include <yolo/YComm/YTcpSession.h>
#include <yolo/YComm/YTcpConnectionEvent.h>
#include <yolo/YComm/YTcpReceiveDataEvent.h>
#include <boost/thread.hpp>


namespace yolo
{
namespace ycomm
{

YTcpSession::YTcpSession(boost::asio::io_service& io_service) : _io_service(io_service), _strand(io_service), _work(io_service)
{
	_tcp_socket = boost::shared_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(io_service));
	_connected = false;
	_number = -1;	
	_receiveDataEvent = NULL;
	_asyncConnectResult = false;
	_bufferSize = defaultBufferSize;
//	_bufferPool = new boost::pool<>(_bufferSize);	
	_valid = true;	
        _receivedSome = false;
        _continueRead = true;
}

YTcpSession::~YTcpSession(void)
{	
//	delete _bufferPool;
    _continueRead = false;
    _readThread->join();
    delete _readThread;

    std::deque<BufferInfo>::iterator itr;
    for(itr = _dataQueue.begin(); itr != _dataQueue.end(); itr++)
    {
            unsigned char* data = itr->data;
            if(data != NULL)
                    delete[] data;
    }
}


bool
YTcpSession::connect(std::string serverAddress, unsigned short serverPort, unsigned int timeout)
{
	char buffer[10];
	sprintf(buffer, "%d", serverPort);
	bool connectSuccess = false;

	try
	{
		boost::asio::ip::tcp::resolver resolver(_io_service);
		boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), serverAddress, buffer);
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		
		if(timeout == 0)
		{
			boost::asio::connect(*_tcp_socket, iterator);
			
			connectSuccess = true;						
		}
		else
		{
			// ���� timeout�� �����Ͽ����� async_connect�� �õ��ϰ� timeout �ð���ŭ timer�� �����Ѵ�.
			// �� �ð� �Ŀ� ������ ���� �ʾҴٸ� false�� �����Ѵ�.
			// timer�� ���� ���߿� ������ �Ǹ� timer�� ����ϰ� true�� �����ϵ��� �Ѵ�.
			boost::asio::async_connect(*_tcp_socket, iterator,
				boost::bind(&YTcpSession::handle_connect, this, boost::asio::placeholders::error)
				);
			
			//boost::asio::io_service::work work(_io_service);
			boost::thread t2(boost::bind(&boost::asio::io_service::run_one, &_io_service));

			// block thread�� ���۽�Ų��.
			_asyncWaitTimeout = true;
			boost::thread t(boost::bind(&YTcpSession::block_thread, this, timeout));
			t.join();

			// ���� ����� �Ǵ��Ѵ�.
			connectSuccess = _asyncConnectResult;

			// timeout�� �Ǿ��µ� ���� ������ �ȵǾ��ٸ� ���ӽõ��� ����
			// ������ ���� ������ �� ���� �����Ƿ� socket�� �ݴ´�.
			if(!connectSuccess) {
				_tcp_socket->cancel();
				_tcp_socket->close();
			}
		}

		if(connectSuccess)
		{
			_tcp_socket->set_option(boost::asio::socket_base::reuse_address(true));

			_connected = true;
			// ��ϵ� connection callback�� ȣ���Ѵ�.
			if(_connectionEvent.size() != 0)
			{
				deque<YTcpConnectionEvent*>::iterator itr;
				for(itr = _connectionEvent.begin(); itr != _connectionEvent.end(); itr++) {
					YTcpConnectionEvent* e = *itr;
					if(e != NULL)
						e->onConnected(*this);
				}
			}
		}
		
	}
	catch(std::exception& e)
	{
		_connected = false;
		std::cerr << "[YComm] Exception: " << e.what() << std::endl;		
	}

	return connectSuccess;
}

void
YTcpSession::handle_connect(const boost::system::error_code& ec)
{
	if(!ec)
	{
		// ���� ����
		_asyncConnectResult = true;		
	}
	else
	{
		// ���� ����
		_asyncConnectResult = false;		
	}

	// ���� ���ΰ� �ǰ��� ������ block �����带 �����.
	_asyncWaitTimeout = false;
}

void
YTcpSession::block_thread(unsigned int timeout)
{
	// 50ms �������� check�Ѵ�.
	const int TIME_STEP = 10;	// 10ms

	while(_asyncWaitTimeout && timeout > 0) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(TIME_STEP));
		timeout -= TIME_STEP;
	}
}

bool
YTcpSession::disconnect()
{
	boost::system::error_code error;
	_tcp_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	_tcp_socket->close(error);
	

	if(!error)
		return true;
	else
		return false;
}

std::string
YTcpSession::getPeerAddress()
{
	std::string addr;

	if(_tcp_socket->is_open())
	{
		addr = _tcp_socket->remote_endpoint().address().to_string();
	}
	else
	{
		addr = "";
	}

	return addr;
}

unsigned short
YTcpSession::getPeerPort()
{	
	unsigned short port = 0;

	if(_tcp_socket->is_open())
	{
		port = _tcp_socket->remote_endpoint().port();
	}

	return port;
}

std::string
YTcpSession::getLocalAddress()
{
	std::string addr;

	if(_tcp_socket->is_open())
	{
		addr = _tcp_socket->local_endpoint().address().to_string();		
	}
	else
	{
		addr = "";
	}

	return addr;
}

unsigned short
YTcpSession::getLocalPort()
{
	unsigned short port = 0;

	if(_tcp_socket->is_open())
	{
		port = _tcp_socket->local_endpoint().port();
	}

	return port;
}

void
YTcpSession::registerConnectionEventCallback(YTcpConnectionEvent* e)
{
	_connectionEvent.push_back(e);
}

void
YTcpSession::registerReceiveDataEventCallback(YTcpReceiveDataEvent* e)
{
	_receiveDataEvent = e;
}

size_t
YTcpSession::send(unsigned char* data, size_t dataLength)
{
	size_t sendLength = 0;

	try {
		//boost::mutex::scoped_lock lock(_mutex);

		if(_valid)
			sendLength = boost::asio::write(*_tcp_socket, boost::asio::buffer(data, dataLength));
	}
	catch(boost::system::system_error& e) {		
		std::cerr << "[YComm] TCP send failed" << std::endl;
		std::cerr << "[YComm] Exception: " << e.what() << std::endl;
	}

	return sendLength;
}

size_t
YTcpSession::recv(unsigned char* data, size_t dataLength)
{	
	size_t recvLength = 0;	

	// data buffer queue���� ù ��° �׸��� �����´�.
	std::deque<BufferInfo>::iterator itrFirst = _dataQueue.begin();
	if(itrFirst != _dataQueue.end())
	{		
		if(itrFirst->bytesTransferred == 0 || data == NULL)
		{
			// ���Ź��� �����Ͱ� ���µ� queue�� �� ���̴�.
			// �̷� ��찡 ����� �̴� queue�� �ӽ÷� �־���� element�� �����Ͱ� ä������ ����
			// �б⸦ �õ��� ���̴�. �� ���� �״�� return			
		}
		else if(data != NULL && itrFirst->bytesTransferred > dataLength)
		{
			memcpy(data, itrFirst->data + itrFirst->dataOffset, dataLength);
			itrFirst->dataOffset += dataLength;
			itrFirst->bytesTransferred -= dataLength;
			recvLength = dataLength;

			// ���ŵ� �����Ͱ� ���� ���� �ִ� ��Ȳ�̹Ƿ� queue�� �ǵ帮�� �ʴ´�.
		}
		else if(data != NULL && itrFirst->bytesTransferred <= dataLength)
		{
			memcpy(data, itrFirst->data + itrFirst->dataOffset, itrFirst->bytesTransferred);
			recvLength = itrFirst->bytesTransferred;
			itrFirst->dataOffset = 0;
			itrFirst->bytesTransferred = 0;

			// queue���� ���� ù ��° ���۸� �� �о����Ƿ� queue���� pop�ϰ� data�� �����Ѵ�.
			// queue�� ���� element�� ���� handle_read���� ó���ϰų�
			// ����� �ڵ忡�� ó���� �̷�����Ƿ� ���⼭ �Ϻη� ó���� �ʿ�� ����.
			delete[] itrFirst->data;			
			//_bufferPool->ordered_free(itrFirst->data);
			_dataQueue.pop_front();
		}
	}	
	
	return recvLength;
}


void
YTcpSession::start()
{	
	_tcp_socket->set_option(boost::asio::socket_base::reuse_address(true));
        _receivedSome = true;
	//readSome();	
        _readThread =  new boost::thread(boost::bind(&YTcpSession::readSomeThread, this));
}

void
YTcpSession::readSome()
{
	// �����ΰ� �����ϱ� ���� ���� ���� element�� queue�� �־�д�.
	// �� element ���۸� ����Ͽ� �����͸� �����Ѵ�.
	BufferInfo info;
	
	// 2011-11-09 �̿��� ����
	// ���۸� new�� ���� ������� �ʰ� boost::pool�� ����ϵ��� �����Ѵ�.->����
	info.data = new unsigned char[_bufferSize];
	//ZeroMemory(info.data, _bufferSize);
	memset(info.data, 0, _bufferSize);

	_dataQueue.push_back(info);
//	std::cout << "queue count: " << _dataQueue.size() << std::endl;
	_tcp_socket->async_read_some(boost::asio::buffer(info.data, _bufferSize),
		boost::bind(&YTcpSession::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred, info.data) );

//	boost::asio::io_service::work work(_io_service);
//	boost::thread t2(boost::bind(&boost::asio::io_service::run, &_io_service));			
}

void
YTcpSession::handle_read(const boost::system::error_code& error, size_t bytes_transferred, unsigned char* data)
{
    if(!_valid)
    {		
        delete this;
    }
    else
    {
        _receivedSome = true;
        if(!error)
        {
            // �����ΰ� �����͸� �����ϸ� ���� ���� �غ� ���Ͽ� readSome()�� ȣ���ϰ�
            // ��ϵ� callback�� ȣ���Ѵ�.
            //		printf("Some message received by async socket!\n");

            if(bytes_transferred > 0)
            {
                /*
                std::deque<BufferInfo>::iterator itrFirst = _dataQueue.begin();
                if(itrFirst != _dataQueue.end())
                {
                        // bytes_transferred ���� �����Ѵ�.
                        itrFirst->bytesTransferred = bytes_transferred;
                }*/
                                                
                for(auto itr = _dataQueue.begin(); itr != _dataQueue.end(); ++itr)
                {
                    /*
                    if(itr->bytesTransferred == 0)
                    {
                        itr->bytesTransferred = bytes_transferred;
                        break;
                    }
                    */
                    if(itr->data == data)
                    {
                        itr->bytesTransferred = bytes_transferred;
                        break;
                    }
                }

                //readSome();

                // ���� ������ ó��
                // ������ ���� �̺�Ʈ�� callback�Ѵ�.
                if(_receiveDataEvent != NULL)
                {
                    _receiveDataEvent->onReceiveData(*this);
                }		
            }
            else
            {
                //readSome();
            }
            
        }
        else
        {
            boost::mutex::scoped_lock lock(_mutex);
            
            _valid = false;

            // ������ ������
            // ��ϵ� connection callback�� ȣ���Ѵ�.
            if(_connectionEvent.size() != 0)
            {
                deque<YTcpConnectionEvent*>::iterator itr;
                for(itr = _connectionEvent.begin(); itr != _connectionEvent.end(); itr++) {
                    YTcpConnectionEvent* e = *itr;
                    if(e != NULL)
                        e->onDisconnected(*this);
                }
                //delete this;
                //readSome();
                _receivedSome = true;
            }
        }
    }	
}

void
YTcpSession::handle_send(const boost::system::error_code& error, size_t bytes_transferred)
{
	if(!_valid)
	{		
	}
	else
	{
		if(!error)
		{	
			
		}
		else
		{
			cerr << "[YTcpSession::handle_send] send failed!!!! : " << bytes_transferred << endl;
		}
	}	
}

int
YTcpSession::getSocketHandle()
{
	boost::asio::ip::tcp::socket::native_handle_type handle = _tcp_socket->native_handle();
	
	return (int)handle;
}

void
YTcpSession::readSomeThread()
{
    boost::thread t(boost::bind(&boost::asio::io_service::run, &_io_service));

    while(_continueRead)
    {
        if(_receivedSome) {
            _receivedSome = false;
            readSome();
        } else {
            boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        }
    }
}

}	// namespace ycomm
}	// namespace yolo

