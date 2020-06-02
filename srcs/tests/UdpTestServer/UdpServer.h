#pragma once
#include "UdpSender.h"

//====================================================================================================
// UdpServer
//====================================================================================================
class UdpServer
{
	
public :
	UdpServer(boost::asio::io_service& io_service, int port, std::vector<std::shared_ptr<UdpSender>> *senders);

private:
	void ReceiveSession();
	void ReceiveHandle(std::shared_ptr<UdpSession> session, const boost::system::error_code& ec, std::size_t transferred);
	void OnRegisterSession(std::shared_ptr<UdpSession> session);

private :

	std::vector<std::shared_ptr<UdpSender>> *_senders;
	boost::asio::ip::udp::socket	_server_socket;
	
	// 전송 multithread 처리
	int _session_insert_index = 0;
	
	friend struct UdpSession;

};