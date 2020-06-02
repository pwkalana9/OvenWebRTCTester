#pragma once
#include "../common/network/protocol_object/web_socket_request_object.h"
#include "stream_manager.h"
#include "../jsoncpp/json/json.h"

//====================================================================================================
// Interface 
//====================================================================================================
class ISignallingServerCallback
{
public:
	virtual bool OnJsonDataFail_SignallingServer(int index_key,int stream_key) = 0;

	virtual bool OnHandshakeComplete_SignallingServer(int index_key, 
													int stream_key, 
													SignallingCompleteInfo &complete_info,
													std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos) = 0;


};

//====================================================================================================
// SignallingServerObject
//====================================================================================================
class SignallingServerObject : public WebSocketRequestObject
{
public :
	SignallingServerObject();
	virtual ~SignallingServerObject();

public :
	bool Create(int stream_key, 
				std::string &remote_host,
				std::string &app_name,
				std::string &stream_name,
				TcpNetworkObjectParam *param);
	void Destroy();
	
	bool SendHandshake();

	// command
	bool SendRequestOfferCommand(); 
	bool SendAnswerCommand(); 
	bool SendCandidateCommand(); 

	int GetStreamKey() { return _stream_key; } 

protected :
	// WebSocketRequestObject 함수 구현
	virtual bool OnHandshakeComplete(bool result);
	virtual bool OnRecvWebsocketData(std::shared_ptr<std::vector<uint8_t>> &payload, WebsocketOpcode op_code);

	bool ParsingSignallingData(Json::Value &root);
	bool ParsingCandidate(std::string candidate);
	bool ParsingSdp(std::string &sdp);
	bool ParsingMediaLine(char type, std::string content);

private : 
	int			_stream_key;
	std::string _remote_host;
	std::string _app_name;
	std::string _stream_name;
	std::string _session_id;

	std::string _peer_ufrag;
	


	struct MlineInfo
	{
		std::string mline;
		std::string mid;
		std::string rtpmap_line;

		StreamTrackInfo track_info; 
	};

	std::vector<std::shared_ptr<MlineInfo>> _mline_infos; 
	std::shared_ptr<MlineInfo> _current_mline_info;

	

	std::string _offer_ufrag;
	std::string _offer_pwd;
	std::string _offer_ice_option;

	uint32_t	_candidate_ip; 
	std::string _candidate_ip_string;
	int			_candidate_port; 
	std::string _candidate_offer_sdp;
};
