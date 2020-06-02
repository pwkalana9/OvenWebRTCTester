#pragma once
#include "thread_timer.h"
#include "./common/network/engine/network_service_pool.h"
#include "signalling_server/signalling_server_map.h"
#include "ome_stream/ome_stream_map.h"
#include "test_udp_server/test_udp_server_map.h"
#include "stream_manager.h" 
#include <iostream>
#include <fstream>

// 동기화 처리 대기를 줄이기 위해 해당 개수의 정보 저장 객체 생성 
// 전체적인 데이터 처리를 위해서는 모든 객체 정보를 모아야 함 ... 
#define STREAM_MANAGER_OBJECT_COUNT  (100)

//====================================================================================================
// NetEngine Object Key
//====================================================================================================
enum class SignallingServerType : int32_t
{
	Ome = 0,   // Oven Media Engine
	Wowza, // Wowza Media Engin
};


//====================================================================================================
// NetEngine Object Key
//====================================================================================================
enum class NetworkObjectKey : int32_t
{
	SignallingServer,
	WowzaSignallingServer,
	OmeStream,
	TestUdpServer,
	Max
};

//====================================================================================================
// Param
//====================================================================================================
struct CreateParam
{
	char 		version[MAX_PATH];
	bool		debug_mode;
	int 		keepalive_time; 
	int			http_client_keepalive_time; 
	char 		host_name[MAX_PATH];
	char 		real_host_name[MAX_PATH];
	int			network_bandwidth; //Kbps
	uint32_t	local_ip;
	char 		file_save_path[MAX_PATH];
	char		ethernet_name[MAX_PATH];

	char		signalling_address[MAX_PATH];
	int			signalling_port;
	char		signalling_app[MAX_PATH];
	char		signalling_stream[MAX_PATH];

	int			streaming_start_count;
	int			streaming_create_interval;
	int			streaming_create_count;
	int			streaming_max_count;

	int			thread_pool_count;	
	bool		traffic_test;

	SignallingServerType signalling_server_type; 


};

//====================================================================================================
// SignallingServer 연결 Param
//====================================================================================================
struct SignallingServerConnectedParam
{
	int		stream_key;
	char	remote_host[MAX_PATH]; // host
	char	app_name[MAX_PATH]; // host
	char	stream_name[MAX_PATH]; // host
};

//====================================================================================================
// StunServer 연결 Param
//====================================================================================================
struct StunServerConnectedParam
{
	int		stream_key;
};


//====================================================================================================
// MainObject
//====================================================================================================
class MainObject : 	public ITimerCallback,
					public INetworkCallback,
					public ISignallingServerCallback,
					public IOmeStreamCallback,
					public ITestUdpServerCallback
{
public:
	MainObject(void);
	~MainObject(void);

public:
	std::string GetCurrentDateTime();
	bool Create(CreateParam *param);
	void Destroy(void);
	bool RemoveNetwork(int object_key, int index_key); 
	bool RemoveNetwork(int object_key, std::vector<int> & index_keys); 
	
	static std::string GetNetworkObjectName(NetworkObjectKey object_key);
	std::shared_ptr<StreamManager> &GetStreamManager(int stream_key);
	
private:
	
	// INetworkCallback 구현 
	bool OnTcpNetworkAccepted(int object_key, NetTcpSocket * & socket, uint32_t ip, int port);	
	
	bool OnTcpNetworkAcceptedSSL(int object_key, NetSocketSSL * & socket_ssl, uint32_t ip, int port) { return true; }
	
	bool OnTcpNetworkConnected(int object_key,
							NetworkConnectedResult result,
							std::shared_ptr<std::vector<uint8_t>> connected_param,
							NetTcpSocket * socket,
							unsigned ip,
							int port);
	
	bool OnTcpNetworkConnectedSSL(int object_key,
								NetworkConnectedResult result,
								std::shared_ptr<std::vector<uint8_t>> connected_param,
								NetSocketSSL *socket,
								unsigned ip,
								int port);




	int  OnNetworkClose(int object_key, int index_key, uint32_t ip, int port); 	
	
	// INetworkCallback 구현 
	bool OnUdpNetworkConnected(int object_key, 
								NetworkConnectedResult result,
								char * connected_param, 
								NetUdpSocket * socket, 
								unsigned ip, 
								int port) { return true;  }	 
	int OnUdpNetworkClose(int object_key, int index_key, uint32_t ip, int port) { return 0;  }
	
	// ISignallingServer 구현 
	bool OnHandshakeComplete_SignallingServer(int index_key,
											int stream_key,
											SignallingCompleteInfo &complete_info,
											std::shared_ptr<std::vector<StreamTrackInfo>> &track_infos);

	virtual bool OnJsonDataFail_SignallingServer(int index_key, int stream_key);
		
	// IOmeStrema 구현
	bool OnStunComplete_OmeStream(int index_key, int stream_key);
	bool OnDtlsComplete_OmeStream(int index_key, int stream_key); 
	bool OnStreamingStart_OmeStream(int index_key, int stream_key);

	// tcp 연결 처리 
	bool SignallingServerConnectedProc(NetworkConnectedResult result_code, 
										SignallingServerConnectedParam *connected_param, 
										NetTcpSocket *socket, 
										uint32_t ip, 
										int port);
	// Timer 
	void OnThreadTimer(uint32_t timer_id, bool &delete_timer);
	void TrafficTestTimeProc();
	void TrafficTestCheckTimeProc();
	void StartTimeProc();
	void PrintInfoTimeProc(); 
	void DataCollectTimeProc();
	void StreamCreateTimeProc();
	void GarbageCheckTimeProc();
	
private:
	std::vector<std::shared_ptr<StreamManager>> _stream_managers;

	ThreadTimer			_thread_timer;
	CreateParam			_create_param;
	std::shared_ptr<NetworkServicePool> _network_service_pool = nullptr;	
	
	SignallingServerMap _signalling_servers;
	OmeStreamMap		_ome_streams;
	TestUdpServerMap    _test_udp_servers;

	NetworkManager		*_network_table[(int)NetworkObjectKey::Max];
	time_t				_start_time;


	int					_stream_index_key = 0;
	time_t				_collect_start_time = 0;
	std::ofstream		_csv_file;

};