#pragma once
#include "./common_function.h"
#include <mutex>
#include <memory>
#pragma pack(1)

class MainObject; 

enum class StreamTrackType
{
	Video,
	Audio,
	Application,
	Unknown,
};

enum class StreamCodecType
{
	VP8,
	H264,
	OPUS,
	AAC,
};

struct StreamTrackInfo
{
	StreamTrackType	type;		// video/audio
	uint32_t		track_id;
	StreamCodecType	codec_type;
	uint32_t		timescale;
};

struct SignallingCompleteInfo
{
	uint32_t	ome_stram_ip;
	int			ome_stram_port;
	std::string peer_ufrag;
	std::string offer_ufrag;
	std::string offer_pwd;
	std::string codec_name; 
	uint32_t    timescale; 
};

// 데이터 수집 값(수집 기간)  
struct CollectData
{
	void Clear()
	{
		frame_count			= 0;
		packet_loss_count	= 0;
		latency				= 0;
		packet_count		= 0;
	}
	uint32_t duration = 0; //ms
	int32_t frame_count = 0;
	int32_t packet_loss_count = 0;
	int32_t	latency = 0;
	int32_t packet_count = 0; 
};

struct TrfficCollectData
{
	TrfficCollectData()
	{

	}

	TrfficCollectData(int stream_key_, CollectData &video_collect_data_, CollectData &audio_collect_data_)
	{
		stream_key			= stream_key_;
		video_collect_data	= video_collect_data_;
		audio_collect_data	= audio_collect_data_;
	}

	int			stream_key = -1; 
	double		send_bitrate = 0; //bps
	double		recv_bitrate = 0; //bps 
	CollectData video_collect_data;
	CollectData audio_collect_data; 
 
};

struct CollectResultData
{
	time_t	collect_time = 0;
	uint32_t stream_count = 0;
	uint32_t network_bitrate = 0; // kbps
	uint32_t video_framerate = 0; // fps 
	uint32_t audio_framerate = 0; // fps
	uint32_t video_latency_fail = 0;	// +20ms ~ -20ms  
	uint32_t audio_latency_fail = 0;	// +20ms ~ -20ms  
	uint32_t video_latency = 0;	// ms (절대값 총합)  
	uint32_t audio_latency = 0;	// ms (절대값 총합) 
	uint32_t packet_loss = 0;
};

enum class StreamStatus
{
	Ready = 0,
	SignallingComplete,
	DtlsComplete, 
	StunComplete,
	Streaming, 	
};

enum class StreamError
{
	None = 0, 
	SignallingServer,	
	OmeStream,
	StreamCompleteTimerOver,
	SignallingConnectionFail, 
	SignallingConnectionClose,
	SignallingJsonDataFail,

	Unknown,	
};

; 

//====================================================================================================
// Streaam Info 
//====================================================================================================
struct StreamInfo
{
public :
	StreamInfo(int &stream_key, int signalling_server_key_)
	{
		create_time				= time(nullptr);
		status					= StreamStatus::Ready;
		error					= StreamError::None;
		signalling_server_key = signalling_server_key_;
	}

public :
	time_t			create_time;	 	//스트림 생성 시작 시간 
	StreamStatus 	status;
	StreamError		error;

	int				signalling_server_key = -1;
	int				ome_stream_key = -1;

	std::shared_ptr<std::vector<StreamTrackInfo>> track_infos;

};

#pragma pack()

//====================================================================================================
// StreamManager
//====================================================================================================
class StreamManager 
{
public:
	StreamManager(MainObject * pMainObject);
	virtual ~StreamManager();
	
public :
	bool	CreateStream(int &stream_key, int signalling_server_key);
			
	bool	Remove(int &stream_key);
	bool	SetError(int &stream_key, StreamError error);
	bool	SetStatus(int &stream_key, StreamStatus status);
	bool	GetStatus(int &stream_key, StreamStatus &status);
	
	bool	SetSignallingServerInfo(int stream_key, int signalling_server_key);
	bool	SetOmeStreamInfo(int stream_key, int ome_stram_key, std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos);

	int		GarbageCheck(std::vector<int> &RemoveStreamKeys); 
	int		GetStreamCount();
	
	static std::string GetStreamErrorString(StreamError object_key);
	static std::string GetStreamStatusString(StreamStatus object_key);

private :
	std::shared_ptr<StreamInfo> FindStream(int & stream_key, bool erase = false);

private :	
	std::map<int, std::shared_ptr<StreamInfo>> 	_stream_info_map;
	std::mutex									_stream_info_map_mutex;
	MainObject									*_main_object;

	

}; 

