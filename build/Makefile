.SUFFIXES : .cpp .o



VPATH =	../srcs/common:\
		../srcs/common/network/protocol_object:\
		../srcs/common/network/engine:\
		../srcs/jsoncpp:\
		../srcs/ome_stream:\
		../srcs/signalling_server:\
		../srcs/test_udp_server:\
		../srcs/
				
CXX = g++

INCLUDE =	-I../srcs/common\
			-I../srcs/common/network/protocol_object\
			-I../srcs/common/network/engine\
			-I../srcs/jsoncpp\
			-I../srcs/ome_stream\
			-I../srcs/signalling_server\
			-I../srcs/test_udp_server\
			-I../srcs/

BASE_LIBS = -lpthread

DEFINES = -D_THREAD_SAFE

BOOST_INCLUDE = -I/usr/local/include
BOOST_LIB = -lboost_system  -lboost_thread

OPENSSL_INCLUDE = -I/usr/local/include
OPENSSL_LIB = -lssl -lcrypto

CFLAGS = -g -O -Wall -march=x86-64 -std=c++14 -Wno-deprecated -Wunused-function -Waddress -Wunused-value $(DEFINES)\
		$(INCLUDE)\
		$(BOOST_INCLUDE)\
		$(OPENSSL_INCLUDE)

OBJS =	syslog.o\
		common_function.o\
		config_parser.o\
		thread_timer.o\
		network_service_pool.o\
		network_manager.o\
		tcp_network_manager.o\
		udp_network_manager.o\
		tcp_network_object.o\
		udp_network_object.o\
		http_request_object.o\
		http_response_object.o\
		web_socket_request_object.o\
		jsoncpp.o\
		tls.o\
		certificate.o\
		dtls_transport.o\
		ome_stream_map.o\
		ome_stream_object.o\
		signalling_server_map.o\
		signalling_server_object.o\
		test_udp_server_map.o\
		test_udp_server_object.o\
		stream_manager.o\
		main_object.o\
		stream_tester.o\
	 
TARGET	= OvenWebRTCTester

all : $(TARGET)

%.o:%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) 

$(TARGET) : $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(BASE_LIBS) $(BOOST_LIB) $(OPENSSL_LIB)

clean :
	rm -rf $(OBJS) $(TARGET) core.*

install :
	mkdir -p ../bin ../bin/log && touch ../bin/OvenWebRTCTester.conf
	mv ./$(TARGET) ../bin/$(TARGET)

new : 
	$(MAKE) clean 
	$(MAKE)
