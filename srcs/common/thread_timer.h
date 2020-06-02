#pragma once 

#include <mutex>

#ifdef WIN32
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 1 
#include <hash_map>
using namespace stdext;
#else
#include <ext/hash_map>
using namespace __gnu_cxx;
#endif


//////////////////////////////////////////////////////////////////////////
class ITimerCallback
{
public:
	virtual void 			OnThreadTimer(uint32_t timer_id, bool &delete_timer) = 0;
};


//////////////////////////////////////////////////////////////////////////
class ThreadTimer
{
public:
	typedef struct _TIMER_INFO // new, delete사용
	{
		uint32_t			elapse;			// 몇ms마다 타이머 이벤트 발생할것인지
		uint32_t			recent_bell;	// 가장 최근에 이벤트 발생한 시간
	} TIMER_INFO, *PTIMER_INFO;

	typedef hash_map<uint32_t, PTIMER_INFO>					TimerHashMap;	// <timer_id, PTIMER_INFO>

public:
							ThreadTimer();
	virtual 				~ThreadTimer( );

public:
	bool					Create(ITimerCallback *callback, uint32_t Resolution=100); // 단위ms
	void					Destroy( );

	bool					SetTimer(uint32_t timer_id, uint32_t Elapse); // 단위는 ms, 이미 있으면 Elapse만 변경
	bool					KillTimer(uint32_t timer_id);

private:
#ifdef WIN32
	static uint32_t __stdcall 	_TimerThread(LPVOID pParameter);
#else
	static void* 			_TimerThread(void* pParameter);
#endif
	uint32_t					_TimerThreadMain( );

private:
	ITimerCallback * _timer_interface;
	uint32_t					m_Resolution;

	TimerHashMap			_timer_map;
	std::recursive_mutex	_timer_map_mutex; // 동일 Thread에서 접속 가능

	bool					m_bStopThread;
	THREAD_HANDLE			m_hThread;
};


