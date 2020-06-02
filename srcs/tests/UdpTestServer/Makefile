.SUFFIXES : .cpp .o

VPATH =	./
				
CXX = g++

INCLUDE =	-I./

BASE_LIBS = -lpthread

DEFINES = -D_THREAD_SAFE

BOOST_INCLUDE = -I/usr/local/include
#BOOST_LIB = -L/usr/local/lib -lboost_system -lboost_thread -lboost_chrono
BOOST_LIB = -lboost_system  -lboost_thread

CFLAGS = -g -O -O2 -Wall -march=x86-64 -std=c++14 $(DEFINES)\
		$(INCLUDE)\
		$(BOOST_INCLUDE)\
		$(OPENSSL_INCLUDE)

OBJS =	UdpSender.o\
		UdpServer.o\
		Main.o\
		 
TARGET	= test_udp_server

all : $(TARGET)

%.o:%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) 

$(TARGET) : $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(BASE_LIBS) $(BOOST_LIB)

clean :
	rm -rf $(OBJS) $(TARGET) core.*
    
install :
	mkdir -p ./bin
	mv ./$(TARGET) ./bin/$(TARGET)

new : 
	$(MAKE) clean 
	$(MAKE)