#include <yolo/YComm/YTcpClient.h>
#include <yolo/YComm/YTcpSession.h>
#include <yolo/YComm/YComm.h>
#include <boost/thread.hpp>

namespace yolo
{
namespace ycomm
{

YTcpClient::YTcpClient(std::string serverAddress, unsigned short serverPort) 
	: _serverAddress(serverAddress), _serverPort(serverPort)
{
	_c_event = NULL;
	_r_event = NULL;

	_session = NULL;

	_connected = false;

	_bufferSize = defaultBufferSize;

	_isTryToReconnect = false;
	_disconnecting = false;
}

YTcpClient::YTcpClient(std::string serverAddress, unsigned short serverPort, bool isTryToReconnect)
	: _serverAddress(serverAddress), _serverPort(serverPort)
{
	_c_event = NULL;
	_r_event = NULL;

	_session = NULL;

	_connected = false;

	_bufferSize = defaultBufferSize;

	_isTryToReconnect = isTryToReconnect;
	_disconnecting = false;

}

YTcpClient::~YTcpClient(void)
{
	if(_session != NULL) {
		//_session->disconnect();
		//delete _session;
	}
}

void
YTcpClient::onConnected(YTcpSession& session)
{
	// ������ �Ǿ����� ���� ���� �� ���� ����.
}

void
YTcpClient::onDisconnected(YTcpSession& session)
{
	if(_session == &session)
	{
		//_session = NULL;
		// ���� session ��ü�� session���� �˾Ƽ� ����Ƿ� ���⼭�� ó���� �ʿ䰡 ����.
		// TcpClient���� session�� �����ϱ� ������ �� Ŭ�������� ����� ���� ��Ģ������
		// onDisconnected�� callback�̸� �� callback�� �ٸ� Ŭ������ ���� �� �ֱ� ������
		// ��� callback�� ó���� ���Ŀ� session Ŭ�������� �ڽ��� ���쵵�� �ϴ� ���� �´�.


		if(_isTryToReconnect && !_disconnecting)
		{
			// ������ flag�� Ȱ��ȭ�Ǿ� ������ ������ �ٽ� �õ��Ѵ�.

			// TCP session�� �����Ͽ� ������ �õ��Ѵ�.
			boost::asio::io_service& io = YComm::getInstance().getIOService();
			_session = new YTcpSession(io);
			_session->setBufferSize(_bufferSize);
			// TCP session ��ü�� connection, recv data �̺�Ʈ �ݹ��� ����Ѵ�.			
			_session->registerConnectionEventCallback(_c_event);			
			_session->registerConnectionEventCallback(this);
			_session->registerConnectionEventCallback(&YComm::getInstance());
			
			_session->registerReceiveDataEventCallback(_r_event);

			auto bf = boost::bind(&YTcpClient::reconnectThread, this, _1);
			//_threadPool->runThread(bf, (void*)_session);
			boost::thread t(bf, (void*)_session);
		}

	}
}

bool
YTcpClient::connect(unsigned int timeout)
{
	bool success = false;

	//if(_io_service != NULL)
	{		
		// TCP session�� �����Ͽ� ������ �õ��Ѵ�.
		boost::asio::io_service& io = YComm::getInstance().getIOService();
		_session = new YTcpSession(io);
		_session->setBufferSize(_bufferSize);
		// TCP session ��ü�� connection, recv data �̺�Ʈ �ݹ��� ����Ѵ�.		
		_session->registerConnectionEventCallback(_c_event);
		_session->registerConnectionEventCallback(this);
		_session->registerConnectionEventCallback(&YComm::getInstance());
		
		_session->registerReceiveDataEventCallback(_r_event);
		
		if(!_isTryToReconnect)
		{
			success = _session->connect(_serverAddress, _serverPort, timeout);
			
			if(success)
				_session->start();		
		}
		else
		{
			// ������ thread ����
			auto bf = boost::bind(&YTcpClient::reconnectThread, this, _1);
			//_threadPool->runThread(bf, (void*)_session);
			boost::thread t(bf, (void*)_session);

			success = true;

			cout << "[YTcpClient] connection failed: trying to reconnect....(" << _serverAddress << ":" << _serverPort << ")" << endl;
		}		
	}

	return success;
}

bool
YTcpClient::disconnect()
{
	bool success = false;

	if(_session != NULL)
	{
		_disconnecting = true;
		success = _session->disconnect();
		//delete _session;
		//_session = NULL;
	}

	return success;
}

size_t
YTcpClient::send(unsigned char* data, size_t dataLength)
{
	if(_session != NULL && _session->getSocket().is_open())
	{
		return _session->send(data, dataLength);
	}
	else
		return 0;
}

size_t
YTcpClient::recv(unsigned char* data, size_t dataLength)
{
	if(_session != NULL && _session->getSocket().is_open())
	{
		return _session->recv(data, dataLength);
	}
	else
		return 0;
}


void
YTcpClient::reconnectThread(void* arg)
{
	YTcpSession* session = (YTcpSession*)arg;

	bool success = false;
	do
	{
		success = session->connect(_serverAddress, _serverPort, 1000);

		if(!success)
		{
			cout << "[YTcpClient] reconnect failed... trying to reconnect...(" << _serverAddress << ":" << _serverPort << ")" << endl;
		}
		else
		{
			session->start();
			break;
		}

		boost::this_thread::sleep(boost::posix_time::seconds(1));
	} while(!success);
}

}	// namespace ycomm
}	// namespace yolo

