#include "common_hdr.h"

#ifndef _WIN32
#include <fcntl.h>
#include <iostream>
#include <string>
#include <syslog.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <signal.h>
#include "main_object.h"

#define _VERSION_ 		"1.0.000"
#define _PROGREAM_NAME_ "stream_tester"

bool gRun = true;

//====================================================================================================
// 시그널 핸들러 
//====================================================================================================
void SignalHandler(int sig)
{
	gRun = false;
	
	// ignore all these signals now and let the connection close
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
}

//====================================================================================================
// 기본 설정 정보 출력 
//====================================================================================================
void SettingPrint( )
{

	LOG_WRITE(("=================================================="));
	LOG_WRITE(("[ %s Config]", 		_PROGREAM_NAME_));
	LOG_WRITE(("	- %s : %s", 	CONFIG_VERSION, 					GET_CONFIG_VALUE(CONFIG_VERSION)));
	LOG_WRITE(("	- %s : %s",		CONFIG_THREAD_POOL_COUNT,			GET_CONFIG_VALUE(CONFIG_THREAD_POOL_COUNT)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_DEBUG_MODE, 					GET_CONFIG_VALUE(CONFIG_DEBUG_MODE)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_SYS_LOG_BACKUP_HOUR, 		GET_CONFIG_VALUE(CONFIG_SYS_LOG_BACKUP_HOUR)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_LOG_FILE_PATH, 				GET_CONFIG_VALUE(CONFIG_LOG_FILE_PATH)));
	LOG_WRITE(("	- %s : %s",	 	CONFIG_KEEPALIVE_TIME,				GET_CONFIG_VALUE(CONFIG_KEEPALIVE_TIME)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_HTTP_CLIENT_KEEPALIVE_TIME,	GET_CONFIG_VALUE(CONFIG_HTTP_CLIENT_KEEPALIVE_TIME)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_HOST_NAME,					GET_CONFIG_VALUE(CONFIG_HOST_NAME)));
	LOG_WRITE(("	- %s : %sKbps", CONFIG_NETWORK_BAND_WIDTH,			GET_CONFIG_VALUE(CONFIG_NETWORK_BAND_WIDTH)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_FILE_SAVE_PATH,				GET_CONFIG_VALUE(CONFIG_FILE_SAVE_PATH)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_ETHERNET,					GET_CONFIG_VALUE(CONFIG_ETHERNET)));
	LOG_WRITE(("--------------- signalling ---------------"));
	LOG_WRITE(("	- %s : %s",		CONFIG_SIGNALLING_TYPE,				GET_CONFIG_VALUE(CONFIG_SIGNALLING_TYPE)));
	LOG_WRITE(("	- %s : %s",		CONFIG_SIGNALLING_ADDRESS,			GET_CONFIG_VALUE(CONFIG_SIGNALLING_ADDRESS)));
	LOG_WRITE(("	- %s : %s",		CONFIG_SIGNALLING_PORT,				GET_CONFIG_VALUE(CONFIG_SIGNALLING_PORT)));
	LOG_WRITE(("	- %s : %s",		CONFIG_SIGNALLING_APP,				GET_CONFIG_VALUE(CONFIG_SIGNALLING_APP)));
	LOG_WRITE(("	- %s : %s",		CONFIG_SIGNALLING_STREAM,			GET_CONFIG_VALUE(CONFIG_SIGNALLING_STREAM)));
	LOG_WRITE(("--------------- streaming ---------------"));
	LOG_WRITE(("	- %s : %s",		CONFIG_STREAMING_START_COUNT,		GET_CONFIG_VALUE(CONFIG_STREAMING_START_COUNT)));
	LOG_WRITE(("	- %s : %s",		CONFIG_STREAMING_CREATE_INTERVAL,	GET_CONFIG_VALUE(CONFIG_STREAMING_CREATE_INTERVAL)));
	LOG_WRITE(("	- %s : %s",		CONFIG_STREAMING_CREATE_COUNT,		GET_CONFIG_VALUE(CONFIG_STREAMING_CREATE_COUNT)));
	LOG_WRITE(("	- %s : %s",		CONFIG_STREAMING_MAX_COUNT,			GET_CONFIG_VALUE(CONFIG_STREAMING_MAX_COUNT)));
	LOG_WRITE(("	- %s : %s",		CONFIG_TRAFFIC_TEST,				GET_CONFIG_VALUE(CONFIG_TRAFFIC_TEST)));
	LOG_WRITE(("=================================================="));
}

//====================================================================================================
// 설정 파일 로드 
//====================================================================================================
bool LoadConfigFile(char * szProgramPath)
{
	// Confit 파일 경로 설정 
	char szConfigFile[300] = {0, };
	
	
#ifndef _WIN32
	char szDelimiter = '/';
#else
	char szDelimiter = '\\';
#endif

	const char* pszAppName = strrchr(szProgramPath, szDelimiter);

	if (nullptr != pszAppName)
	{
		int folderlenth = strlen(szProgramPath) - strlen(pszAppName) + 1;
		strncpy(szConfigFile, szProgramPath, folderlenth);
	}
	strcat(szConfigFile, DEFAULT_CONFIG_FILE);
	

	//Config 파일 로드 
	if (CONFIGPARSER.LoadFile(szConfigFile) == false) 
	{
		return false;
	}

	return true; 
}

//====================================================================================================
// Main
//====================================================================================================
int main(int argc, char* argv[])
{	
	// 설정 파일 로드 
	if (LoadConfigFile(argv[0]) == false) 
	{
		fprintf(stderr, "Config file not found \r\n");
		return -1; 
	}
	
	signal(SIGINT, SignalHandler);
	signal(SIGTERM, SignalHandler);
	
	//리눅스 데몬 처리 및 로그 초기화 
#ifndef _WIN32
    signal(SIGCLD,SIG_IGN);
#endif
 
	//로그 파일 초기화 
	if(LOG_INIT(GET_CONFIG_VALUE(CONFIG_LOG_FILE_PATH), atoi(GET_CONFIG_VALUE(CONFIG_SYS_LOG_BACKUP_HOUR))) == false)
	{	
		fprintf(stderr,"%s Log System Error \n", _PROGREAM_NAME_ );  
		return -1; 
	}
	 
	//네트워크 초기화 
    InitNetwork();

	//기본 설정 정보 출력 
	SettingPrint( );

	//생성 파라메터 설정 
	CreateParam param;
	strcpy(param.version, 				GET_CONFIG_VALUE(CONFIG_VERSION));
	param.thread_pool_count				= atoi(GET_CONFIG_VALUE(CONFIG_THREAD_POOL_COUNT));
	param.debug_mode					= atoi(GET_CONFIG_VALUE(CONFIG_DEBUG_MODE)) == 1 ? true : false;
	param.keepalive_time 				= atoi(GET_CONFIG_VALUE(CONFIG_KEEPALIVE_TIME));
	param.http_client_keepalive_time	= atoi(GET_CONFIG_VALUE(CONFIG_HTTP_CLIENT_KEEPALIVE_TIME));
	strcpy(param.host_name, 			GET_CONFIG_VALUE(CONFIG_HOST_NAME)); 
	param.network_bandwidth				= atoi(GET_CONFIG_VALUE(CONFIG_NETWORK_BAND_WIDTH)); 
	param.local_ip						= inet_addr(GET_CONFIG_VALUE(CONFIG_LOCAL_IP));
	strcpy(param.file_save_path, 		GET_CONFIG_VALUE(CONFIG_FILE_SAVE_PATH));
	strcpy(param.ethernet_name,			GET_CONFIG_VALUE(CONFIG_ETHERNET));
	param.signalling_server_type		= (SignallingServerType)atoi(GET_CONFIG_VALUE(CONFIG_SIGNALLING_TYPE));
	strcpy(param.signalling_address,	GET_CONFIG_VALUE(CONFIG_SIGNALLING_ADDRESS));
	param.signalling_port				= atoi(GET_CONFIG_VALUE(CONFIG_SIGNALLING_PORT));
	strcpy(param.signalling_app,		GET_CONFIG_VALUE(CONFIG_SIGNALLING_APP));
	strcpy(param.signalling_stream,		GET_CONFIG_VALUE(CONFIG_SIGNALLING_STREAM));
	param.streaming_start_count			= atoi(GET_CONFIG_VALUE(CONFIG_STREAMING_START_COUNT));
	param.streaming_create_interval		= atoi(GET_CONFIG_VALUE(CONFIG_STREAMING_CREATE_INTERVAL));
	param.streaming_create_count		= atoi(GET_CONFIG_VALUE(CONFIG_STREAMING_CREATE_COUNT));
	param.streaming_max_count			= atoi(GET_CONFIG_VALUE(CONFIG_STREAMING_MAX_COUNT));
	param.traffic_test					= atoi(GET_CONFIG_VALUE(CONFIG_TRAFFIC_TEST)) == 1 ? true : false;
	
	//MainObject 생성 
	MainObject main_object;

   	if(main_object.Create(&param) == false)
	{
		LOG_WRITE(("[ %s ] MainObject Create Error\r\n", _PROGREAM_NAME_));
		return -1;
	}
 
	std::string time_string; 

	GetStringTime(time_string, 0);
	LOG_WRITE(("===== [ %s Start(%s)] =====", _PROGREAM_NAME_, time_string.c_str()));

	//프로세스 대기 루프 및 종료 처리  
	while(gRun == true)
	{
#ifndef _WIN32
		Sleep(100);
#else
        if(getchar() == 'x') 
		{
			gRun = false;
        }
#endif
	}

	GetStringTime(time_string, 0);
	LOG_WRITE(("===== [ %s End(%s) ] =====", _PROGREAM_NAME_, time_string.c_str()));
	
	CONFIGPARSER.UnloadFile();

    //네트워크 종료 
    ReleaseNetwork();

	return 0;
}



