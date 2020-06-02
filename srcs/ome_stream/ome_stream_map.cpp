#include "ome_stream_map.h" 

//====================================================================================================
// 생성자 
//====================================================================================================
OmeStreamMap::OmeStreamMap(int object_key) : UdpNetworkManager(object_key)
{
	 
}

//====================================================================================================
// Object 추가(연결) 
//====================================================================================================
#include "boost/asio.hpp"
#include <boost/asio/basic_datagram_socket.hpp>
bool OmeStreamMap::ConnectAdd(IOmeStreamCallback *callback, 
								uint32_t remote_ip, 
								int remote_port, 
								int stream_key, 
								SignallingCompleteInfo &complete_info, 
								std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos,
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
	object = std::shared_ptr<UdpNetworkObject>(new OmeStreamObject());
	if (!std::static_pointer_cast<OmeStreamObject>(object)->Create(&object_param, stream_key, complete_info, track_infos))
	{
		index_key = -1;
		return false;
	}

	// Object 정보 추가 
	index_key = Insert(object);

	return index_key == -1 ? false : true;
}

//====================================================================================================
// 전체 세션에 BindingRequest 재전송
//====================================================================================================
bool OmeStreamMap::SendKeepalive()
{
	bool result = false;

	LOCK_NETWORK_INFO

	for (auto &item : _network_info_map)
	{
		if (item.second != nullptr)
		{
			result = std::static_pointer_cast<OmeStreamObject>(item.second)->SendBindingRequest();
		}
	}
	   
	return result;
}

//====================================================================================================
// 트래픽 정보 수집
// - 정식적으로 데이터 수집 session만 정보 처리
//====================================================================================================
bool OmeStreamMap::GetTrafficCollectData(std::vector<std::shared_ptr<TrfficCollectData>> &traffic_collect_infos)
{
	LOCK_NETWORK_INFO

	for (const auto &item : _network_info_map)
	{
		if (item.second != nullptr)
		{
			auto traffic_collect_data = std::static_pointer_cast<OmeStreamObject>(item.second)->GetTrafficCollectData();
			if (traffic_collect_data != nullptr)
			{
				traffic_collect_infos.push_back(traffic_collect_data);
			}
		}
	}

	return true;
}