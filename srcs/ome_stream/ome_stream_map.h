#pragma once
#include "network/engine/udp_network_manager.h"  
#include "ome_stream_object.h" 

//====================================================================================================
// OmeStreamMap
//====================================================================================================
class OmeStreamMap : public UdpNetworkManager
{
public:
	OmeStreamMap(int object_key);

public : 
	bool ConnectAdd(IOmeStreamCallback *callback,
					uint32_t remote_ip,
					int remote_port,
					int stream_key,
					SignallingCompleteInfo &complete_info,
					std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos,
					int &index_key);

	bool SendKeepalive(); 

	bool GetTrafficCollectData(std::vector<std::shared_ptr<TrfficCollectData>> &traffic_collect_infos);

private : 

	
}; 
