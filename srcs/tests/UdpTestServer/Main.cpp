#include "UdpServer.h"

//====================================================================================================
// main
//====================================================================================================
int main(int argc, char *argv[])
{
	
	
	int port = UDP_SERVER_PORT;
	int thread_count = boost::thread::hardware_concurrency()/2;
	int timer_interval = TRAFFIC_SEND_INTERVAL; //ms

	if (argc >= 4)
	{
		port = atoi(argv[1]);
		thread_count = atoi(argv[2]);
		timer_interval = atoi(argv[3]);

	}

	printf("start - port(%d) thread(%d) timer(%dms)\n", port, thread_count, timer_interval);
	
	try 
	{
		std::vector<std::shared_ptr<boost::asio::io_service>> io_service_pool;
		std::vector<std::shared_ptr<UdpSender>> udp_senders;
		boost::thread_group group;

		// sender 
		for (int index = 0; index < thread_count; ++index)
		{
			auto io_service = std::make_shared<boost::asio::io_service>();
			io_service_pool.push_back(io_service);
			
			auto sender = std::make_shared<UdpSender>(*io_service, timer_interval);
			udp_senders.push_back(sender);
		
			group.create_thread(bind(&boost::asio::io_service::run, boost::ref(*io_service)));
		}

		// udp server
		auto io_service = std::make_shared<boost::asio::io_service>();
		auto udp_server = std::make_shared<UdpServer>(*io_service, port, &udp_senders);
		group.create_thread(bind(&boost::asio::io_service::run, boost::ref(*io_service)));

		// thread end wait 
		group.join_all();
	}
	catch (std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
	}
}