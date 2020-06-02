#include "common_hdr.h"
#include "network_service_pool.h" 
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>



//====================================================================================================
// 생성자  
//====================================================================================================
NetworkServicePool::NetworkServicePool(int pool_count)
{
	if(pool_count <= 0) _pool_count = std::thread::hardware_concurrency();
	else _pool_count = pool_count; 
	
	LOG_WRITE(("serverice thread pool count %d", _pool_count ));
		
	_is_run  				= false; 
	_service_index			= 0; 
	_private_service_index	= 0;

	for(int index = 0 ; index < _pool_count ; index++)
	{
		auto service = std::make_shared<NetService>();
		auto work = std::make_shared<boost::asio::io_service::work>(boost::asio::io_service::work(*service));
		
		_network_services.push_back(service);
		_networks.push_back(work);
	}

}

//====================================================================================================
// 소멸자 
//====================================================================================================
NetworkServicePool::~NetworkServicePool()
{
	//Stop
	if(_is_run == true)
	{
		Stop();
	}

}

//====================================================================================================
// Run 
//====================================================================================================

void NetworkServicePool::Run()
{
	if(_is_run == true) 
	{
		return; 
	}
			   	 
	for(int index = 0 ; index < _pool_count ; ++index)
	{
		auto thread = std::make_shared<std::thread>(boost::bind(&NetService::run, _network_services[index]));
		_network_threads.push_back(thread);

#ifdef WIN32
		bool res = SetThreadAffinityMask(thread->native_handle(), 1u << index);
		assert(res);
#else
		
#endif 
	}

	_is_run = true; 
}

//====================================================================================================
// Stop 
//====================================================================================================
void NetworkServicePool::Stop()
{
	if(_is_run == false) 
	{
		return; 
	}

	//서비스 중지 
	for (int index = 0 ; index < _pool_count ; ++index)
	{
		_network_services[index]->stop();
	}
	
	//시비스 중지 완료 
	for (int index = 0 ; index < _pool_count ; ++index)
	{
		_network_threads[index]->join();
	}
	
	_is_run = false; 
}

//====================================================================================================
// IO 서비스 획득 
//====================================================================================================
std::shared_ptr<NetService> NetworkServicePool::GetService()
{
	std::shared_ptr<NetService> service;

	if(_is_run == false) 
	{	
		return std::shared_ptr<NetService>(); 
	}
	
	std::lock_guard<std::mutex> service_index_lock(_service_index_mutex);;
		
	service = _network_services[_service_index++];
	

	if(_service_index >= _pool_count)
	{
		//_service_index = 0; 		
		_service_index = _private_service_index;		

	}
	
	return service;
}

//====================================================================================================
//  전용 IO 서비스 획득( 전용 Thread IO 서비스 사용)  
//  - Service 최대 약 30%를 넘지 않는다 .
//
//====================================================================================================
std::shared_ptr<NetService> NetworkServicePool::GetPrivateService()
{
	int				index = 0; 
	std::shared_ptr<NetService>	net_service;
	if(_is_run == false) 
	{
		return std::shared_ptr<NetService>();  
	}
	
	std::lock_guard<std::mutex> service_index_lock(_service_index_mutex);;

	// 30%이하 Accepter전용 서비스 사용 
	if(_private_service_index < _pool_count/3)
	{
		index = _private_service_index++; 
	}
	// 약 30% 초과 하면 전용 사용을 하지 않음 
	else
	{
		index = _service_index++;

		if(_service_index >= _pool_count)
		{
			_service_index = _private_service_index;		
		}
	}
	
	net_service = _network_services[index];
	
	return net_service;
}
