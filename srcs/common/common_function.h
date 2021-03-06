﻿
#pragma once
#include "common_hdr.h"

#include <time.h>
#include <string>
#include <vector> 
#include <map>
#include <queue>
#include <deque>
#include <stdio.h>
#include "linux_definition.h"
#include "syslog.h" 
 
#ifdef WIN32
	#pragma comment (lib, "rpcrt4.lib") // for UuidCreate
#endif 

//------------------------------로그 관련 정의------------------------------ 
// SYSLOG V1
#define LOG_INIT(file_name, nBackupHour)   LogInit(file_name, nBackupHour)
#define LOG_WRITE(pFormat)                  LogPrint pFormat


//------------------------------로그 관련 함수------------------------------ 
// SYSLOG V1
#define			MAX_LOG_TEXT_SIZE	(4096)
extern bool		LogInit(const char * file_name, int nBackupHour); 
extern void		LogPrint(const char *str, ...);


//------------------------------코드 동기화 관련 정의------------------------------ 
#ifdef WIN32
	typedef int socklen_t;
	#define THREAD_HANDLE			HANDLE
	#define snprintf                _snprintf
#else
	#define THREAD_HANDLE			pthread_t
	void 	InitRecursiveMutex(pthread_mutex_t *pMutex);
#endif

extern void 		InitNetwork();
extern void 		ReleaseNetwork();
extern void			Tokenize(const std::string & str, std::vector<std::string> & tokens, const std::string& delimiters);
extern void			Tokenize2(const char * pText, std::vector<std::string>& tokens, char delimiter);
extern std::string 	GetStringIP(uint32_t ip); 
extern void			GetStringTime(std::string & time_string, time_t time_value); 
extern std::string 	GetStringTime2(time_t time_value, bool bDate = true);
extern std::string 	GetStringDate(time_t time_value, const char * pFormat = nullptr); 

extern void 		ReplaceString( std::string & strText, const char * pszBefore, const char * pszAfter );
extern void			StringNCopy(char * pDest, const char * pSource, int pMaxLength); 
extern uint32_t GetAddressIP(const char * pAddress);
extern int 			UrlEncode(const char * pInput, char * pOutput); 
extern void 		UrlDecode(const char * pInput, char * pOutput);
extern int 			GetAudioTimePerFrame(int nSamplesPerSecond, int nTimeScale, int nSamplesPerFrame);
extern bool 		GetUUID(std::string & strUUID);
extern bool			IsEmptyString(char * pString);
extern int			GetCpuCount();
extern void			MakeDirectory(const char * pPath); 
extern int 			RoundAdjust(double nNumber);
extern bool			FileExist(const char * pFilePath);
extern bool			RtmpUrlParsing(const char * pAddress, std::string & strServer, std::string & strApp, std::string & stream_name, int & port);
extern void			SleepWait(int nMillisecond); 
extern uint32_t 		get_tick_count();
extern uint64_t 	get_tick_count64();
extern int			GetAudioSampleIndex(int samplerate); 
extern bool 		AdtsDataSizeParsing(uint8_t * data, int data_size, int nAdtsHeaderSize, std::vector<int> & FrameSizeInfos);
extern bool 		AdtsHeaderParsing(uint8_t * data, int data_size, int & nAdtsHeaderSize, int & sample_index, int & nChannel);
extern bool         AdtsHeaderParsing(uint8_t * data, int data_size, int & nAdtsHeaderSize, int & sample_index, int & nChannel, int & nFrameCnt, int & nSamplingFrequency);
extern bool			AvcNalHeaderPatternCheck(char * data, int data_size, int & patten_index, int & patten_size);
extern bool			AvcNalHeaderSizeCheck(bool key_frame, char * data, int data_size, int & header_size, int & sps_size, int & pps_size);
extern bool 		AvcHeaderParsing(char * data, int data_size, char * pSps, int & sps_size, char * pPps, int & pps_size);
extern int			GetLocalAddress(char * pAddress);
extern bool			GetLocalHostName(std::string & host_name);

extern std::string	RandomString(uint32_t size);
extern std::string	RandomNumberString(uint32_t size);