#include "test_udp_server_object.h"
#include <sstream>





//====================================================================================================
// 생성자  
//====================================================================================================
TestUdpServerObject::TestUdpServerObject() : UdpNetworkObject(true)
{
	_stream_key				= -1; 
	_last_recv_time			= GetCurrentMilliSecond();

}

//====================================================================================================
// 소멸자 
//====================================================================================================
TestUdpServerObject::~TestUdpServerObject()
{	

}

//====================================================================================================
// Create
//====================================================================================================
bool TestUdpServerObject::Create(UdpNetworkObjectParam *param, int stream_key)
{
	if (UdpNetworkObject::Create(param) == false)
	{
		return false;
	}

 	_stream_key = stream_key;

	SendStartRequest();

	return true;

}


//====================================================================================================
// Traffic Test시작 요청
//====================================================================================================
bool  TestUdpServerObject::SendStartRequest()
{
	auto data = std::make_shared<std::vector<uint8_t>>(TRANFFIC_SEND_DATA_SIZE);
	TrafficTestHeader * header = (TrafficTestHeader *)data->data();
	header->command = TRAFFIC_TEST_START;
	header->sequence_number = 0;
	header->timestamp = time(nullptr);
	header->packet_size = TRANFFIC_SEND_DATA_SIZE; // header + data

	return PostSend(data);;
}


//====================================================================================================
//  패킷 수신 
//====================================================================================================
void TestUdpServerObject::RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data)
{
	_last_recv_time = GetCurrentMilliSecond();

	_packet_count++; 

	TrafficTestHeader * header = (TrafficTestHeader *)data->data();

	if (data->size() != header->packet_size)
	{
		LOG_WRITE(("WARNING : data size error - data(%d) header(%d)", data->size(), header->packet_size));
		return; 
	}

	if (header->command == TRAFFIC_TEST_DATA)
	{
		TrafficTestDataProc(header, data); 
	}
	
}

//====================================================================================================
// Traffic Test data 처리 
// - sequence number 확인 
// - latency 확인
//====================================================================================================
bool TestUdpServerObject::TrafficTestDataProc(TrafficTestHeader * header, const std::shared_ptr<std::vector<uint8_t>> &data)
{
	if (_last_sequence_number < header->sequence_number)
	{
		if (header->sequence_number != _last_sequence_number + 1 && _last_sequence_number != 0)
		{
			_packet_loss += (header->sequence_number - _last_sequence_number - 1); 
			// LOG_WRITE(("WARNING : sequence number fail - stream(%d) sequence(%u/%u)  fail(%u)", _stream_key, _last_sequence_number, header->sequence_number, _packet_loss));
		}
		_last_sequence_number = header->sequence_number; 
	}
	else
	{
		 // LOG_WRITE(("WARNING : sequence number fail - stream(%d) sequence(%u/%u) ", _stream_key, _last_sequence_number, header->sequence_number));
	}



	return true; 
}
