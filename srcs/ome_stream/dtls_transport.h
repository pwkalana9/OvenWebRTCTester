#pragma once
#include "tls.h"
//#include "srtp_adapter.h"
#include "stream_manager.h"

#define DTLS_RECORD_HEADER_LEN                  13
#define MAX_DTLS_PACKET_LEN                     2048
#define MIN_RTP_PACKET_LEN                      12


enum class RtpPacketType
{
	Video,
	Audio,
	Unknown,
};

struct RtpPacketInfo
{
	int				data_size		= 0;
	RtpPacketType	packet_type		= RtpPacketType::Unknown;
	uint16_t		sequence_number = 0;
	uint32_t		timestamp		= 0;
	bool			is_end_packet	= false;
};



//====================================================================================================
// Interface
//====================================================================================================
class IDtlsTransportCallback
{

public:
	virtual bool OnDataSend_DtlsTransport(std::shared_ptr<std::vector<uint8_t>> &packet) = 0;
	virtual bool OnRtpStart_DtlsTransport(uint32_t timescal, uint32_t timestamp) = 0;
	virtual bool OnDtlsComplete_DtlsTransport() = 0;



};

//====================================================================================================
// DtlsTransport
//====================================================================================================
class DtlsTransport  
{
public:
	explicit DtlsTransport(int stream_key, std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos);
	virtual ~DtlsTransport() = default;

	bool StartDTLS(IDtlsTransportCallback * physical_callback);
	bool SendData(const std::shared_ptr<std::vector<uint8_t>> &data);
	bool OnDataReceived(const std::shared_ptr<std::vector<uint8_t>> &data, int64_t recv_time);
	bool RecvPacket(const std::shared_ptr<std::vector<uint8_t>> &data);

	std::shared_ptr<TrfficCollectData> GetTrafficCollectData();

protected:
	size_t Read(Tls *tls, void *buffer, size_t length);
	size_t Write(Tls *tls, const void *data, size_t length);
	bool VerifyPeerCertificate();

private:
	bool ContinueSSL();
	bool IsDtlsPacket(const std::shared_ptr<const std::vector<uint8_t>> data);
	bool IsRtpPacket(const std::shared_ptr<const std::vector<uint8_t>> data);
	bool SaveDtlsPacket(const std::shared_ptr<const std::vector<uint8_t>> data);
	std::shared_ptr<const std::vector<uint8_t>> TakeDtlsPacket();

	bool MakeSrtpKey();

	bool RtpPacketParsing(std::shared_ptr<std::vector<uint8_t>> data, RtpPacketInfo & packet_info);
	bool SetVideoTraffic(RtpPacketInfo & packet_info, int64_t current_time);
	bool SetAudioTraffic(RtpPacketInfo & packet_info, int64_t current_time);

	enum SSLState
	{
		SSL_NONE,
		SSL_WAIT,
		SSL_CONNECTING,
		SSL_CONNECTED,
		SSL_ERROR,
		SSL_CLOSED
	};
	


	int _stream_key = -1;
	time_t _start_time = 0; 

	Tls _tls;
	StreamTrackInfo _video_track_info;
	StreamTrackInfo _audio_track_info; 

	IDtlsTransportCallback * _physical_callback;

	SSLState _state = SSL_NONE;
	bool _peer_cerificate_verified = false;
	
	std::shared_ptr<Certificate> _local_certificate;
	std::shared_ptr<Certificate> _peer_certificate;
	std::deque<std::shared_ptr<const std::vector<uint8_t>>> _packet_buffer;
	
	// std::shared_ptr<SrtpAdapter> _srtp_recv_adapter;
	
	bool		_video_stream_started = false; 
	bool		_audio_stream_started = false;
	bool		_stream_start_notify = false; 

	uint32_t	_video_start_timestamp = 0;
	int64_t		_video_start_time = 0; // ms
	uint32_t	_audio_start_timestamp = 0;
	int64_t		_audio_start_time = 0; // ms


	int64_t		_video_collect_start_timestamp = 0; // ms
	int64_t		_video_collect_start_time = 0; // ms
	uint32_t	_video_last_timestamp = 0;
	int64_t		_video_last_time = 0;
	uint32_t	_video_timestamp_change_timestamp = 0;
	int64_t		_video_timestamp_changed_time = 0;

	int64_t		_audio_collect_start_timestamp = 0; // ms
	int64_t		_audio_collect_start_time = 0; // ms
	uint32_t	_audio_last_timestamp = 0;
	int64_t		_audio_last_time = 0;
	
	int			_video_last_sequence_number = -1;
	int			_audio_last_sequence_number = -1;

	CollectData _video_collect_data; 
	CollectData _audio_collect_data;

};
