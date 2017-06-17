#include <yolo/YComm/YUdpReceiveDatagramHandler.h>
#include <yolo/YComm/YUdpReceiveDatagramEvent.h>
#include <yolo/YComm/YUdp.h>
#include <algorithm>

namespace yolo
{
namespace ycomm
{

YUdpReceiveDatagramHandler::YUdpReceiveDatagramHandler()
{	
	_eventListner = NULL;	
	_bufferSize = 65535;
	_buffer = new unsigned char[_bufferSize];
	_id = 0;
	_udpObj = NULL;
}

YUdpReceiveDatagramHandler::YUdpReceiveDatagramHandler(unsigned short bufferSize)
{
	_eventListner = NULL;
	_bufferSize = bufferSize;
	_buffer = new unsigned char[_bufferSize];
	_id = 0;
	_udpObj = NULL;
}

YUdpReceiveDatagramHandler::~YUdpReceiveDatagramHandler(void)
{
	delete [] _buffer;
}

void
YUdpReceiveDatagramHandler::handle_receive(const boost::system::error_code& error, size_t bytes_transferred)
{
	if(!error)
	{
		// YUdp�� readSome()���� _buffer�� _senderEndpoint�� ����Ͽ� async_receive�� ȣ���ϰ�
		// udp datagram�� �����ϸ� �� �Լ��� callback �ȴ�.
		// ���⼭�� ��ϵ� _eventListener�� onReceiveDatagram �Լ��� ȣ�����ָ� �ȴ�.

		if(_eventListner != NULL && _buffer != NULL)
		{
			_eventListner->onReceiveDatagram(_senderEndpoint.address().to_string()
				, _senderEndpoint.port(), _buffer, (unsigned short)bytes_transferred);

			// ���� handler ��ü�� handle_receive�� �������Ƿ� �ٽ� UDP ��ü��
			// receiveSomeAsync�� �ٽ� ȣ�����־�� �Ѵ�.
			// �� �� �ش� handler ��ü�� handle_receive�� �ٽ� ����ϸ� �ǹǷ�
			// id�� ���ڷ� �ѱ��.
			if(_udpObj != NULL)
				_udpObj->receiveSomeAsync(_id);
		}
	}
	else
	{
		// ���⼭�� ���� ó������ ���� ���µ�...
		std::cout << error.message() << std::endl;
	}
}


}	// namespace ycomm
}	// namespace yolo
