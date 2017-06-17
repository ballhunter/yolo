#pragma once

#include <yolo/YComm/YCommCommon.h>
#include <set>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <list>

namespace yolo
{
namespace ycomm
{

class YUdpReceiveDatagramEvent;
class YUdpReceiveDatagramHandler;

// UDP�� TCP�� �޸� session�� ������ �����Ƿ� YUdp Ŭ������ ���ϰ���, ������ �ۼ��� ó���� ��� ����Ѵ�.
class YCOMMDLL_API YUdp

{

public:
	YUdp(std::string localAddress, unsigned short localport , std::string remoteAddress, unsigned short remoteport, CastType castType);
	YUdp(unsigned short port, CastType castType);	
	YUdp(std::string localAddress, unsigned short port, CastType castType);	

private:	
	YUdp(void){}
	~YUdp(void);
	//inline void					setAsioIoService(boost::asio::io_service* s) { _io_service = s; }
	bool						open();
	void						close();

	// 2013-02-18 �̿��� �߰�
	void						receiveSome();
	void						receiveSomeAsync();	
	void						handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

	void						receiveSomeThread();
	void						start();

public:
	void						receiveSomeAsync(unsigned int i);

	size_t						sendTo(unsigned char* data, size_t dataLength);
	size_t						recvFrom(unsigned char* data, size_t dataLength);
	size_t						sendTo(unsigned char* data, size_t dataLength, std::string remoteAddress, unsigned short remoteport);
	size_t						recvFrom(unsigned char* data, size_t dataLength, std::string remoteAddress, unsigned short remoteport);

	// 2013-02-18 �̿��� �߰�
	// receive event ��ü ������ udp socket open������ �̷������ �ϹǷ� YComm::registerUDP ������ ȣ���ؾ� �Ѵ�.
	inline void					setReceiveDatagramEvent(YUdpReceiveDatagramEvent* e) { _receiveEvent = e; }
	void						addReceiveDatagramEvent(YUdpReceiveDatagramEvent* e, std::string remoteAddress, unsigned short remoteport);


	inline void					setMulticastAddress(std::string addr) { _multicastAddress = boost::asio::ip::address::from_string(addr); }
	inline void					setMulticastPort(unsigned short port) { _multicastPort = port; }
private:
	friend class YComm;
	boost::asio::ip::udp::socket*	_udp_socket;

	std::string					_LAddress;
	unsigned short				_LPort;

	std::string					_RAddress;
	unsigned short				_RPort;

	CastType					_CastType;
	boost::asio::ip::udp::endpoint _receiverEndpoint;
	boost::asio::ip::udp::endpoint _senderEndpoint;

	bool						_valid;			

	YUdpReceiveDatagramEvent*	_receiveEvent;
	unsigned char*				_receivedDatagram;
	unsigned char*				_receivedDatagramForHandler;
	unsigned short				_receivedDatagramLength;

	std::vector<YUdpReceiveDatagramHandler*> _handlerList;


	boost::asio::ip::address	_multicastAddress;
	unsigned short				_multicastPort;
		
	boost::mutex				_mutex;

	boost::asio::io_service*	_io_service;
	boost::asio::strand*		_strand;
	boost::asio::io_service::work*	_work;
	boost::thread*			_receiveThread;

	bool				_receivedSome;
	bool				_continueReceive;
};

}	// namespace ycomm
}	// namespace yolo
