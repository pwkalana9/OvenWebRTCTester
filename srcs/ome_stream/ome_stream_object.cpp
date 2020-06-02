#include "ome_stream_object.h"
#include <sstream>


// ********** Stun **********
// c->s : BindingRequest 
// s->c : BindingSuccessResponse
// s->c : BindingRequest
// c->s:  BindingSuccessResponse

// RFC5389, section 6
// All STUN messages MUST start with a 20-byte header followed by zero
// or more Attributes.  The STUN header contains a STUN message type,
// 		magic cookie, transaction ID, and message length.
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |0 0|     STUN Message Type     |         Message Length        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         Magic Cookie                          |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                                                               |
// |                     Transaction ID (96 bits)                  |
// |                                                               |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Figure 2: Format of STUN Message Header


#define STUN_HEADER_LENGTH					(sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + (sizeof(uint8_t) * 12))
#define STUN_TRANSACTION_ID_LENGTH			(12)
#define STUN_FINGERPRINT_XOR_VALUE			(0x5354554E)
#define STUN_MAGIC_COOKIE					(0x2112A442)
// SHA1 digest 크기와 동일
#define STUN_HASH_LENGTH					(20)
#define STUN_LENGTH_AND_VALUE				(0x03)
#define STUN_FINGERPRINT_ATTRIBUTE_LENGTH	(8)
static unsigned long crc_table[256];
static int crc_table_computed = false;

static void MakeCrcTable()
{
	unsigned long c;

	int n, k;
	for (n = 0; n < 256; n++)
	{
		c = (unsigned long)n;
		for (k = 0; k < 8; k++)
		{
			if (c & 1)
			{
				c = 0xEDB88320L ^ (c >> 1L);
			}
			else
			{
				c = c >> 1;
			}
		}
		crc_table[n] = c;
	}

	crc_table_computed = true;
}

static unsigned long MakeCrc(unsigned long initial, const unsigned char *buf, uint64_t len)
{
	unsigned long c = initial ^ 0xFFFFFFFFL;
	uint64_t n;

	if (crc_table_computed == false)
	{
		MakeCrcTable();
	}
	for (n = 0; n < len; n++)
	{
		c = crc_table[(c ^ buf[n]) & 0xFF] ^ (c >> 8);
	}
	return c ^ 0xFFFFFFFFL;
}

//====================================================================================================
// 생성자  
//====================================================================================================
OmeStreamObject::OmeStreamObject() : UdpNetworkObject(true)
{
	_stream_key				= -1; 
	_binding_request_time	= 0; 
	_is_completed			= false;
	_dtls_transport			= nullptr;
	_last_recv_time			= GetCurrentMilliSecond();

}

//====================================================================================================
// 소멸자 
//====================================================================================================
OmeStreamObject::~OmeStreamObject()
{	

}

//====================================================================================================
// Create
//====================================================================================================
bool OmeStreamObject::Create(UdpNetworkObjectParam *param, int stream_key, SignallingCompleteInfo &complete_info, std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos)
{
	if (UdpNetworkObject::Create(param) == false)
	{
		return false;
	}

 	_stream_key = stream_key;
	_complete_info = complete_info;
	_binding_request_time = time(nullptr); 
	
	_dtls_transport = std::make_shared<DtlsTransport>(stream_key, track_infos);
	
	SetKeepAliveSendTimer(1000, static_cast<bool (UdpNetworkObject::*)()>(&OmeStreamObject::SendBindingRequest));

	return true;

}

//====================================================================================================
// Binding Request 요청 패킷 
// - 바로 Binding Request를 요청 하면 OME에서 아직 스트리 설정이 안되 있음
//====================================================================================================
bool OmeStreamObject::SendBindingRequest()
{
	if (_bind_request_packet == nullptr)
	{
		_bind_request_packet = std::make_shared<std::vector<uint8_t>>();
		MakeBindingRequestPacket(_bind_request_packet);
	}

	return PostSend(_bind_request_packet);;
}

//====================================================================================================
// Binding Success 응답 패킷 
//====================================================================================================
bool OmeStreamObject::SendBindingResponse()
{
	if (_bind_response_packet == nullptr)
	{
		_bind_response_packet = std::make_shared<std::vector<uint8_t>>();
		MakeBindingResponsePacket(_bind_response_packet);
	}
	
	return PostSend(_bind_response_packet);
}


//====================================================================================================
//  패킷 수신 
//====================================================================================================
void OmeStreamObject::RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data)
{
	_last_recv_time = GetCurrentMilliSecond();

	// Header Parsing
	if (!StunParsing(data))
	{
		// DTLS 패킷 처리
		if (!_dtls_transport->OnDataReceived(data, _last_recv_time))
		{
			LOG_WRITE(("ERROR : [%s] RecvHandler - Stun/Dtls Data Fail - stream(%d)", _object_name.c_str(), _stream_key));
		}
	}
}

//====================================================================================================
// Stun 파싱 
//====================================================================================================
bool OmeStreamObject::StunParsing(const std::shared_ptr<std::vector<uint8_t>> &data)
{
	//Stun Message 확인 
	if (data->size() < STUN_HEADER_LENGTH)
	{
		// dtls 추가 확인 가능
		return false;
	}

	// Bind Request/Succes Response only check 
	if (!(data->at(0) == 0x00 && data->at(1) == 0x01) && 
		!(data->at(0) == 0x01 && data->at(1) == 0x01))
	{
		// dtls 추가 확인 가능
		return false;
	}

	// Message Length 
	uint16_t message_length = 0;
	message_length += ((int)data->at(2) << 4);
	message_length += data->at(3);
	if (data->size() < STUN_HEADER_LENGTH + message_length)
	{
		// dtls 추가 확인 가능
		return false;
	}

	// Magic Cookie 확인 #define STUN_MAGIC_COOKIE			(0x2112A442)
	if (data->at(4) != 0x21 && data->at(5) == 0x12 && data->at(6) == 0xA4 && data->at(7) == 0x42)
	{
		// dtls 추가 확인 가능
		return false;
	}

	// transcation_id 복사 
	_offer_transcation_id.clear();
	_offer_transcation_id.insert(_offer_transcation_id.end(), data->begin() + 8, data->begin() + 8 + STUN_TRANSACTION_ID_LENGTH);

	bool result = false;
	if (data->at(0) == 0x01 && data->at(1) == 0x01)		result = RecvBindingSuccessResponse();
	else if(data->at(0) == 0x00 && data->at(1) == 0x01) result = RecvBindingRequest();

	if (!result)
	{
		LOG_WRITE(("ERROR : StunParsing fail - stream(%d)", _object_name.c_str(), _stream_key));
	}

	return result;
}

//====================================================================================================
// Binding Success Response 처리 
//====================================================================================================
bool OmeStreamObject::RecvBindingSuccessResponse()
{
	return true;
}

//====================================================================================================
// Binding Request 처리 
//====================================================================================================
bool OmeStreamObject::RecvBindingRequest()
{	
	bool result = SendBindingResponse();

	if (!_is_completed)
	{
		_is_completed = true;
	
		// dtls 생성 및 handshake
		if (!_dtls_transport->StartDTLS(this))
		{
			LOG_WRITE(("ERROR : DTLS start fail - stream(%d)", _object_name.c_str(), _stream_key));
			return false;
		}
				
		// 콜백 호출 
		if (!((IOmeStreamCallback *)_object_callback)->OnStunComplete_OmeStream(_index_key, _stream_key))
		{
			LOG_WRITE(("ERROR : OnStunComplete_OmeStream fail - stream(%d)", _object_name.c_str(), _stream_key));
			return false;
		}
	}
	return result;
}


//====================================================================================================
// Binding Request 패킷 생성
// - 1초 반복 재전송
//====================================================================================================
bool OmeStreamObject::MakeBindingRequestPacket(std::shared_ptr<std::vector<uint8_t>> &packet)
{
	std::vector<uint8_t> user_name_attriubte;
	std::vector<uint8_t> finger_print_attriubte;
	uint16_t	packet_data_size = 0; 
	
	uint8_t charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	// random 한 transaction id 생성
	_peer_transcation_id.clear();
	for (int index = 0; index < STUN_TRANSACTION_ID_LENGTH; index++)
	{
		_peer_transcation_id.push_back(charset[rand() % sizeof(charset)/sizeof(charset[0])]);
	}

	// Attributes
	//  - type(2byte)
	//  - length(2byte)
	//  - data
	// USERNAME, MESSAGE-INTEGRITY, FINGERPRINT 

	// USERNAME
	std::string user_name = _complete_info.offer_ufrag + ":" + _complete_info.peer_ufrag;
	int padding_count = 4 - (user_name.size() % 4); // 마지막 attribute 4의배수 처리 위해 패딩 추가

	user_name_attriubte.push_back(0x00);
	user_name_attriubte.push_back(0x06);
	user_name_attriubte.push_back(((user_name.size()+ padding_count) >> 8) & 0xff);
	user_name_attriubte.push_back((user_name.size() + padding_count) & 0xff);
	user_name_attriubte.insert(user_name_attriubte.end(), user_name.begin(), user_name.end());
	for (int index = 0; index < padding_count; index++)
	{
		user_name_attriubte.push_back(0x00);
	}

	// Size Setting
	packet_data_size = user_name_attriubte.size() + STUN_FINGERPRINT_ATTRIBUTE_LENGTH;

	// MessageType(2byte)
	// - binding requext 0x00001
	packet->push_back(0x00);
	packet->push_back(0x01);
	
	// Message Length(2byte)
	packet->push_back((packet_data_size >> 8) & 0xff);
	packet->push_back(packet_data_size & 0xff);

	// Magic Cookie(4byte)
	packet->push_back((STUN_MAGIC_COOKIE >> 24) & 0xff);
	packet->push_back((STUN_MAGIC_COOKIE >> 16) & 0xff);
	packet->push_back((STUN_MAGIC_COOKIE >> 8) & 0xff);
	packet->push_back(STUN_MAGIC_COOKIE & 0xff);

	// Transcation ID(12byte)
	packet->insert(packet->end(), _peer_transcation_id.begin(), _peer_transcation_id.end());
	
	// USERNAME Attribute 
	packet->insert(packet->end(), user_name_attriubte.begin(), user_name_attriubte.end());
	
	// FINGERPRINT  Arrrubute (계산)  - fingerprint
	uint32_t crc = MakeCrc(0, packet->data(), packet->size());
	// 계산한 CRC에 xoring
	crc ^= STUN_FINGERPRINT_XOR_VALUE;

	// type
	finger_print_attriubte.push_back(0x80);
	finger_print_attriubte.push_back(0x28);

	// length 
	finger_print_attriubte.push_back(0x00);
	finger_print_attriubte.push_back(0x04);
	
	// crc 
	finger_print_attriubte.push_back((crc >> 24) & 0xff);
	finger_print_attriubte.push_back((crc >> 16) & 0xff);
	finger_print_attriubte.push_back((crc >> 8) & 0xff);
	finger_print_attriubte.push_back(crc & 0xff);
	
	packet->insert(packet->end(), finger_print_attriubte.begin(), finger_print_attriubte.end());

	return true;
}

//====================================================================================================
// Binding Response 패킷 생성
//====================================================================================================
bool OmeStreamObject::MakeBindingResponsePacket(std::shared_ptr<std::vector<uint8_t>> &packet)
{
	std::vector<uint8_t> finger_print_attriubte;
	uint16_t	packet_data_size = 0;
	
	// Attributes
	// FINGERPRINT 
	
	// Size Setting
	packet_data_size = STUN_FINGERPRINT_ATTRIBUTE_LENGTH;

	// MessageType(2byte)
	// - binding requext 0x00001
	packet->push_back(0x01);
	packet->push_back(0x01);

	// Message Length(2byte)
	packet->push_back((packet_data_size >> 8) & 0xff);
	packet->push_back(packet_data_size & 0xff);

	// Magic Cookie(4byte)
	packet->push_back((STUN_MAGIC_COOKIE >> 24) & 0xff);
	packet->push_back((STUN_MAGIC_COOKIE >> 16) & 0xff);
	packet->push_back((STUN_MAGIC_COOKIE >> 8) & 0xff);
	packet->push_back(STUN_MAGIC_COOKIE & 0xff);

	// Transcation ID(12byte)
	packet->insert(packet->end(), _offer_transcation_id.begin(), _offer_transcation_id.end());
	
	// FINGERPRINT  Arrrubute (계산)  - fingerprint
	uint32_t crc = MakeCrc(0, packet->data(), packet->size());
	// 계산한 CRC에 xoring
	crc ^= STUN_FINGERPRINT_XOR_VALUE;

	// type
	finger_print_attriubte.push_back(0x80);
	finger_print_attriubte.push_back(0x28);

	// lenght 
	finger_print_attriubte.push_back(0x00);
	finger_print_attriubte.push_back(0x04);

	// crc 
	finger_print_attriubte.push_back((crc >> 24) & 0xff);
	finger_print_attriubte.push_back((crc >> 16) & 0xff);
	finger_print_attriubte.push_back((crc >> 8) & 0xff);
	finger_print_attriubte.push_back(crc & 0xff);

	packet->insert(packet->end(), finger_print_attriubte.begin(), finger_print_attriubte.end());

	return true;
}


//====================================================================================================
// 최초 video 패킷 timestamp 정보
//====================================================================================================
bool OmeStreamObject::OnDtlsComplete_DtlsTransport()
{
	// 콜백 호출 
	if (!((IOmeStreamCallback *)_object_callback)->OnDtlsComplete_OmeStream(_index_key, _stream_key))
	{
		LOG_WRITE(("ERROR : [%s] OnDtlsComplete_DtlsTransport fail - stream(%d)", _object_name.c_str(), _stream_key));
		return false;
	}

	return true;
}

//====================================================================================================
// DtlsTransport 전송 요청 
// Tls -> DtlsTransport -> OmeStream
//====================================================================================================
bool OmeStreamObject::OnDataSend_DtlsTransport(std::shared_ptr<std::vector<uint8_t>> &packet)
{
	return PostSend(packet);
}

//====================================================================================================
// 최초 video 패킷 timestamp 정보
//====================================================================================================
bool OmeStreamObject::OnRtpStart_DtlsTransport(uint32_t timescal, uint32_t timestamp)
{

	// 콜백 호출 
	if (!((IOmeStreamCallback *)_object_callback)->OnStreamingStart_OmeStream(_index_key, _stream_key))
	{
		LOG_WRITE(("ERROR : [%s] OnStreamingStart_OmeStream fail - stream(%d)", _object_name.c_str(), _stream_key));
		return false;
	}

	return true;
}

//====================================================================================================
// 최초 video 패킷 timestamp 정보
//====================================================================================================
std::shared_ptr<TrfficCollectData> OmeStreamObject::GetTrafficCollectData()
{
	if (!IsStreaming())
	{
		LOG_WRITE(("WARNING : [%s] Stremaing fail - stream(%d)", _object_name.c_str(), _stream_key));
		return nullptr;
	}

	auto collect_data = _dtls_transport->GetTrafficCollectData();

	double  send_bitrate = 0;
	double  recv_bitrate = 0;
	GetTrafficRate(send_bitrate, recv_bitrate);

	if (collect_data != nullptr)
	{
		collect_data->send_bitrate = send_bitrate;
		collect_data->recv_bitrate = recv_bitrate;
	}
	
	return collect_data; 
}