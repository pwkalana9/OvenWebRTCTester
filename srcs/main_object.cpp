#include "common_hdr.h"
#include "main_object.h"
#include "openssl/ssl.h"
#include <iomanip>

enum
{
	START_TIMER_ID,
	PRINT_INFO_TIMER_ID,
	DATA_COLLECT_TIMER_ID,
	STREAM_CREATE_TIMER_ID,
	GARBAGE_CHECK_TIMER_ID,	
};

#define START_TIMER_ID_INTERVAL			(1)
#define PRINT_INFO_TIMER_INTERVAL		(60)
#define DATA_COLLECT_TIMER_INTERVAL		(10)
#define GARBAGE_CHECK_TIMER_INTERVAL	(20)


#define CSV_LINE_SEPARATOR  (',')
#define CSV_LINE_END		('\n')

//====================================================================================================
// Http Header Date Gmt 시간 문자 
//====================================================================================================
std::string  MainObject::GetCurrentDateTime()
{
	std::ostringstream time_string;
	time_t		time_value = time(nullptr);
	struct tm	time_struct;
	time_struct = *localtime(&time_value);

	time_string << std::setfill('0')
		<< time_struct.tm_year + 1900
		<< std::setw(2) << time_struct.tm_mon + 1
		<< std::setw(2) << time_struct.tm_mday
		<< std::setw(2) << time_struct.tm_hour
		<< std::setw(2) << time_struct.tm_min
		<< std::setw(2) << time_struct.tm_sec;

	return time_string.str();
}

//===============================================================================================
// GetNetworkObjectName
//===============================================================================================
std::string MainObject::GetNetworkObjectName(NetworkObjectKey object_key)
{
	std::string object_key_string;

	switch (object_key)
	{
	case NetworkObjectKey::SignallingServer:
		object_key_string = "signalling_server";
		break;
	case NetworkObjectKey::OmeStream:
		object_key_string = "ome_stream";
		break;
	case NetworkObjectKey::TestUdpServer:
		object_key_string = "test_udp_server";
		break;

	default:
		object_key_string = "unknown";
		break;
	}

	return object_key_string;
}

//====================================================================================================
// 생성자
//====================================================================================================
MainObject::MainObject( ) : 
	_signalling_servers((int)NetworkObjectKey::SignallingServer),
	_ome_streams((int)NetworkObjectKey::OmeStream),
	_test_udp_servers((int)NetworkObjectKey::TestUdpServer)
{
	// Network Table 구성 
	_network_table[(int)NetworkObjectKey::SignallingServer]			= &_signalling_servers;
	_network_table[(int)NetworkObjectKey::OmeStream]				= &_ome_streams;
	_network_table[(int)NetworkObjectKey::TestUdpServer]			= &_test_udp_servers;
		
	_start_time = time(nullptr);


	std::string file_name = "result_" + GetCurrentDateTime() + ".csv";

	_csv_file.open(file_name);
	


}

//====================================================================================================
// 소멸자
//====================================================================================================
MainObject::~MainObject( )
{
	Destroy();	
	_csv_file.close(); 

}

//====================================================================================================
// 종료 
//====================================================================================================
void MainObject::Destroy( )
{
	_thread_timer.Destroy();

	//종료 
	for(int index = 0; index < (int)NetworkObjectKey::Max ; index++)
	{
		_network_table[index]->PostRelease(); 
	}
	
	//서비스 종료
	if(_network_service_pool != nullptr)
	{
		_network_service_pool->Stop(); 

		LOG_WRITE(("INFO : Network Service Pool Stop Completed"));
	}
}

//====================================================================================================
// StreamManager 객체 접근
//====================================================================================================
std::shared_ptr<StreamManager> &MainObject::GetStreamManager(int stream_key)
{
	return _stream_managers.at(stream_key%_stream_managers.size());
}

//====================================================================================================
// 생성
//====================================================================================================
bool MainObject::Create(CreateParam *param)
{  
	// Init OpenSSL
	::SSL_library_init();
	::SSL_load_error_strings();
	::ERR_load_BIO_strings();
	::OpenSSL_add_all_algorithms();

	/*
	int strp_error = srtp_init();
	if (strp_error != srtp_err_status_ok)
	{
		LOG_WRITE((" ERROR - SRTP Could not initialize SRTP (err: %d)", strp_error));
		return 1;
	}
	*/

	if(param == nullptr)
    {
		LOG_WRITE(("ERROR : Create Param Fail"));
		return false;
    }

	// Real Host  설정 
	std::string host_name; 

	if(GetLocalHostName(host_name) == true)
	{
		strcpy(param->real_host_name, host_name.c_str());
	}
	else
	{
		strcpy(param->real_host_name, param->host_name);
	}

	LOG_WRITE(("INFO : Host Name - %s(%s) ", param->host_name, param->real_host_name));
	
	memcpy(&_create_param, param, sizeof(CreateParam));

	// StreamManager 설정 
	for (int index = 0; index < STREAM_MANAGER_OBJECT_COUNT; index++)
	{
		auto stream_mamanger = std::make_shared<StreamManager>(this);
		_stream_managers.push_back(stream_mamanger);
	}

	// IoService 실행 
	_network_service_pool = std::make_shared<NetworkServicePool>(param->thread_pool_count);
	_network_service_pool->Run();  	
	
	if (!_create_param.traffic_test)
	{
		// signalling server생성
		if (!_signalling_servers.Create(this, _network_service_pool, (char *)GetNetworkObjectName(NetworkObjectKey::SignallingServer).c_str()))
		{
			LOG_WRITE(("ERROR : [%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::SignallingServer).c_str()));
			return false;
		}

		// stun server 생성
		if (!_ome_streams.Create(this, _network_service_pool, (char *)GetNetworkObjectName(NetworkObjectKey::OmeStream).c_str()))
		{
			LOG_WRITE(("ERROR : [%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::OmeStream).c_str()));
			return false;
		}
	}
	else
	{
		// test udp server 생성
		if (!_test_udp_servers.Create(this, _network_service_pool, (char *)GetNetworkObjectName(NetworkObjectKey::TestUdpServer).c_str()))
		{
			LOG_WRITE(("ERROR : [%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::TestUdpServer).c_str()));
			return false;
		}
	}

	// Timer 생성 	
	if(	_thread_timer.Create(this) == false || 
		_thread_timer.SetTimer(START_TIMER_ID, START_TIMER_ID_INTERVAL* 1000) == false ||
		_thread_timer.SetTimer(PRINT_INFO_TIMER_ID,PRINT_INFO_TIMER_INTERVAL*1000) == false ||
		_thread_timer.SetTimer(DATA_COLLECT_TIMER_ID, DATA_COLLECT_TIMER_INTERVAL * 1000) == false ||
		_thread_timer.SetTimer(GARBAGE_CHECK_TIMER_ID, 	GARBAGE_CHECK_TIMER_INTERVAL*1000)== false)
	{
		LOG_WRITE(("ERROR : Init Timer Fail"));
	    return false;   
	}
	
	return true;
}

//====================================================================================================
// Network Accepted 콜백
//====================================================================================================
bool MainObject::OnTcpNetworkAccepted(int object_key, NetTcpSocket * & socket, uint32_t ip, int port)
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkAccepted - Unkown ObjectKey - Key(%d)", object_key));
		return false; 
	}
		
	int index_key = -1; 
		
	// if (object_key == xxx)		_xxx.AcceptedAdd(socket, ip, port, this, _create_param.debug_mode, index_key);

	if(index_key == -1)
	{
		LOG_WRITE(("ERRRO : [%s] OnTcpNetworkAccepted - Object Add Fail - IndexKey(%d) IP(%s)", 
					GetNetworkObjectName((NetworkObjectKey)object_key).c_str(), 
					index_key, 
					GetStringIP(ip).c_str()));
		return false; 
	}
	
	return true; 
}

//====================================================================================================
// Network Connected 콜백
//====================================================================================================
bool MainObject::OnTcpNetworkConnected(int object_key, 
									NetworkConnectedResult result, 
									std::shared_ptr<std::vector<uint8_t>> connected_param, 
									NetTcpSocket * socket, 
									unsigned ip, 
									int port)
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkConnected - Unkown ObjectKey - Key(%d)", object_key));
		return false; 
	}
	
	if (object_key == (int)NetworkObjectKey::SignallingServer)
		SignallingServerConnectedProc(result, (SignallingServerConnectedParam*)connected_param->data(), socket, ip, port);
	else
		LOG_WRITE(("INFO : [%s] OnTcpNetworkConnected - Unknown Connect Object -IP(%s) Port(%d)", 
			GetNetworkObjectName((NetworkObjectKey)object_key).c_str(), 
			GetStringIP(ip).c_str(), 
			port));

	return true;
}

//====================================================================================================
// Network ConnectedSSL 콜백
//====================================================================================================
bool MainObject::OnTcpNetworkConnectedSSL(int object_key,
	NetworkConnectedResult result,
	std::shared_ptr<std::vector<uint8_t>> connected_param,
	NetSocketSSL * socket,
	unsigned ip,
	int port)
{
	if (object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkConnectedSSL - Unkown ObjectKey - Key(%d)", object_key));
		return false;
	}

	LOG_WRITE(("INFO : [%s] OnTcpNetworkConnectedSSL - Unknown Connect Object -IP(%s) Port(%d)",
		GetNetworkObjectName((NetworkObjectKey)object_key).c_str(),
		GetStringIP(ip).c_str(),
		port));

	return true;
}

//====================================================================================================
// Network 연결 종료 콜백(연결 종료)  
//====================================================================================================
int MainObject::OnNetworkClose(int object_key, int index_key, uint32_t ip, int port)
{
	int stream_key = -1;
			
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnNetworkClose - Unkown ObjectKey - Key(%d)", object_key));
		return 0; 
	}

	LOG_WRITE(("INFO : [%s] OnNetworkClose - IndexKey(%d) IP(%s) Port(%d)", 
		GetNetworkObjectName((NetworkObjectKey)object_key).c_str(), 
		index_key, 
		GetStringIP(ip).c_str(), 
		port));
	
	// TODO : 정보 처러 완료 상태에서는 스트림 제거 필요 없음 steream_manager 객체에서 상태갑 확인 절차 차후 구현 
	if (object_key == (int)NetworkObjectKey::SignallingServer) 
		_signalling_servers.GetStreamKey(index_key, stream_key);
	
	//삭제 
	_network_table[object_key]->Remove(index_key); 
		
	// 스트림 제거 
	if(stream_key != -1)
	{
		// Error설정(Garbage Check에서 정리)
		GetStreamManager(stream_key)->SetError(stream_key, StreamError::SignallingConnectionClose);
	}
	
	return 0;
}

//====================================================================================================
// 소켓 연결 삭제 
// - 스트림 제거시 호출 
//====================================================================================================
bool MainObject::RemoveNetwork(int object_key, int index_key) 
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : RemoveNetwork - ObjectKey(%d)", object_key));
		return false; 
	}

	//삭제 
	_network_table[object_key]->Remove(index_key); 

	return true; 
	
}

//====================================================================================================
// 소켓 연결 삭제 
// - 스트림 제거시 호출 
// - 인덱스키 배열 
//====================================================================================================
bool MainObject::RemoveNetwork(int object_key, std::vector<int> & IndexKeys) 
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : RemoveNetwork - ObjectKey(%d)", object_key));
		return false; 
	}

	//삭제 
	_network_table[object_key]->Remove(IndexKeys); 

	return true; 
}
	
//====================================================================================================
// SignallingServer 연결 이후 처리 
//====================================================================================================
bool MainObject::SignallingServerConnectedProc(NetworkConnectedResult result_code, 
											SignallingServerConnectedParam *connected_param, 
											NetTcpSocket *socket, 
											uint32_t ip, 
											int port)
{
	int			signalling_server_key	= -1;
	int			stream_key		= connected_param->stream_key;
	std::string remote_host		= connected_param->remote_host;
	std::string app_name = connected_param->app_name;
	std::string stream_name = connected_param->stream_name;


	if (result_code != NetworkConnectedResult::Success || socket == nullptr)
	{
		LOG_WRITE(("ERROR : SignallingServer - Connected Result Fail - stream(%d) signalling_server(%s)", 
					stream_key, 
					remote_host.c_str()));

		return false;
	}

	// SignallingServer 연결 추가( +handshake)
	if (!_signalling_servers.ConnectAdd(socket, this, stream_key, remote_host, app_name, stream_name, signalling_server_key))
	{
		LOG_WRITE(("ERROR : SignallingServer - ConnectAdd Fail - stream(%d) signalling_server(%s)", 
					stream_key, 
					remote_host.c_str()));

		return false;
	}

	// handshake
	if (!_signalling_servers.SendHandshake(signalling_server_key))
	{
		GetStreamManager(stream_key)->SetError(stream_key, StreamError::SignallingServer);
		return false;
	}

	return true;
}

//====================================================================================================
// SignallingServer HandshakeComplete 콜백  
// - OmeStream(Stun) 접속 
//====================================================================================================
bool MainObject::OnJsonDataFail_SignallingServer(int index_key, int stream_key)
{
	// Error설정(Garbage Check에서 정리)
	GetStreamManager(stream_key)->SetError(stream_key, StreamError::SignallingJsonDataFail);

	return true;
}
//====================================================================================================
// SignallingServer HandshakeComplete 콜백  
// - OmeStream(Stun) 접속 
//====================================================================================================
bool MainObject::OnHandshakeComplete_SignallingServer(	int index_key,
														int stream_key,
														SignallingCompleteInfo &complete_info,
														std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos)
{
	int ome_stram_key = -1;
	
	if (!_ome_streams.ConnectAdd(this, 
								complete_info.ome_stram_ip, 
								complete_info.ome_stram_port, 
								stream_key, 
								complete_info,
								track_infos,
								ome_stram_key))
	{
		LOG_WRITE(("ERROR : test server connect add fail"));
		return false;
	}

	// OmeStream 정보 추가  
	if (!GetStreamManager(stream_key)->SetOmeStreamInfo(stream_key, ome_stram_key, track_infos))
	{
		LOG_WRITE(("ERROR : SignallingServer - SetOmeStreamInfo Fail - stream(%d) ome_stream(%s:%d)", 
			stream_key, 
			GetStringIP(complete_info.ome_stram_ip).c_str(), 
			complete_info.ome_stram_port));
		
		return false;
	}


	/*
	LOG_WRITE(("*** signalling handshake complete - stream(%d) ome_stream(%s:%d) ***", 
			stream_key, 
			::GetStringIP(complete_info.ome_stram_ip).c_str(), 
			complete_info.ome_stram_port));
	*/

	// 상태 변경 
	GetStreamManager(stream_key)->SetStatus(stream_key, StreamStatus::SignallingComplete);


	return true;
}

//====================================================================================================
// Dtls Handshake 완료 콜백
// - strun 접속 
//====================================================================================================
bool MainObject::OnDtlsComplete_OmeStream(int index_key, int stream_key)
{
	// LOG_WRITE(("*** dtls complete - stream(%d) ***", stream_key));

	// 상태 변경 
	GetStreamManager(stream_key)->SetStatus(stream_key, StreamStatus::DtlsComplete);

	return true;
}

//====================================================================================================
// Stun 최초 완료 콜백  
//====================================================================================================
bool MainObject::OnStunComplete_OmeStream(int index_key, int stream_key)
{
	// LOG_WRITE(("*** stun complete - stream(%d) ***", stream_key));

	// 상태 변경 
	GetStreamManager(stream_key)->SetStatus(stream_key, StreamStatus::StunComplete);


	return true;
}

//====================================================================================================
// Dtls Handshake 완료 콜백
// - strun 접속 
//====================================================================================================
bool MainObject::OnStreamingStart_OmeStream(int index_key, int stream_key)
{
	// LOG_WRITE(("*** stream create complete - stream(%d) ***", stream_key));

	// 상태 변경 
	GetStreamManager(stream_key)->SetStatus(stream_key, StreamStatus::Streaming);

	return true;
}


//====================================================================================================
// 기본 타이머 Callback
//====================================================================================================
void MainObject::OnThreadTimer(uint32_t timer_id, bool &delete_timer/* = false */)
{
	switch(timer_id)
	{
		case START_TIMER_ID:
			if (!_create_param.traffic_test)
			{
				StartTimeProc();
				delete_timer = true;
			}
			else
			{
				TrafficTestTimeProc();
				delete_timer = true;
			}
			break;
		case PRINT_INFO_TIMER_ID : 
			PrintInfoTimeProc();
			break; 
		case DATA_COLLECT_TIMER_ID :
			if (!_create_param.traffic_test)
			{
				DataCollectTimeProc();
			}
			else
			{
				TrafficTestCheckTimeProc(); 
			}
			break;

		case STREAM_CREATE_TIMER_ID:
			StreamCreateTimeProc(); 
			break;
		case GARBAGE_CHECK_TIMER_ID :
			GarbageCheckTimeProc();
			break;	
	}
}

//====================================================================================================
// Traffic Test
//  - test udp server connect
//  - signalling 접속 정보와 동일
//  - 시작 개수만 영향
//====================================================================================================
void MainObject::TrafficTestTimeProc()
{

	for (int index = 0; index < _create_param.streaming_start_count; index++)
	{
		int test_udp_server_key = -1;

		if (!_test_udp_servers.ConnectAdd(this,
			GetAddressIP(_create_param.signalling_address),
			_create_param.signalling_port,
			_stream_index_key++,
			test_udp_server_key))
		{
			LOG_WRITE(("ERROR : test_udp_server connect add fail"));
			return;
		}
	}

}

//====================================================================================================
// 테스트 정보 출력
//====================================================================================================
void MainObject::TrafficTestCheckTimeProc()
{
	int			session_count = 0;
	uint64_t	network_traffc = 0;
	uint32_t	packet_loss = 0;
	uint32_t    packet_count = 0;
	uint32_t	packet_loss_percent = 0; 


	_test_udp_servers.GetTrafficInfo(session_count, network_traffc, packet_loss, packet_count);

	if (packet_count != 0)packet_loss_percent = packet_loss * 100 /packet_count;

	LOG_WRITE(("*** traffic test : session(%u) traffic(%ukbps) loss(%d%% : %u : %u)", 
		session_count, 
		network_traffc /1000, 
		packet_loss_percent,
		packet_loss, 
		packet_count));

	return;

}

//====================================================================================================
// 시작 테스트 처리 
// - 1회 실행
//====================================================================================================
void MainObject::StartTimeProc()
{
	

	for (int index = 0; index < _create_param.streaming_start_count; index++)
	{
		auto connected_param = std::make_shared<std::vector<uint8_t>>(sizeof(SignallingServerConnectedParam));
		SignallingServerConnectedParam * signalling_server_param = (SignallingServerConnectedParam *)connected_param->data();

		signalling_server_param->stream_key = _stream_index_key++;
		strcpy(signalling_server_param->remote_host,	_create_param.signalling_address);
		strcpy(signalling_server_param->app_name,		_create_param.signalling_app);
		strcpy(signalling_server_param->stream_name,	_create_param.signalling_stream);
		
		// 스트림 추가 
		GetStreamManager(signalling_server_param->stream_key)->CreateStream(signalling_server_param->stream_key, -1);
		
		// 연결
		if(_create_param.signalling_server_type == SignallingServerType::Ome)
		{
			_signalling_servers.PostConnect(GetAddressIP(_create_param.signalling_address), _create_param.signalling_port, connected_param);
		}

	}

	LOG_WRITE(("*** stream create start - count(%d) address(%s:%d/%s/%s) ***", 
				_stream_index_key, 
				_create_param.signalling_address, 
				_create_param.signalling_port, 
				_create_param.signalling_app,
				_create_param.signalling_stream));


	// Timer 설정
	if (_create_param.streaming_create_interval != 0 && _create_param.streaming_create_count != 0)
	{
		_thread_timer.SetTimer(STREAM_CREATE_TIMER_ID, _create_param.streaming_create_interval * 1000);
	}
}
//====================================================================================================
// 스트림 생성
// - config 정보 반영 
//====================================================================================================
void MainObject::StreamCreateTimeProc()
{
	if (_create_param.streaming_max_count != 0 && _stream_index_key >= _create_param.streaming_max_count)
	{
		return;
	}

	for (int index = 0; index < _create_param.streaming_create_count; index++)
	{
		auto connected_param = std::make_shared<std::vector<uint8_t>>(sizeof(SignallingServerConnectedParam));
		SignallingServerConnectedParam * signalling_server_param = (SignallingServerConnectedParam *)connected_param->data();

		signalling_server_param->stream_key = _stream_index_key++;
		strcpy(signalling_server_param->remote_host, _create_param.signalling_address);
		strcpy(signalling_server_param->app_name, _create_param.signalling_app);
		strcpy(signalling_server_param->stream_name, _create_param.signalling_stream);
		
		// 스트림 추가 
		GetStreamManager(signalling_server_param->stream_key)->CreateStream(signalling_server_param->stream_key, -1);

		// 연결
		if (_create_param.signalling_server_type == SignallingServerType::Ome)
		{
			_signalling_servers.PostConnect(GetAddressIP(_create_param.signalling_address), _create_param.signalling_port, connected_param);
		}
		
		if (_create_param.streaming_max_count != 0  && _stream_index_key >= _create_param.streaming_max_count)
		{
			return;
		}
	}

	LOG_WRITE(("*** stream create start - count(%d) address(%s:%d/%s) ***", 
			_stream_index_key, 
			_create_param.signalling_address, 
			_create_param.signalling_port, 
			_create_param.signalling_stream));

}


//====================================================================================================
// 정보 출력  타이머 처리 
//====================================================================================================
void MainObject::PrintInfoTimeProc()
{
	// 연결 정보 출력 
	int	signalling_server_count = _signalling_servers.GetCount();
	int ome_stream_count = _ome_streams.GetCount(); 
	int	stream_count = 0;
	
	// 전체 계산
	for (auto stream_manager : _stream_managers)
	{
		stream_count += stream_manager->GetStreamCount();
	}
			
	LOG_WRITE(( "*** Connected - StreamCount(%d) Signalling(%d) OmeStream(%d)***",
				stream_count,
				signalling_server_count,
				ome_stream_count));
}

//====================================================================================================
// 데이터 수집
// - 10초 단위 정보 수집 
// 
//====================================================================================================
#define VIDEO_LATENCY_MARGEN (35) // +-ms
#define AUDIO_LATENCY_MARGEN (25) //+-ms
void MainObject::DataCollectTimeProc()
{
	time_t	current_time = time(nullptr);
 	std::vector<std::shared_ptr<TrfficCollectData>> traffic_collect_datas;
	
	uint32_t total_network_bitrate = 0;
	uint32_t total_video_framerate = 0;
	uint32_t total_audio_framerate = 0;
	uint32_t total_video_latency_fail = 0;	 
	uint32_t total_audio_latency_fail = 0;	 
	uint32_t total_video_latency = 0;
	uint32_t total_audio_latency = 0; 
	uint32_t total_packet_loss = 0;
	uint32_t total_packet_loss_fail = 0;
	uint32_t total_video_packet_count = 0; 
	uint32_t total_audio_packet_count = 0;
	static uint32_t total_video_packet_count2 = 0; 
	static uint32_t total_audio_packet_count2 = 0;
	uint32_t packet_loss_percent = 0; 
	static uint32_t max_loss = 0;
	static uint32_t min_loss = UINT_MAX;
	static uint32_t total_loss = 0;
	static uint32_t sample_count = 0;

	if (_collect_start_time == 0)
	{
		_collect_start_time = current_time;

		_csv_file << "start : " << GetStringTime2 (current_time) << "(" << current_time << ")" <<CSV_LINE_END;
		_csv_file	<< "time(sec)," 
							<< "stream(count),"
							<< "network bitrate(kbps)," 
							<< "video framerate(fps avg)," 
							<< "audio framerate(fps avg)," 
							<< "video latency fail(count)," 
							<< "audio latency fail(count)," 
							<< "video latency(ms avg)," 
							<< "audio latency(ms avg),"
							<< "packet loss(percent)"
							<< "packet loss(count)" 
							<< CSV_LINE_END;
	}
	
	_ome_streams.GetTrafficCollectData(traffic_collect_datas);

	if (traffic_collect_datas.size() <= 0)
	{
		return;
	}

	for (const auto & traffic_collect_data : traffic_collect_datas)
	{
		if (traffic_collect_data->video_collect_data.duration > 0)
			total_video_framerate += (((double)traffic_collect_data->video_collect_data.frame_count / traffic_collect_data->video_collect_data.duration) * 1000); // fps 
		
		if (traffic_collect_data->audio_collect_data.duration > 0)
			total_audio_framerate += (((double)traffic_collect_data->audio_collect_data.frame_count / traffic_collect_data->audio_collect_data.duration) * 1000); // fps 
		
		total_network_bitrate	+= (traffic_collect_data->send_bitrate + traffic_collect_data->recv_bitrate)/1000;

		if(traffic_collect_data->video_collect_data.packet_loss_count > 0 || traffic_collect_data->audio_collect_data.packet_loss_count > 0)
		{
			total_packet_loss += (traffic_collect_data->video_collect_data.packet_loss_count + traffic_collect_data->audio_collect_data.packet_loss_count);

			total_packet_loss_fail++;
		}
		// LOG_WRITE(("WARNING : video latency - %dms", traffic_collect_data->video_collect_data.latency));
		if (abs(traffic_collect_data->video_collect_data.latency) > VIDEO_LATENCY_MARGEN) // +- 1 frame duration 
		{
			total_video_latency_fail++;
		}
		total_video_latency += abs(traffic_collect_data->video_collect_data.latency);

		// LOG_WRITE(("WARNING : audio latency - %dms", traffic_collect_data->audio_collect_data.latency));
		if (abs(traffic_collect_data->audio_collect_data.latency) > AUDIO_LATENCY_MARGEN) // +- 1 frame duration
		{
			total_audio_latency_fail++;
		}
		total_audio_latency += abs(traffic_collect_data->audio_collect_data.latency);

		// packet count
		total_video_packet_count += traffic_collect_data->video_collect_data.packet_count;
		total_audio_packet_count += traffic_collect_data->audio_collect_data.packet_count;

		total_video_packet_count2 += traffic_collect_data->video_collect_data.packet_count;
		total_audio_packet_count2 += traffic_collect_data->audio_collect_data.packet_count;
	}

	auto result_data = std::make_shared<CollectResultData>();
	result_data->collect_time		= current_time - _collect_start_time;
	result_data->stream_count		= traffic_collect_datas.size();
	result_data->network_bitrate	= total_network_bitrate;
	result_data->video_framerate	= total_video_framerate / result_data->stream_count;
	result_data->audio_framerate	= total_audio_framerate / result_data->stream_count;
	result_data->video_latency_fail	= total_video_latency_fail;
	result_data->audio_latency_fail	= total_audio_latency_fail;
	result_data->video_latency		= total_video_latency / result_data->stream_count;
	result_data->audio_latency		= total_audio_latency / result_data->stream_count;
	result_data->packet_loss		= total_packet_loss;



	packet_loss_percent = (total_video_packet_count + total_audio_packet_count) == 0 ? 0 : 
							(result_data->packet_loss * 100 / (total_video_packet_count + total_audio_packet_count));
	max_loss = std::max(max_loss, packet_loss_percent);
	min_loss = std::min(min_loss, packet_loss_percent);

	total_loss += result_data->packet_loss;
	sample_count++;

	float avg_loss = ((total_loss * 100.0) / (total_video_packet_count2 + total_audio_packet_count2));


	LOG_WRITE(("*** Traffic Collect - Stream(%d:%d) *** \n\t"\
				"- bitrate :     %uKbps \n\t"\
				"- framerate :   v(%ufps) a(%ufps) \n\t"\
				"- latency_fail: v(%u) a(%u)  \n\t"\
				"- latency :     v(%u) a(%u)  \n\t"\
				"- packet_loss : loss(%u/%u:%u%%, max: %u%%, min: %u%%, avg: %.1f%%)  packet(%u/%u) fail(%u)",
				result_data->stream_count, 
				_stream_index_key,
				result_data->network_bitrate,
				result_data->video_framerate,
				result_data->audio_framerate,
				result_data->video_latency_fail,
				result_data->audio_latency_fail,
				result_data->video_latency,
				result_data->audio_latency,
				result_data->packet_loss,
				total_loss,
				packet_loss_percent,
				max_loss, min_loss, avg_loss,
				total_video_packet_count + total_audio_packet_count,
				total_video_packet_count2 + total_audio_packet_count2,
				total_packet_loss_fail));

	// CSV 파일 저정
	_csv_file
	<< result_data->collect_time		<< CSV_LINE_SEPARATOR
	<< result_data->stream_count		<< CSV_LINE_SEPARATOR
	<< result_data->network_bitrate		<< CSV_LINE_SEPARATOR
	<< result_data->video_framerate		<< CSV_LINE_SEPARATOR
	<< result_data->audio_framerate		<< CSV_LINE_SEPARATOR
	<< result_data->video_latency_fail	<< CSV_LINE_SEPARATOR
	<< result_data->audio_latency_fail	<< CSV_LINE_SEPARATOR
	<< result_data->video_latency		<< CSV_LINE_SEPARATOR
	<< result_data->audio_latency		<< CSV_LINE_SEPARATOR
	<< packet_loss_percent				<< CSV_LINE_SEPARATOR
	<< result_data->packet_loss			<< CSV_LINE_END;
	_csv_file.flush(); 
}

//====================================================================================================
// Garbage Check  타이머 처리 
//====================================================================================================
void MainObject::GarbageCheckTimeProc()
{
	int remove_count = 0; 
	std::vector<int> remove_broad_keys; 
	 
	

	// 전체 스트림 계산
	for (auto stream_manager : _stream_managers)
	{
		remove_count += stream_manager->GarbageCheck(remove_broad_keys);
	}

	LOG_WRITE(("INFO - GarbageCheck - rmoeve(%d)", remove_count));

					
}
