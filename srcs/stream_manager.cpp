#include "common_hdr.h"
#include "stream_manager.h"
#include "main_object.h" 

#define READY_STATUS_MAX_WAIT_TIME		(30) 
#define STREAMING_STATUS_MAX_WAIT_TIME	(30) 

//===============================================================================================
// GetStreamErrorString
//===============================================================================================
std::string StreamManager::GetStreamErrorString(StreamError error)
{
	std::string error_string;

	switch (error)
	{
	case StreamError::None:
		error_string = "None";
		break;

	case StreamError::SignallingServer:
		error_string = "SingnallingServer Error";
		break;

	case StreamError::OmeStream:
		error_string = "OmeStream Error";
		break;

	case StreamError::StreamCompleteTimerOver:
		error_string = "Stream Complete Timer Over";
		break;

	case StreamError::SignallingConnectionFail:
		error_string = "Signalling Connection fail";
		break;

	case StreamError::SignallingConnectionClose:
		error_string = "Signalling Connection Close";
		break;

	case StreamError::SignallingJsonDataFail:
		error_string = "Signalling Json Data Fail";
		break;

	default:
		error_string = "Unknown error";
		break;
	}

	return error_string;
}

//===============================================================================================
// GetStreamStatusString
//===============================================================================================
std::string StreamManager::GetStreamStatusString(StreamStatus status)
{
	std::string status_string;

	switch (status)
	{
	case StreamStatus::Ready:
		status_string = "Ready";
		break;
	case StreamStatus::SignallingComplete:
		status_string = "Signalling Complete";
		break;
	case StreamStatus::DtlsComplete:
		status_string = "Dtls Complete";
		break;
	case StreamStatus::StunComplete:
		status_string = "Stun Complete";
		break;
	case StreamStatus::Streaming:
		status_string = "Streaming";
		break;
	default:
		status_string = "Unknown";
		break;
	}

	return status_string;
}

//====================================================================================================
// 생성자 
//====================================================================================================
StreamManager::StreamManager(MainObject * pMainObject)
{
	_main_object = pMainObject;
	_stream_info_map.clear();
}

//====================================================================================================
// 소멸자 
//====================================================================================================
StreamManager::~StreamManager( )
{
	_stream_info_map.clear();

}

//====================================================================================================
// 스트림 생성 
//====================================================================================================
bool StreamManager::CreateStream(int & stream_key, int signalling_server_key)
{
	bool result = false; 
	
	std::lock_guard<std::mutex> stream_info_map_lock(_stream_info_map_mutex);

	auto item = _stream_info_map.find(stream_key);

	// 추가(신규 스트림) 
	if(item == _stream_info_map.end())
	{
		auto stream_info = std::make_shared<StreamInfo>(stream_key, signalling_server_key);

		_stream_info_map[stream_key] = stream_info;
		result 		= true;
	}

	return result; 
}

//====================================================================================================
// 삭제   
//====================================================================================================
bool StreamManager::Remove(int & stream_key) 
{
	int signalling_server_key = -1; 
	int ome_stream_key = -1;

	auto stream_info = FindStream(stream_key, true);

	if (stream_info != nullptr)
	{
		signalling_server_key = stream_info->signalling_server_key;
		ome_stream_key = stream_info->ome_stream_key;

		return true;
	}

	if(signalling_server_key != -1)_main_object->RemoveNetwork((int)NetworkObjectKey::SignallingServer, signalling_server_key);
	if (ome_stream_key != -1)_main_object->RemoveNetwork((int)NetworkObjectKey::OmeStream, ome_stream_key);

	return true;

}

//====================================================================================================
// Find Stream
//====================================================================================================
std::shared_ptr<StreamInfo> StreamManager::FindStream(int & stream_key, bool erase/*= false*/)
{
	std::shared_ptr<StreamInfo> stream = nullptr;

	std::lock_guard<std::mutex> stream_info_map_lock(_stream_info_map_mutex);
	
	auto item = _stream_info_map.find(stream_key);
	
	if( item != _stream_info_map.end() )
	{
		stream = item->second;

		if (erase)
			_stream_info_map.erase(item);
	}

	return stream;
}

//====================================================================================================
// Error code 설정  
//====================================================================================================
bool StreamManager::SetError(int & stream_key, StreamError error)
{
	auto stream_info = FindStream(stream_key);

	if(stream_info != nullptr)
	{
		if(stream_info->error == StreamError::None)
			stream_info->error = error;
		
		return true; 
	}

	return false; 
}

//====================================================================================================
// 상태값 설정  
//====================================================================================================
bool StreamManager::SetStatus(int & stream_key, StreamStatus status)
{
	auto stream_info = FindStream(stream_key);

	if(stream_info != nullptr)
	{
		stream_info->status = status; 
		return true; 
	}

	return false; 
}

//====================================================================================================
// 상태 정보 응답
//====================================================================================================
bool StreamManager::GetStatus(int & stream_key, StreamStatus & status)
{
	auto stream_info = FindStream(stream_key);

	if(stream_info != nullptr)
	{
		status = stream_info->status; 
		
		return true;
	}

	return false;
}

//====================================================================================================
// SignallingServer 정보 설정 
//====================================================================================================
bool StreamManager::SetSignallingServerInfo(int stream_key, int signalling_server_key)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		stream_info->signalling_server_key = signalling_server_key;
		return true;
	}

	return false;
}

//====================================================================================================
// OmeStream 정보 설정 
//====================================================================================================
bool StreamManager::SetOmeStreamInfo(int stream_key, int ome_stram_key, std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		stream_info->ome_stream_key = ome_stram_key;
		stream_info->track_infos = track_infos;
		return true;
	}

	return false;
}

//====================================================================================================
// Garbage Check
// - Error 상태
// - Stream KeepAlive 시간 확인 
//====================================================================================================
int StreamManager::GarbageCheck(std::vector<int> & RemoveStreamKeys) 
{
	time_t	current_time	= time(nullptr); 
	int		create_time_gap	= 0; 
	bool	remove 			= false; 
	int 	remove_count	= 0;
	std::vector<int> signalling_server_keys; 
	std::vector<int> ome_stream_keys;
	
	std::lock_guard<std::mutex> stream_info_map_lock(_stream_info_map_mutex);

	for(auto item = _stream_info_map.begin() ; item != _stream_info_map.end();)
	{
		remove 			= false;
		auto stream_info		= item->second;
		create_time_gap = current_time - stream_info->create_time;
	
		// 스트림 KeepAlive 확인 
		if(stream_info != nullptr)
		{
			// 에러 코드 확인 
			if(stream_info->error != StreamError::None)
			{
				remove = true; 	
			}
			// 최대 생성 대기 시간 확인
			else if(stream_info->status != StreamStatus::Streaming && (create_time_gap > STREAMING_STATUS_MAX_WAIT_TIME))
			{
				stream_info->error = StreamError::StreamCompleteTimerOver;
				remove = true;
			}
							
		}

		//삭제 
		if(remove == true)
		{
			LOG_WRITE(("ERROR : *** stream remove - Stream(%d) Status(%s) Error(%s)***",
				item->first,
				GetStreamStatusString(stream_info->status).c_str(),
				GetStreamErrorString(stream_info->error).c_str()));
			
			signalling_server_keys.push_back(stream_info->signalling_server_key);
			ome_stream_keys.push_back(stream_info->ome_stream_key);

			_stream_info_map.erase(item++);
			remove_count++; 
		}
		else
		{
			item++; 
		}
	}

	if(signalling_server_keys.size() > 0)
		_main_object->RemoveNetwork((int)NetworkObjectKey::SignallingServer, signalling_server_keys);

	if(ome_stream_keys.size() > 0)
		_main_object->RemoveNetwork((int)NetworkObjectKey::OmeStream, ome_stream_keys);

	return remove_count; 
}

//====================================================================================================
// 정보 출력 
//====================================================================================================
int StreamManager::GetStreamCount()
{
	int stream_count = 0; 

	std::lock_guard<std::mutex> stream_info_map_lock(_stream_info_map_mutex);

	stream_count = (int)_stream_info_map.size();
	
	return stream_count;
}
