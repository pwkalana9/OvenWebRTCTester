#include "common_hdr.h"
#include "signalling_server_map.h"

//====================================================================================================
// 생성자 
//====================================================================================================
SignallingServerMap::SignallingServerMap(int object_key) : TcpNetworkManager(object_key)
{
	
}

//====================================================================================================
// 소멸자 
//====================================================================================================
SignallingServerMap::~SignallingServerMap()
{

}

//====================================================================================================
// Object 추가(연결) 
//====================================================================================================
bool SignallingServerMap::ConnectAdd(NetTcpSocket *socket, 
									ISignallingServerCallback *callback, 
									int stream_key,
									std::string &remote_host,
									std::string &app_name,
									std::string &stream_name,
									int &index_key)
{
	std::shared_ptr<TcpNetworkObject>		object;
	TcpNetworkObjectParam 	object_param;

	//Object Param 설정 	
	object_param.object_key = _object_key;
	object_param.socket = socket;
	object_param.network_callback = _network_callback;
	object_param.object_callback = callback;
	object_param.object_name = _object_name;

	//TcpNetworkObject 소멸자에서	삭제 처리 하므로 nullptr로 설정 
	socket = nullptr;

	// Object 생성(스마트 포인터) 
	object = std::shared_ptr<TcpNetworkObject>(new SignallingServerObject());
	if (!std::static_pointer_cast<SignallingServerObject>(object)->Create(stream_key, remote_host, app_name, stream_name, &object_param))
	{
		index_key = -1;
		return false;
	}

	// Object 정보 추가 
	index_key = Insert(object);

	return index_key == -1 ? false : true;
}


//====================================================================================================
// handshake 요청  
//====================================================================================================
bool SignallingServerMap::SendHandshake(int index_key)
{
	bool result = false;
	
	auto object = Find(index_key);

	if (object != nullptr)
	{
		result = std::static_pointer_cast<SignallingServerObject>(object)->SendHandshake();
	}


	return result;
}

//====================================================================================================
// stream key 정보 
//====================================================================================================
bool SignallingServerMap::GetStreamKey(int index_key, int &stream_key)
{
	bool result = false;

	auto object = Find(index_key);

	if (object != nullptr)
	{
		stream_key = std::static_pointer_cast<SignallingServerObject>(object)->GetStreamKey();
		result = true;
	}

	return result;
}