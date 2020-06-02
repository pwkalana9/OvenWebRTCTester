#include "test_udp_server_map.h" 

//====================================================================================================
// 생성자 
//====================================================================================================
TestUdpServerMap::TestUdpServerMap(int object_key) : UdpNetworkManager(object_key)
{
	 
}

//====================================================================================================
// Object 추가(연결) 
//====================================================================================================
#include "boost/asio.hpp"
#include <boost/asio/basic_datagram_socket.hpp>
bool TestUdpServerMap::ConnectAdd(ITestUdpServerCallback *callback,
								uint32_t remote_ip, 
								int remote_port, 
								int stream_key, 
								int &index_key)
{
	std::shared_ptr<UdpNetworkObject>	object;
	UdpNetworkObjectParam 	object_param;

	NetUdpSocket *socket = new NetUdpSocket(*(_service_pool->GetService()), NetUdpEndPoint(boost::asio::ip::udp::v4(), 0));

	boost::asio::socket_base::receive_buffer_size  option(4000000);
	socket->set_option(option);
	
	socket->non_blocking(true);
	
	//Object Param 설정 	
	object_param.object_key			= _object_key;
	object_param.remote_ip			= remote_ip;
	object_param.remote_port		= remote_port;
	object_param.socket				= socket;
	object_param.network_callback	= _network_callback;
	object_param.object_callback	= callback;
	object_param.object_name		= _object_name;
	
	// Object 생성(스마트 포인터) 
	object = std::shared_ptr<UdpNetworkObject>(new TestUdpServerObject());
	if (!std::static_pointer_cast<TestUdpServerObject>(object)->Create(&object_param, stream_key))
	{
		index_key = -1;
		return false;
	}

	// Object 정보 추가 
	index_key = Insert(object);

	return index_key == -1 ? false : true;
}
//====================================================================================================
// 트래픽 정보 수집
//====================================================================================================
bool TestUdpServerMap::GetTrafficInfo(int &session_count, uint64_t &network_traffc, uint32_t &packet_loss, uint32_t &packet_count)
{
	double session_send_bitrate;
	double session_recv_bitrate;
	uint32_t session_packet_loss;
	uint32_t session_packet_count;


	LOCK_NETWORK_INFO

	session_count = _network_info_map.size(); 

	for (const auto &item : _network_info_map)
	{
		if (item.second != nullptr)
		{
			if (std::static_pointer_cast<TestUdpServerObject>(item.second)->GetTrafficRate(session_send_bitrate, session_recv_bitrate))
			{
				network_traffc += session_recv_bitrate;
			}

			std::static_pointer_cast<TestUdpServerObject>(item.second)->GetPacketInfo(session_packet_loss, session_packet_count);
			packet_loss += session_packet_loss; 
			packet_count += session_packet_count;
		}
	}
		
	return true;
}