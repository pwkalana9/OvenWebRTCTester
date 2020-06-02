
#include "UdpServer.h"

//====================================================================================================
// UdpServer
//====================================================================================================
UdpServer::UdpServer(boost::asio::io_service& io_service, int port, std::vector<std::shared_ptr<UdpSender>> *senders) :	
		_server_socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)),
		_senders(senders)
{
	ReceiveSession();
}

//====================================================================================================
// ReceiveSession
//====================================================================================================
void UdpServer::ReceiveSession()
{
	// our session to hold the buffer + endpoint
	auto session = std::make_shared<UdpSession>(this);


	_server_socket.async_receive_from(
		boost::asio::buffer(session->_recv_buffer),
		session->_remote_endpoint,
		bind(&UdpServer::ReceiveHandle, 
				this,
				session,  
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}

//====================================================================================================
// ReceiveHandle
//====================================================================================================
void UdpServer::ReceiveHandle(std::shared_ptr<UdpSession> session, const boost::system::error_code& ec, std::size_t transferred)
{
	// now, handle the current session on any available pool thread
	_server_socket.get_io_service().post(bind(&UdpSession::RequestHandle, session, ec, transferred));
	
	// immediately accept new datagrams
	ReceiveSession();
}

//====================================================================================================
// OnRegisterSession
//====================================================================================================
void UdpServer::OnRegisterSession(std::shared_ptr<UdpSession> session)
{
	auto sender = _senders->at((_session_insert_index++) % _senders->size());

	session->SetSender(sender);

	// Sender에 endpoint 등록
	sender->OnRegisterSession(session); 

}
