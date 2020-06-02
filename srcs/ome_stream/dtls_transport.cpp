#include "dtls_transport.h"
#include <utility>
#include <algorithm>

#define TRAFFIC_COLLECT_MIN_START_TIME	(3) // s 
#define TRAFFIC_COLLECT_MIN_DURATION	(3000) // ms  

DtlsTransport::DtlsTransport(int stram_key, std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos)
{

	_stream_key = stram_key; 

	for (auto track_info : *track_infos)
	{
		if (track_info.type == StreamTrackType::Video)		_video_track_info = track_info;
		else if (track_info.type == StreamTrackType::Audio) _audio_track_info = track_info;

	}
}

// Start DTLS
bool DtlsTransport::StartDTLS(IDtlsTransportCallback * physical_callback)
{
	TlsCallback callback; 

	
	callback.create_callback = [](Tls *tls, SSL_CTX *context) -> bool
	{
			tls->SetVerify(SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT);

			// SSL_CTX_set_tlsext_use_srtp() returns 1 on error, 0 on success
			if(SSL_CTX_set_tlsext_use_srtp(context, "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32"))
			{
				LOG_WRITE(("ERROR : SSL_CTX_set_tlsext_use_srtp failed"));
				return false;
			}

			return true;
	};
	callback.read_callback = std::bind(&DtlsTransport::Read, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	callback.write_callback = std::bind(&DtlsTransport::Write, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	callback.destroy_callback = nullptr;
	callback.ctrl_callback = [](Tls *tls, int cmd, long num, void *ptr) -> long
	{
		switch(cmd)
		{
			case BIO_CTRL_RESET:
			case BIO_CTRL_WPENDING:
			case BIO_CTRL_PENDING:
				return 0;

			case BIO_CTRL_FLUSH:
				return 1;

			default:
				return 0;
		}
	};
	callback.verify_callback = [](Tls *tls, X509_STORE_CTX *store_context) -> bool
	{
		// LOG_WRITE(("INFO : SSLVerifyCallback enter"));
		return true;
	};
	

	_local_certificate = std::make_shared<Certificate>();
	if (!_local_certificate->Generate())
	{
		LOG_WRITE(("ERROR : Cannot create certificate"));
		return false;
	}


	if(_tls.Initialize(DTLS_client_method(), _local_certificate, nullptr, "DEFAULT:!NULL:!aNULL:!SHA256:!SHA384:!aECDH:!AESGCM+AES256:!aPSK", callback) == false)
	{
		_state = SSL_ERROR;
		return false;
	}

	_state				= SSL_CONNECTING;
	_physical_callback	= physical_callback;
	
	// 한번 connect를 시도해본다.
	ContinueSSL();
	return true;
}

bool DtlsTransport::ContinueSSL()
{
	//LOG_WRITE(("INFO : Continue DTLS..."));

	int error = _tls.Connect();

	if(error == SSL_ERROR_NONE)
	{
		_state = SSL_CONNECTED;

		_peer_certificate = _tls.GetPeerCertificate();

		if(_peer_certificate == nullptr)
		{
			return false;
		}

		if(VerifyPeerCertificate() == false)
		{
			// Peer Certificate 검증 실패
		}

		_peer_certificate->Print();

		return MakeSrtpKey();
	}

	return false;
}

bool DtlsTransport::MakeSrtpKey()
{
	if(_peer_cerificate_verified == false)
	{
		return false;
	}

	// https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#exporter-labels
	const std::string label = "EXTRACTOR-dtls_srtp";

	auto crypto_suite = _tls.GetSelectedSrtpProfileId();

	std::shared_ptr<std::vector<uint8_t>> server_key = std::make_shared<std::vector<uint8_t>>();
	std::shared_ptr<std::vector<uint8_t>> client_key = std::make_shared<std::vector<uint8_t>>();

	_tls.ExportKeyingMaterial(crypto_suite, label, server_key, client_key);

	/* srtp 처리 - 현재는 필요 없어 주석 처리
	_srtp_recv_adapter = std::make_shared<SrtpAdapter>();
	if (_srtp_recv_adapter == nullptr)
	{
		_srtp_recv_adapter.reset();
		_srtp_recv_adapter = nullptr;
		return false;
	}

	if (!_srtp_recv_adapter->SetKey(ssrc_any_inbound, crypto_suite, server_key))
	{
		return false;
	}
	*/

	return true;
}

bool DtlsTransport::VerifyPeerCertificate()
{
	// TODO: PEER CERTIFICATE와 SDP에서 받은 Digest를 비교하여 검증한다.
	// LOG_WRITE(("INFO : Accepted peer certificate"));
	_peer_cerificate_verified = true;
	return true;
}

// 데이터를 upper에서 받는다. lower node로 보낸다.
// Session -> Makes SRTP Packet -> DtlsTransport -> Ice로 전송
bool DtlsTransport::SendData(const std::shared_ptr<std::vector<uint8_t>> &data)
{
	switch(_state)
	{
		case SSL_NONE:
		case SSL_WAIT:
		case SSL_CONNECTING:
			// 연결이 완료되기 전에는 아무것도 하지 않는다.
			break;
		case SSL_CONNECTED:
			/*
			// SRTP는 이미 암호화가 되었으므로 ICE로 바로 전송한다.
			if(from_node == SessionNodeType::Srtp)
			{
				auto node = GetLowerNode(SessionNodeType::Ice);
				if(node == nullptr)
				{
					return false;
				}
				//logtd("DtlsTransport Send next node : %d", data->GetLength());
				return node->SendData(GetNodeType(), data);
			}
			else
			{
				// 그 이외의 패킷은 암호화 하여 전송한다.
				// TODO: 현재는 사용할 일이 없고 향후 데이터 채널을 붙이면 다시 검증한다. (현재는 검증 전)

				size_t written_bytes;

				int ssl_error = _tls.Write(data->GetData(), data->GetLength(), &written_bytes);

				if(ssl_error == SSL_ERROR_NONE)
				{
					return true;
				}

				logte("SSL_wirte error : error(%d)", ssl_error);
			}
			*/
			break;
		case SSL_ERROR:
		case SSL_CLOSED:
			break;
		default:
			break;
	}

	return false;
}

//====================================================================================================
// OnDataReceived 
// - UDP 수신 데이터 전달
// ==> (object) -> DtlsTransport(OnDataReceived) -> (Buffer Save) -> Tls(Read) 
// ==> OpenSSL Read -> Tls(TlsRead) -> DtlsTransport(Read) -> (Buffer Read) 
//====================================================================================================
bool DtlsTransport::OnDataReceived(const std::shared_ptr<std::vector<uint8_t>> &data, int64_t recv_time)
{
	/*
	// Node 시작 전에는 아무것도 하지 않는다.
	if(GetState() != SessionNode::NodeState::Started)
	{
		logtd("SessionNode has not started, so the received data has been canceled.");
		return false;
	}
	*/
	
	switch(_state)
	{
		case SSL_NONE:
		case SSL_WAIT:
			// SSL이 연결되기 전에는 아무것도 하지 않는다.
			break;
		case SSL_CONNECTING:
		case SSL_CONNECTED:
			// DTLS 패킷인지 검사한다.
			if(IsDtlsPacket(data))
			{
				// Packet을 Queue에 쌓는다.
				SaveDtlsPacket(data);

				if (_state == SSL_CONNECTING)
				{
					// 연결중이면 SSL_accept를 해야 한다.
					ContinueSSL();
					return true; 

				}

				// 연결이 완료된 상태면 암호화를 위해 읽어가야 한다.
				char buffer[MAX_DTLS_PACKET_LEN];

				// SSL_read는 다음과 같이 동작한다.
				// SSL -> Read() -> TakeDtlsPacket() -> Decrypt -> buffer
				int ssl_error = _tls.Read(buffer, sizeof(buffer), nullptr);

				// 말이 안되는데 여기 들어오는지 보자.
				int pending = _tls.Pending();

				if(pending >= 0)
				{
					// LOG_WRITE(("INFO : Short DTLS read. Flushing %d bytes - stream(%d)", pending, _stream_key));
					_tls.FlushInput();
				}

				// 읽은 패킷을 누군가에게 줘야 하는데... 줄 놈이 없다.
				// TODO: 향후 SCTP 등을 연결하면 준다. 지금은 DTLS로 암호화 된 패킷을 받을 객체가 없다.
				 LOG_WRITE(("INFO : Unknown dtls packet received - stream(%d) error(%d)",_stream_key,  ssl_error));
				

				return true;
			}
			// SRTP, SRTCP,
			else
			{
				// Rtp 패킷 확인
				if(!IsRtpPacket(data))
				{
					break;
				}

				// strp unprotect - 현재 필요 없음 주석처리  
				// _srtp_recv_adapter->UnprotectRtp(data);

				// Rtp 패킷 파싱
				RtpPacketInfo packet_info; 
				
				if (!RtpPacketParsing(data, packet_info))
				{
					LOG_WRITE(("ERROR : Rtp Packet parsing fail - stream(%d)", _stream_key));
					return false;
				}

				if (packet_info.packet_type == RtpPacketType::Video)
					SetVideoTraffic(packet_info, recv_time);
				else if (packet_info.packet_type == RtpPacketType::Audio)
					SetAudioTraffic(packet_info, recv_time);
				

				//if (packet_info.packet_type == RtpPacketType::Video)
				//	LOG_WRITE(("INFO : Video Rtp - sequence(%d) timestamp(%u)  size(%d)", packet_info.sequence_number, packet_info.timestamp,  packet_info.data_size));

				// 스트림 시작 알림
				if (!_stream_start_notify && _video_stream_started && _audio_stream_started)
				{
					_stream_start_notify = true; 
					_physical_callback->OnRtpStart_DtlsTransport(1000, packet_info.timestamp);
				}

				return true;
			}
			break;
		case SSL_ERROR:
		case SSL_CLOSED:
			break;
		default:
			break;
	}

	return false;
}

//====================================================================================================
// Read
//====================================================================================================
size_t DtlsTransport::Read(Tls *tls, void *buffer, size_t length)
{
	std::shared_ptr<const std::vector<uint8_t>> data = TakeDtlsPacket();

	if(data == nullptr)
	{
		return 0;
	}

	size_t read_len = std::min<size_t>(length, static_cast<size_t>(data->size()));

	::memcpy(buffer, data->data(), read_len);

	return read_len;
}

//====================================================================================================
// Write 
// ==> (OpenSSL) -> Tls(Write) -> DtlsTransport(Write) -> (object OnDataSend_DtlsTransport)
//====================================================================================================
size_t DtlsTransport::Write(Tls *tls, const void *data, size_t length)
{
	auto packet = std::make_shared<std::vector<uint8_t>>((uint8_t *)data,  (uint8_t *)data + length);

	if (_physical_callback == nullptr)
	{
		LOG_WRITE(("ERROR : DtlsTransport - callback nullptr"));
		return -1;
	}
	
	if (!_physical_callback->OnDataSend_DtlsTransport(packet))
	{
		LOG_WRITE(("WARNING : DtlsTransport - OnDataSend_DtlsTransport fail"));
		return 0; 
	}

	// retry
	return length;
}

//====================================================================================================
// IsDtlsPacket
// - dtls 패킷 확인
//====================================================================================================
bool DtlsTransport::IsDtlsPacket(const std::shared_ptr<const std::vector<uint8_t>> data)
{
	//TODO: DTLS처럼 보이는 쓰레기가 있는지 더 엄격하게 검사한다.
	uint8_t *buffer = (uint8_t *)data->data();
	return (data->size() >= DTLS_RECORD_HEADER_LEN && (buffer[0] > 19 && buffer[0] < 64));
}

//====================================================================================================
// IsRtpPacket
// - Rtp 패킷 확인
//====================================================================================================
bool DtlsTransport::IsRtpPacket(const std::shared_ptr<const std::vector<uint8_t>> data)
{
	uint8_t *buffer = (uint8_t *)data->data();
	return (data->size() >= MIN_RTP_PACKET_LEN && (buffer[0] & 0xC0) == 0x80);
}

//====================================================================================================
// SaveDtlsPacket
//====================================================================================================
bool DtlsTransport::SaveDtlsPacket(const std::shared_ptr<const std::vector<uint8_t>> data)
{
	// 이미 하나가 저장되어 있는데 또 저장하려는 것은 잘못된 것이다.
	// 구조상 저장하자마자 사용해야 한다.
	if(_packet_buffer.size() == 1)
	{
		LOG_WRITE(("ERROR : Ssl buffer is full"));
		return false;
	}

	_packet_buffer.push_back(data);

	return true;
}

//====================================================================================================
// TakeDtlsPacket
// - 저장된 데이터를 Openssl에서 읽어감 
//====================================================================================================
std::shared_ptr<const std::vector<uint8_t>> DtlsTransport::TakeDtlsPacket()
{
	if(_packet_buffer.size() <= 0)
	{
		//LOG_WRITE(("INFO : Ssl buffer is empty"));
		return nullptr;
	}

	std::shared_ptr<const std::vector<uint8_t>> data = _packet_buffer.front();
	_packet_buffer.pop_front();
	return data;
}



// Rtp Packet
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V=2|P|X|  CC   |M|     PT      |       sequence number         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           timestamp                           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           synchronization source (SSRC) identifier            |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |            Contributing source (CSRC) identifiers             |
// |                             ....                              |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |One-byte eXtensions id = 0xbede|       length in 32bits        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                          Extensions                           |
// |                             ....                              |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                           Payload                             |
// |             ....              :  padding...                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |               padding         | Padding size  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

//====================================================================================================
// RtpPacketParsing
// - Sequence Number(2byte) - 0~65535(0xffff) 
// - timestap(4byte) - 0~4294967295
// - 길이에 대한 확인은 이미 끝남
//====================================================================================================
bool DtlsTransport::RtpPacketParsing(std::shared_ptr<std::vector<uint8_t>> data, RtpPacketInfo & packet_info)
{
	uint8_t marker; // 0 - continue, 1 - end 
	uint8_t payload_type;
	
	packet_info.data_size = data->size();

	marker			= (uint16_t)data->at(1) >> 7;

	payload_type	= (uint16_t)data->at(1) & 0x7F;
	if (payload_type == _video_track_info.track_id)
		packet_info.packet_type = RtpPacketType::Video;
	else if (payload_type == _audio_track_info.track_id)
		packet_info.packet_type = RtpPacketType::Audio;
	else
		packet_info.packet_type = RtpPacketType::Unknown;

	packet_info.sequence_number = ((uint16_t)data->at(2) << 8) + ((uint16_t)data->at(3));

	packet_info.timestamp		= ((uint16_t)data->at(4) << 24) +
								((uint16_t)data->at(5) << 16) +
								((uint16_t)data->at(6) << 8) +
								((uint16_t)data->at(7));

	// video maker == 1 : end , audio는 무조건 end
	// timestamp -> ms
	if (packet_info.packet_type == RtpPacketType::Video)
	{
		if (_video_track_info.timescale >= 1000)
		{
			packet_info.timestamp /= (_video_track_info.timescale / 1000);
		}

		if (marker == 1) packet_info.is_end_packet = true;
		else packet_info.is_end_packet = false;
	}
	else if (packet_info.packet_type == RtpPacketType::Audio)
	{
		if (_audio_track_info.timescale >= 1000)
		{
			packet_info.timestamp /= (_audio_track_info.timescale / 1000);
		}
		
		packet_info.is_end_packet = true;
	}

	return true;
}


//====================================================================================================
// RtpPacketParsing
// - Sequence Number(2byte) - 0~65535(0xffff) 
// - timestap(4byte) - 0~4294967295
// - 길이에 대한 확인은 이미 끝남
// - latency는 비디오 기준으로만 축정(측정 시작이 비디오 기준이기 때문)
//====================================================================================================
bool DtlsTransport::SetVideoTraffic(RtpPacketInfo & packet_info, int64_t current_time)
{
	

	// video 최초 timestamp 변경 시점
	// - 3초 이후 측정(초기 지연 밀려들어와 latency 부정확 방지)  
	if (!_video_stream_started && 
		_video_last_timestamp != 0 && 
		_video_last_timestamp < packet_info.timestamp && 
		time(nullptr) - _start_time > TRAFFIC_COLLECT_MIN_START_TIME)
	{
		_video_stream_started		= true;

		_video_start_timestamp						= packet_info.timestamp;
		_video_start_time							= current_time;

		_video_collect_start_timestamp				= packet_info.timestamp;
		_video_collect_start_time					= current_time;
		
		_video_last_sequence_number					= packet_info.sequence_number;
		_video_last_timestamp						= packet_info.timestamp;
		_video_last_time							= current_time;
		
		return true;
	}

	// 시작 설정 완료 이후 측정 시작 
	if (!_video_stream_started)
	{
		_video_last_timestamp = packet_info.timestamp;
		_video_last_time = current_time; 

		if (_start_time == 0)
		{
			_start_time = time(nullptr);
		}
		return true;
	}

	
	// 계산 편의를 위해 최대값(0xffff)이후 값 변경 
	int current_sequence_number = packet_info.sequence_number;
	if (_video_last_sequence_number > 0xff00 && current_sequence_number < 0xf000)
	{
		current_sequence_number += 0xffff;

	}

	_video_collect_data.packet_count++;

	// timestamp 변화(프레임 증가)  
	if (_video_last_timestamp < packet_info.timestamp)
	{
		

		// 프레임 증가
		_video_collect_data.frame_count++;
		
		// sequence number 이상 packet loss
		if (current_sequence_number > (_video_last_sequence_number + 1))
			_video_collect_data.packet_loss_count += (current_sequence_number - (_video_last_sequence_number + 1));
		
		_video_last_sequence_number = packet_info.sequence_number;
		_video_last_timestamp		= packet_info.timestamp;
		_video_last_time			= current_time;		

		_video_timestamp_change_timestamp = packet_info.timestamp;
		_video_timestamp_changed_time = current_time;

	}
	// sequence만 증가하거나 loss
	else if (_video_last_timestamp == packet_info.timestamp)
	{
		 

		// sequence number 이상 packet loss
		if (current_sequence_number > (_video_last_sequence_number + 1))
			_video_collect_data.packet_loss_count += (current_sequence_number - (_video_last_sequence_number + 1));
		
		_video_last_sequence_number = packet_info.sequence_number;
		_video_last_timestamp		= packet_info.timestamp;
		_video_last_time			= current_time;

	}
	else // (_video_last_timestamp > packet_info.timestamp)
	{
		// 이미 loss 처리 패킷
	}
	
	return true;
}


//====================================================================================================
// RtpPacketParsing
//====================================================================================================
bool DtlsTransport::SetAudioTraffic(RtpPacketInfo & packet_info, int64_t current_time)
{
	// audio 최초 timestamp 변경 시점 
	if (!_audio_stream_started && 
		_audio_last_timestamp != 0 && 
		_audio_last_timestamp < packet_info.timestamp && 
		time(nullptr) - _start_time > TRAFFIC_COLLECT_MIN_START_TIME)
	{
		_audio_stream_started = true;

		_audio_start_timestamp						= packet_info.timestamp;
		_audio_start_time							= current_time;
		
		_audio_collect_start_timestamp				= packet_info.timestamp;
		_audio_collect_start_time					= current_time;
		
		_audio_last_sequence_number					= packet_info.sequence_number;
		_audio_last_timestamp						= packet_info.timestamp;
		_audio_last_time							= current_time;
	}

	// 시작 설정 완료 이후 측정 시작 
	if (!_audio_stream_started)
	{
		_audio_last_timestamp = packet_info.timestamp;
		_audio_last_time = current_time;

		if (_start_time == 0)
		{
			_start_time = time(nullptr);
		}

		return true;
	}

	// 계산 편의를 위해 최대값(0xffff)이후 값 변경 
	int current_sequence_number = packet_info.sequence_number;
	if (_audio_last_sequence_number > 0xff00 && current_sequence_number < 0xf000)
	{
		current_sequence_number += 0xffff;

	}

	_audio_collect_data.packet_count++;

	// timestamp 변화(프레임 증가)  
	if (_audio_last_timestamp < packet_info.timestamp)
	{
		// 프레임 증가
		_audio_collect_data.frame_count++;

		// sequence number 이상 packet loss
		if (current_sequence_number > (_audio_last_sequence_number + 1))
			_audio_collect_data.packet_loss_count += (current_sequence_number - (_audio_last_sequence_number + 1));
				
		_audio_last_sequence_number = packet_info.sequence_number;
		_audio_last_timestamp		= packet_info.timestamp; 
		_audio_last_time			= current_time;

	}
	else // (_audio_last_timestamp > packet_info.timestamp)
	{
		// 이미 loss 처리 패킷
	}  
	return true;
}

//====================================================================================================
// GetTrafficCollectData
// 데이터 전달 이후 초기화
// 
//====================================================================================================

std::shared_ptr<TrfficCollectData> DtlsTransport::GetTrafficCollectData()
{
	// 스트림 진행 확인
	if (!_video_stream_started || !_audio_stream_started)
	{
		return nullptr; 
	}

	// duration 
	_video_collect_data.duration = _video_timestamp_changed_time - _video_collect_start_time;
	_audio_collect_data.duration = _audio_last_time - _audio_collect_start_time;

	// 최소 수집 시간 확인 
	if (_video_collect_data.duration < TRAFFIC_COLLECT_MIN_DURATION ||
		_audio_collect_data.duration < TRAFFIC_COLLECT_MIN_DURATION)
	{
		return nullptr;
	}

	// latency
	_video_collect_data.latency = (_video_timestamp_changed_time - _video_start_time) - (_video_timestamp_change_timestamp - _video_start_timestamp) ;
	_audio_collect_data.latency = (_audio_last_time - _audio_start_time) - (_audio_last_timestamp - _audio_start_timestamp);

	// 데이터 설정 
	auto data = std::make_shared<TrfficCollectData>(_stream_key, _video_collect_data, _audio_collect_data);

	_video_collect_start_timestamp	= _video_timestamp_change_timestamp;
	_video_collect_start_time		= _video_timestamp_changed_time;

	_audio_collect_start_timestamp	= _audio_last_timestamp;
	_audio_collect_start_time		= _audio_last_time;

	_video_collect_data.Clear();
	_audio_collect_data.Clear();

	return data;
}