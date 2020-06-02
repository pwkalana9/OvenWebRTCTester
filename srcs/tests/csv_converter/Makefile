.SUFFIXES : .cpp .o

VPATH =	./
				
CXX = g++

INCLUDE =	-I./

CFLAGS = -g -O -O2 -Wall -march=x86-64 -std=c++14\
		$(INCLUDE)\
		$(OPENSSL_INCLUDE)

OBJS =	csv_converter.o\
		 
TARGET	= csv_converter

all : $(TARGET)

%.o:%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) 

$(TARGET) : $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS)

clean :
	rm -rf $(OBJS) $(TARGET) core.*
    
install :
	mkdir -p ./bin
	mv ./$(TARGET) ./bin/$(TARGET)

new : 
	$(MAKE) clean 
	$(MAKE)