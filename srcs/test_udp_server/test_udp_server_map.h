#pragma once
#include "network/engine/udp_network_manager.h"  
#include "test_udp_server_object.h" 

//====================================================================================================
// TestUdpServerMap
//====================================================================================================
class TestUdpServerMap : public UdpNetworkManager
{
public:
	TestUdpServerMap(int object_key);

public : 
	bool ConnectAdd(ITestUdpServerCallback *callback,
					uint32_t remote_ip,
					int remote_port,
					int stream_key,
					int &index_key);

	bool GetTrafficInfo(int &session_count, uint64_t &network_traffc, uint32_t &packet_loss, uint32_t &packet_count);
private : 
	

	
}; 
