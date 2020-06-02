
#pragma once
#include "network_object_header.h"
#include <thread>
#include <boost/thread.hpp>
//====================================================================================================
// NetworkServicePool 
//====================================================================================================
class NetworkServicePool
{
public :
	NetworkServicePool(int pool_count);
	virtual ~NetworkServicePool();

public : 
	void 			Run();
	void 			Stop();
	std::shared_ptr<NetService>	GetService();
	std::shared_ptr<NetService>	GetPrivateService();

	bool			IsRun(){ return _is_run; }
	 
private :
	std::vector<std::shared_ptr<NetService>> _network_services;
	std::vector<std::shared_ptr<boost::asio::io_service::work>> _networks;
	std::vector<std::shared_ptr<std::thread>>	_network_threads;

	int			_pool_count;
	int			_private_service_index; 
	int			_service_index; 
	std::mutex	_service_index_mutex;
	bool		_is_run; 
};
