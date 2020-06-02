
#include "UdpSender.h"
#include "UdpServer.h"

//====================================================================================================
//UdpSession : RequestHandle 
//====================================================================================================
void UdpSession::RequestHandle(const boost::system::error_code& error, std::size_t transferred)
{
	if (!error || error == boost::asio::error::message_size)
	{
		TrafficTestHeader * header = (TrafficTestHeader *)_recv_buffer.data();

		if (transferred != header->packet_size)
		{
			printf("WARNING : data size error - data(%lu) header(%u)", transferred, header->packet_size);
			return;
		}

		if (header->command == TRAFFIC_TEST_START)
		{
			// session 등록
			_server->OnRegisterSession(shared_from_this());
		}
	}
}

//====================================================================================================
// UdpSender
//====================================================================================================
UdpSender::UdpSender(boost::asio::io_service& io_service, int timer_interval)
	: _socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)),
	_traffic_send_timer(io_service, boost::posix_time::milliseconds(timer_interval)),
	_timer_interval(timer_interval)
{
	_traffic_send_timer.async_wait(boost::bind(&UdpSender::OnTrafficSendTimer, this));
}

//====================================================================================================
// OnRegisterSession
//====================================================================================================
void UdpSender::OnRegisterSession(std::shared_ptr<UdpSession> session)
{
	std::unique_lock<std::mutex> session_lock(_session_mutex);
	_sessions.push_back(session);
}

//====================================================================================================
// Response
//====================================================================================================
void UdpSender::TrafficSend(std::shared_ptr<std::vector<uint8_t>> const &data, std::shared_ptr<UdpSession> const& session)
{

	_socket.async_send_to(boost::asio::buffer(*data), session->_remote_endpoint,
		bind(&UdpSession::SentHandle,
			session, // keep-alive of buffer/endpoint
			data,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		));
}

//====================================================================================================
// Response
//====================================================================================================
void UdpSender::OnTrafficSendTimer()
{
	std::unique_lock<std::mutex> session_lock(_session_mutex);

	//send data 
	//for (int index = 0; index < TRAFFIC_SEND_CONUT; index++)
	{

		auto data = std::make_shared<std::vector<uint8_t>>(TRANFFIC_SEND_DATA_SIZE);
		TrafficTestHeader * header = (TrafficTestHeader *)data->data();
		header->command = TRAFFIC_TEST_DATA;
		header->sequence_number = _sequence_number++;
		header->timestamp = time(nullptr);
		header->packet_size = TRANFFIC_SEND_DATA_SIZE;

		for (int index = 0; index < _sessions.size(); index++)
		{
			TrafficSend(data, _sessions[index]);
		}

	}

	_traffic_send_timer.expires_at(_traffic_send_timer.expires_at() + boost::posix_time::milliseconds(_timer_interval));
	_traffic_send_timer.async_wait(boost::bind(&UdpSender::OnTrafficSendTimer, this));

	return;
}
