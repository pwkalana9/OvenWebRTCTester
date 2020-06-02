#include "common_hdr.h"
#include "signalling_server_object.h"
#include <regex>

/*
c->s : handshake(http request)
s->c : handshake(http response) 
c->s : command(request_offer) (json)
s->c : candidates(json) 
c->s : command(answer) 
c->s : command(candidate)
c->s : command(candidate)

ex) 

================================================================================================================================================================================================================================================================================================================================================================================================================

{"command":"request_offer"}

================================================================================================================================================================================================================================================================================================================================================================================================================
	"candidates" :
	[
		{
			"candidate":"candidate:0 1 UDP 50 192.168.0.233 10000 typ host",
			"sdpMLineIndex":0
		}
	],
	"id":"300259177",
	"sdp":
	{
		"sdp":"v=0\r\
				no=OvenMediaEngine 2426599745 2 IN IP4 127.0.0.1\r\n
				s=-\r\
				nt=0 0\r\n
				a=group:BUNDLE vH9o2O\r\n
				a=msid-semantic:WMS *\r\n
				a=fingerprint:sha-256 FF:80:73:CE:5A:67:85:A0:70:65:3B:CB:2E:79:39:D1:BA:95:E1:6D:E3:A5:52:A6:1C:FB:4B:67:15:CB:BD:7A\r\n
				a=ice-options:trickle\r\n
				a=ice-ufrag:hR2ogC\r\n
				a=ice-pwd:MBF1KVxzGhnRQfv7JarSA42TOZe5yc3D\r\n
				m=video 9 UDP/TLS/RTP/SAVPF 97\r\n
				c=IN IP4 0.0.0.0\r\n
				a=sendonly\r\n
				a=mid:vH9o2O\r\n
				a=setup:actpass\r\n
				a=rtcp-mux\r\n
				a=rtpmap:97 H264/90000\r\n
				a=fmtp:97 packetization-mode=1;profile-level-id=42e01f\r\n
				a=ssrc:1975182043 cname:jFJTVxBeQtHaWqMS\r\n",

		"type":"offer"
	}

================================================================================================================================================================================================================================================================================================================================================================================================================
"id":"300259177",
"command" : "answer",
"sdp" :
{
	"type":"answer",
		"sdp" : "v=0\r\n
				o=- 813268251950813878 2 IN IP4 127.0.0.1\r\n
				s=-\r\n
				t=0 0\r\n
				a=group:BUNDLE vH9o2O\r\n
				a=msid-semantic: WMS\r\n
				m=video 9 UDP/TLS/RTP/SAVPF 97\r\n
				c=IN IP4 0.0.0.0\r\n
				a=rtcp:9 IN IP4 0.0.0.0\r\n
				a=ice-ufrag:U2Pn\r\n
				a=ice-pwd:NlXsC9Z5zPpm457uU9k9i179\r\n
				a=ice-options:trickle\r\n
				a=fingerprint:sha-256 04:9F:A8:18:8B:1B:52:3E:F2:08:49:E8:37:6E:B3:F5:83:8B:F0:21:EA:98:AC:1E:D4:31:5B:7A:D3:27:61:AE\r\n
				a=setup:active\r\n
				a=mid:vH9o2O\r\n
				a=recvonly\r\n
				a=rtcp-mux\r\n
				a=rtpmap:97 H264/90000\r\n
				a=fmtp:97 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\n"
}

================================================================================================================================================================================================================================================================================================================================================================================================================
	"id":"300259177",
	"command":"candidate",
	"candidates":
	[
		{"candidate":"candidate:3191554232 1 udp 2113937151 192.168.0.230 54787 typ host generation 0 ufrag U2Pn network-cost 999",
		"sdpMid":"vH9o2O",
		"sdpMLineIndex":0,
		"usernameFragment":"U2Pn"}
	]
================================================================================================================================================================================================================================================================================================================================================================================================================
	"id":"300259177",
	"command":"candidate",
	"candidates":
	[
		{"candidate":"candidate:842163049 1 udp 1677729535 106.246.242.10 54787 typ srflx raddr 192.168.0.230 rport 54787 generation 0 ufrag U2Pn network-cost 999",
		"sdpMid":"vH9o2O",
		"sdpMLineIndex":0,
		"usernameFragment":"U2Pn"}
	]
================================================================================================================================================================================================================================================================================================================================================================================================================

*/


//====================================================================================================
// 생성자  
//====================================================================================================
SignallingServerObject::SignallingServerObject()
{
	_stream_key = -1; 
	_candidate_ip = 0;
	_candidate_port = 0;


	_peer_ufrag = ::RandomString(4);


	_current_mline_info = nullptr;

}

//====================================================================================================
// 소멸자   
//====================================================================================================
SignallingServerObject::~SignallingServerObject()
{
	Destroy();
}

//====================================================================================================
//  종료 
//====================================================================================================
void SignallingServerObject::Destroy()
{
  
}

//====================================================================================================
// 생성  
//====================================================================================================
bool SignallingServerObject::Create(int stream_key,
									std::string &remote_host,
									std::string &app_name,
									std::string &stream_name,
									TcpNetworkObjectParam *param)
{
	if(TcpNetworkObject::Create(param) == false)
	{
		return false; 
	}
	
	_stream_key		= stream_key;
	_remote_host	= remote_host;
	_app_name		= app_name;
	_stream_name	= stream_name;

	return true;
} 

//====================================================================================================
// handshake 요총 
//====================================================================================================
bool SignallingServerObject::SendHandshake()
{
	return WebSocketRequestObject::SendHandshake(_remote_host, _app_name + "/" + _stream_name);
}


//====================================================================================================
// handshake 완료 알림 처리
//====================================================================================================
bool SignallingServerObject::OnHandshakeComplete(bool result)
{
	if (result == true)
	{
		// request_offer 전송
		SendRequestOfferCommand(); 
	}

	return true;
}

//====================================================================================================
// signalling : request_offer 전송
//====================================================================================================
bool SignallingServerObject::SendRequestOfferCommand()
{
	std::string command = "{ \"command\":\"request_offer\" }";
	

	auto send_data = std::make_shared<std::vector<uint8_t>>();
	WebSocketRequestObject::MakeTextData(command, send_data);

	return PostSend(send_data);
}

//====================================================================================================
// signalling : answer command전송
/*
v=0\r\n
o=- 7719822503911727611 2 IN IP4 127.0.0.1\r\n
s=-\r\n
t=0 0\r\n
a=group:BUNDLE eEDyg9 r1D9jo\r\n
a=msid-semantic: WMS\r\n
m=video 9 UDP/TLS/RTP/SAVPF 97\r\n
c=IN IP4 0.0.0.0\r\n
a=rtcp:9 IN IP4 0.0.0.0\r\n
a=ice-ufrag:OgOg\r\n
a=ice-pwd:RMLTmttvLhxONrJeIxGihuMk\r\n
a=ice-options:trickle\r\n
a=fingerprint:sha-256 17:E5:DB:83:1E:4B:D1:6E:97:F8:E8:9A:35:EE:F1:9F:96:3A:AE:26:95:B3:04:25:42:6C:2B:89:AD:0A:32:36\r\n
a=setup:active\r\n
a=mid:eEDyg9\r\n
a=recvonly\r\n
a=rtcp-mux\r\n
a=rtpmap:97 VP8/90000\r\n
m=audio 9 UDP/TLS/RTP/SAVPF 113\r\n
c=IN IP4 0.0.0.0\r\n
a=rtcp:9 IN IP4 0.0.0.0\r\n
a=ice-ufrag:OgOg\r\n
a=ice-pwd:RMLTmttvLhxONrJeIxGihuMk\r\n
a=ice-options:trickle\r\n
a=fingerprint:sha-256 17:E5:DB:83:1E:4B:D1:6E:97:F8:E8:9A:35:EE:F1:9F:96:3A:AE:26:95:B3:04:25:42:6C:2B:89:AD:0A:32:36\r\n
a=setup:active\r\n
a=mid:r1D9jo\r\n
a=recvonly\r\n
a=rtcp-mux\r\n
a=rtpmap:113 OPUS/48000/2\r\n
a=fmtp:113 minptime=10;useinbandfec=1\r\n
*/
//====================================================================================================
bool SignallingServerObject::SendAnswerCommand()
{

	std::ostringstream data;
	std::string group_mids;


	for (auto mline_info : _mline_infos)
	{
		group_mids += " ";
		group_mids += mline_info->mid;
	}
	
	data << "{\r\n"
		<< "\"id\":" << _session_id << ",\r\n"
		<< "\"command\":\"answer\",\r\n"
		<< "\"sdp\":{"
		<< "\"type\":\"answer\",\r\n"
		<< "\"sdp\":\""
		<< "v=0\\r\\n"
		<< "o=- " << RandomNumberString(18) << " 2 IN IP4 127.0.0.1\\r\\n"
		<< "s=-\\r\\n"
		<< "t=0 0\\r\\n"
		<< "a=group:BUNDLE" << group_mids << "\\r\\n"
		<< "a=msid-semantic: WMS\\r\\n"; 

	for (auto mline_info : _mline_infos)
	{
		data << "m=" << mline_info->mline << "\\r\\n"
			<< "c=IN IP4 0.0.0.0\\r\\n"
			<< "a=rtcp:9 IN IP4 0.0.0.0\\r\\n"
			<< "a=ice-ufrag:" << _peer_ufrag << "\\r\\n"
			<< "a=ice-pwd:NlXsC9Z5zPpm457uU9k9i179\\r\\n"
			<< "a=ice-options:" << _offer_ice_option << "\\r\\n"
			<< "a=fingerprint:sha-256 04:9F:A8:18:8B:1B:52:3E:F2:08:49:E8:37:6E:B3:F5:83:8B:F0:21:EA:98:AC:1E:D4:31:5B:7A:D3:27:61:AE\\r\\n"
			<< "a=setup:active\\r\\n"
			<< "a=mid:" << mline_info->mid << "\\r\\n"
			<< "a=recvonly\\r\\n"
			<< "a=rtcp-mux\\r\\n"
			<< "a=" << mline_info->rtpmap_line << "\\r\\n";
	}
	
	// << "a=fmtp:97 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\\r\\n\""
		
	data << "\"}\r\n"
		 << "}";

	auto send_data = std::make_shared<std::vector<uint8_t>>();
	std::string  text_data = data.str(); 
	WebSocketRequestObject::MakeTextData(text_data, send_data);
		 
	return PostSend(send_data);
}

//====================================================================================================
// signalling : candidate command 전송
//====================================================================================================
bool SignallingServerObject::SendCandidateCommand()
{
	return true;
}

//====================================================================================================
// 응답 데이터 처리
//====================================================================================================
bool SignallingServerObject::OnRecvWebsocketData(std::shared_ptr<std::vector<uint8_t>> &payload, WebsocketOpcode op_code)
{
	if (op_code == WebsocketOpcode::TextFrame)
	{
		std::string text_data = (char *)payload->data();

		//Json 파싱

		Json::Value root;
		Json::CharReaderBuilder builder;
		std::unique_ptr<::Json::CharReader> const reader(builder.newCharReader());
		std::string error;

		bool result = reader->parse(static_cast<const char *>(text_data.c_str()), static_cast<const char *>(text_data.c_str()) + text_data.size(), &root, &error);

		if (!result)
		{
			LOG_WRITE(("ERROR : Json Root Load Error - error(%s)", error.c_str()));
			((ISignallingServerCallback *)_object_callback)->OnJsonDataFail_SignallingServer(_index_key, _stream_key);
			return false;
		}

		if (!ParsingSignallingData(root))
		{
			LOG_WRITE(("ERROR : Json Parsing fail"));
			((ISignallingServerCallback *)_object_callback)->OnJsonDataFail_SignallingServer(_index_key, _stream_key);
			return false;
		}

		// Answer 전송
		SendAnswerCommand();

		// Candidate 전송
		SendCandidateCommand();

		// Complete 알림
		SignallingCompleteInfo complete_info; 
		complete_info.ome_stram_ip	= _candidate_ip;
		complete_info.ome_stram_port= _candidate_port;
		complete_info.peer_ufrag	= _peer_ufrag;
		complete_info.offer_ufrag	= _offer_ufrag;
		complete_info.offer_pwd		= _offer_pwd;

	
		// Track 정보 구성
		auto track_infos = std::make_shared<std::vector<StreamTrackInfo>>();
		for (auto mline_info : _mline_infos)
		{
			track_infos->push_back(mline_info->track_info); 

		}
		_mline_infos.clear(); 

		// 콜백 호출 
		if (!((ISignallingServerCallback *)_object_callback)->OnHandshakeComplete_SignallingServer(_index_key, _stream_key, complete_info, track_infos))
		{
			LOG_WRITE(("ERROR : OnHandshakeComplete_SignallingServer fail"));
		}

	}

	return true; 
}

//====================================================================================================
// 데이터 파싱(json) 
//====================================================================================================
bool SignallingServerObject::ParsingSignallingData(Json::Value &root)
{
	Json::Value &value = root["candidates"];

	if (value.isNull() || !value.isArray())
	{
		return false;
	}
	auto candidate_iterator = value.begin();
		
	if (!ParsingCandidate((*candidate_iterator)["candidate"].asString()))
	{
		return false;
	}

	// ID 
	value = root["id"]; 
	if (value.isNull()) // number or string 
	{
		return false;
	}
	_session_id = value.asString();

	// SDP
	value = root["sdp"];
	if (value.isNull() || !value.isObject())
	{
		return false;
	}

	if (value["sdp"].isString() == false)
	{
		return false;
	}

	_candidate_offer_sdp = value["sdp"].asString();
	ParsingSdp(_candidate_offer_sdp);


	/*
	std::ostringstream log_data;
	log_data << "\n"
		<< "offer mline : "			<< _offer_mline			<< "\n"
		<< "offer ufrage : "		<< _offer_ufrag			<< "\n"
		<< "offer pwd : "			<< _offer_pwd			<< "\n"
		<< "offer mid : "			<< _offer_mid			<< "\n"
		<< "offer rtpmap : "		<< _offer_rtpmap		<< "\n"
		<< "offer ice_potions : "	<< _offer_ice_option	<< "\n"
		<< "candidate ip : "		<< _candidate_ip_string << "\n"
		<< "candidate port : "		<< _candidate_port		<< "\n"
		<< "session id : "			<< _session_id		<< "\n"
		<< "offer sdp : "			<< _candidate_offer_sdp << "\n";
	
	LOG_WRITE((log_data.str().c_str()));
	*/
	return true;
}

//====================================================================================================
// candidate 문자 파싱
// ex) candidate:0 1 UDP 50 192.168.0.233 10000 typ host
//====================================================================================================
bool SignallingServerObject::ParsingCandidate(std::string candidate)
{
	std::vector<std::string> tokens; 

	Tokenize2(candidate.c_str(), tokens, ' ');

	if (tokens.size() < 7 || tokens[0].find("candidate:") == std::string::npos)
	{
		return false;
	}
	
	std::string foundation	= tokens[0].substr(strlen("candidate:"));
	std::string	component_id= tokens[1];
	std::string transport	= tokens[2];
	std::string priority	= tokens[3];
	std::string ip			= tokens[4];
	std::string port		= tokens[5];
	std::string cand_type	= tokens[6];

	_candidate_ip_string = ip;
	_candidate_ip = inet_addr(ip.c_str());
	_candidate_port = strtol(port.c_str(), nullptr, 10);

	return true;
}

//====================================================================================================
// sdp 문자 파싱
//====================================================================================================
bool SignallingServerObject::ParsingSdp(std::string &sdp)
{
	std::stringstream sdpstream(sdp);
	std::string line;

	while (std::getline(sdpstream, line, '\n'))
	{
		if (line.size() && line[line.length() - 1] == '\r')
		{
			line.pop_back();
		}

		char type = line[0];
		std::string content = line.substr(2);

		//LOG_WRITE(("INFO : line - %s", content.c_str()));

		if (ParsingMediaLine(type, content) == false)
		{
			LOG_WRITE(("ERROR : Media line parsing fail"));
			return false;
		}
	}

	if (_current_mline_info != nullptr)
	{
		_mline_infos.push_back(_current_mline_info);
	}

	return true;
}

//====================================================================================================
// sdp line 문자 파싱
//====================================================================================================
bool SignallingServerObject::ParsingMediaLine(char type, std::string line)
{
	std::smatch matches;

	// m line parsing
	if (type == 'm')
	{
		if (_current_mline_info == nullptr)
		{
			_current_mline_info = std::make_shared<MlineInfo>();
			
		}
		else
		{
			_mline_infos.push_back(_current_mline_info);
			_current_mline_info = std::make_shared<MlineInfo>();
		}

		if (_current_mline_info != nullptr && std::regex_search(line, matches, std::regex("^(\\S*) (\\S*) (\\S*) (\\d*)")) && matches.size() >= 5)
		{
			if (std::string(matches[1]) == "video")		_current_mline_info->track_info.type = StreamTrackType::Video;
			else if (std::string(matches[1]) == "audio")_current_mline_info->track_info.type = StreamTrackType::Audio;
			else										_current_mline_info->track_info.type = StreamTrackType::Unknown;
		}

		// 첫 트랙 정보만 저장
		//_current_mline_info->mline = line;
		_current_mline_info->mline = matches[0];
	}
	// m line parsing
	else if (_current_mline_info != nullptr && std::regex_search(line, matches, std::regex("^mid:(\\S*)")) && matches.size() >= 2)
	{
		_current_mline_info->mid = std::string(matches[1]).c_str();
	}
	// m line parsing( ex) rtpmap:113 OPUS/48000/2) - 첫 트랙 정보만 저장
	else if (_current_mline_info != nullptr && 
		_current_mline_info->rtpmap_line.empty() && 
		std::regex_search(line, matches, std::regex("rtpmap:(\\d*) ([\\w\\-\\.]*)(?:\\s*\\/(\\d*)(?:\\s*\\/(\\S*))?)?")) && matches.size() >= 4)
	{
		_current_mline_info->rtpmap_line = line;
		
		// 코덱 정보,track id, timescale 파싱
		_current_mline_info->track_info.track_id = strtoul(std::string(matches[1]).c_str(), NULL, 10);

		std::string codec_name = matches[2];

		if (codec_name == "VP8")		_current_mline_info->track_info.codec_type = StreamCodecType::VP8;
		else if (codec_name == "H264")	_current_mline_info->track_info.codec_type = StreamCodecType::H264;
		else if (codec_name == "OPUS")	_current_mline_info->track_info.codec_type = StreamCodecType::OPUS;
		else if (codec_name == "AAC")	_current_mline_info->track_info.codec_type = StreamCodecType::AAC;

		_current_mline_info->track_info.timescale = strtoul(std::string(matches[3]).c_str(), NULL, 10);
	}
	else if (_offer_ufrag.empty() && std::regex_search(line, matches, std::regex("^ice-ufrag:(\\S*)")) && matches.size() >= 2)
	{
		_offer_ufrag = std::string(matches[1]).c_str();
	}
	else if (_offer_ufrag.empty() && std::regex_search(line, matches, std::regex("^ice-ufrag:(\\S*)")) && matches.size() >= 2)
	{
		_offer_ufrag = std::string(matches[1]).c_str();
	}
	else if (_offer_pwd.empty() && std::regex_search(line, matches, std::regex("^ice-pwd:(\\S*)")) && matches.size() >= 2)
	{
		_offer_pwd = std::string(matches[1]).c_str();
	}
	else if (_offer_ice_option.empty() && std::regex_search(line, matches, std::regex("^ice-options:(\\S*)")) && matches.size() >= 2)
	{
		_offer_ice_option = std::string(matches[1]).c_str();
	}
	return true;
}
