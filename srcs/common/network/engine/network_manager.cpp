﻿#include "common_hdr.h"
#include "network_manager.h"

//====================================================================================================
// 생성자 
//====================================================================================================
NetworkManager::NetworkManager(int object_key)
{
	_object_key = object_key;
	_object_name = "UnknownObject";
	_service_pool = nullptr;
}


//====================================================================================================
// 소멸자 
//====================================================================================================
NetworkManager::~NetworkManager( )
{
	
}
