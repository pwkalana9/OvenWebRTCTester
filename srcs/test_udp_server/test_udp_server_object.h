#pragma once
#include "network/engine/udp_network_object.h" 
#include "stream_manager.h"


struct TrafficTestHeader
{
	uint32_t command;
	uint32_t stream_key;
	uint64_t sequence_number = 0;
	uint64_t timestamp = 0;
	uint32_t packet_size = 0; // header + data size 
	uint8_t data[0];
};
#define TRAFFIC_TEST_START (0) 
#define TRAFFIC_TEST_DATA  (1)
#define TRAFFIC_SEND_INTERVAL		(30) //ms  1sec/30fps --> 33ms --> 30ms 
#define TRANFFIC_SEND_DATA_SIZE		(1500) // byte
#define TRAFFIC_SEND_CONUT			(5) //



//====================================================================================================
// Interface
//====================================================================================================
class ITestUdpServerCallback
{

public:
};

//====================================================================================================
//TestUdpServerObject 
//====================================================================================================
class TestUdpServerObject : public UdpNetworkObject
{
public :
	TestUdpServerObject();
	virtual ~TestUdpServerObject();
	
public : 
	virtual bool Create(UdpNetworkObjectParam *param, int stream_key);
	
	void GetPacketInfo(uint32_t &packet_loss, uint32_t &packet_count)
	{
		packet_loss = _packet_loss;
		packet_count = _packet_count;
		_packet_loss = 0;
		_packet_count = 0;
	}

protected :
	bool SendStartRequest();
	
	// UdpNetworkObject ±¸Çö
	virtual void	RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data);
	virtual bool	OnHandshakeComplete(bool result) { return true; }
	
	bool TrafficTestDataProc(TrafficTestHeader * header, const std::shared_ptr<std::vector<uint8_t>> &data);

private : 
	int		_stream_key;
	int64_t	_last_recv_time = 0; 
	uint64_t _last_sequence_number = 0;
	uint32_t _packet_loss = 0; 
	uint32_t _packet_count = 0;


};


