#pragma once
#include "network/engine/udp_network_object.h" 
#include "stream_manager.h"
#include "dtls_transport.h"

//====================================================================================================
// Interface
//====================================================================================================
class IOmeStreamCallback
{

public:
	virtual bool OnStunComplete_OmeStream(int index_key, int stream_key) = 0;
	virtual bool OnDtlsComplete_OmeStream(int index_key, int stream_key) = 0;
	virtual bool OnStreamingStart_OmeStream(int index_key, int stream_key) = 0;



};

//====================================================================================================
//OmeStreamObject 
//====================================================================================================
class OmeStreamObject : public UdpNetworkObject, public IDtlsTransportCallback
{
public :
	OmeStreamObject();
	virtual ~OmeStreamObject();
	
public : 
	virtual bool Create(UdpNetworkObjectParam *param, int stream_key, SignallingCompleteInfo &complete_info, std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos);
	bool	SendBindingRequest();
	bool	SendBindingResponse();
	std::shared_ptr<TrfficCollectData> GetTrafficCollectData();
		
	// 5초 이상 수신 데이터 없으면 실패
	bool IsStreaming()
	{
		return (GetCurrentMilliSecond() - _last_recv_time) <  5000;
 	}

protected :
	

	bool	MakeBindingRequestPacket(std::shared_ptr<std::vector<uint8_t>> &packet);
	bool	MakeBindingResponsePacket(std::shared_ptr<std::vector<uint8_t>> &packet);

	bool	StunParsing(const std::shared_ptr<std::vector<uint8_t>> &data);

	// UdpNetworkObject 구현
	virtual void	RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data);
	virtual bool	OnHandshakeComplete(bool result) { return true; }
	
	bool	RecvBindingSuccessResponse();
	bool	RecvBindingRequest();

	//IDtlsTransportCallback 구현 
	virtual bool OnDataSend_DtlsTransport(std::shared_ptr<std::vector<uint8_t>> &packet);
	virtual bool OnRtpStart_DtlsTransport(uint32_t timescal, uint32_t timestamp);
	virtual bool OnDtlsComplete_DtlsTransport();

private : 
	int						_stream_key;
	SignallingCompleteInfo	_complete_info;

	std::vector<uint8_t>	_peer_transcation_id; 
	std::vector<uint8_t>	_offer_transcation_id;

	bool					_is_completed;
	time_t					_binding_request_time;

	std::shared_ptr<DtlsTransport> _dtls_transport; 

	int64_t					_last_recv_time = 0; 

	std::shared_ptr<std::vector<uint8_t>> _bind_request_packet = nullptr; 
	std::shared_ptr<std::vector<uint8_t>> _bind_response_packet = nullptr;



};


