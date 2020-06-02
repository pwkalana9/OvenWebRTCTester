#pragma once
#include "network/engine/tcp_network_manager.h" 
#include "signalling_server_object.h" 

//====================================================================================================
// SignallingServerMap
//====================================================================================================
class SignallingServerMap : public TcpNetworkManager
{
public :
	SignallingServerMap(int object_key);
	virtual ~SignallingServerMap( );
public : 
	bool ConnectAdd(NetTcpSocket *socket,
					ISignallingServerCallback *callback,
					int stream_key,
					std::string &remote_host,
					std::string &app_name,
					std::string &stream_name,
					int &index_key);

	bool SendHandshake(int index_key);
	bool GetStreamKey(int index_key, int &stream_key);
private : 

}; 
