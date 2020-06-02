#pragma once

#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <memory>

#include "./semaphore.h"

class UdpServer; // forward declaration
class UdpSender;

//====================================================================================================
// Protocol Header
//====================================================================================================
struct TrafficTestHeader
{
	uint32_t command;
	uint32_t stream_key;
	uint64_t sequence_number = 0;
	uint64_t timestamp = 0;
	uint32_t packet_size = 0; // header + data size 
	uint8_t data[0];
};
#define TRAFFIC_TEST_START			(0) 
#define TRAFFIC_TEST_DATA			(1)
#define TRAFFIC_SEND_INTERVAL		(5) //ms  1sec/30fps --> 33ms --> 30ms 
#define TRANFFIC_SEND_DATA_SIZE		(1500) // byte
#define TRAFFIC_SEND_CONUT			(5) //

#define UDP_SERVER_PORT				(3334)
#define UDP_RECEIVE_BUFFER_SIZE		(8192)
//====================================================================================================
// UdpSession
//====================================================================================================
struct UdpSession : public std::enable_shared_from_this<UdpSession> 
{

	UdpSession(UdpServer* server) : _server(server) {}

	void RequestHandle(const boost::system::error_code& error, std::size_t transferred);

	void SentHandle(std::shared_ptr<std::vector<uint8_t>> const &data, const boost::system::error_code& ec, std::size_t)
	{
		// here response has been sent
		if (ec) 
		{
			printf("Sending Error : endpoint(%s:%d) message(%s) \n", 
				_remote_endpoint.address().to_string().c_str(), 
				_remote_endpoint.port(), 
				ec.message().c_str());
		}
	}

	void SetSender(std::shared_ptr<UdpSender> &sender) 
	{
		_sender = sender;
	}


	boost::asio::ip::udp::endpoint _remote_endpoint;
	boost::array<uint8_t, UDP_RECEIVE_BUFFER_SIZE> _recv_buffer;
	std::shared_ptr<UdpServer> _server;
	std::shared_ptr<UdpSender> _sender = nullptr;
};

//====================================================================================================
// UdpSender
//====================================================================================================
class UdpSender
{

public :
	UdpSender(boost::asio::io_service& io_service, int timer_interval);
	
public :
	void OnRegisterSession(std::shared_ptr<UdpSession> session);
	void TrafficSend(std::shared_ptr<std::vector<uint8_t>> const &data, std::shared_ptr<UdpSession> const& session);

	void OnTrafficSendTimer();

private:

	int _timer_interval;
	boost::asio::ip::udp::socket _socket;
	boost::asio::deadline_timer _traffic_send_timer;
	uint64_t _sequence_number = 0;

	std::vector<std::shared_ptr<UdpSession>> _sessions;
	std::mutex _session_mutex;
	
	friend struct UdpSession;

};

