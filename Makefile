CXX=g++
CXXFLAGS= -Wall -g -std=c++11 -I./net2 -I. -I/usr/local/log4cxx/include 
CXX_OPTS= -c
LDFLAGS= -L/usr/local/apr/lib -L/usr/local/log4cxx/lib -llog4cxx -lapr-1 -laprutil-1 \
		 -lpthread -lboost_system -lboost_thread -lboost_serialization -lboost_filesystem\
		 -lboost_date_time

INSTALL=install

PROG=program

VPATH=net2
NET=socket_session.o server_socket_utils.o session_manager.o

%.o: %.c                                                                         
	$(CXX) $(CXXFLAGS) $(CXX_OPTS) $< -o $@ 

%.o: %.cpp                                                                         
	$(CXX) $(CXXFLAGS) $(CXX_OPTS) $< -o $@ 

all: $(PROG).o $(NET)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(PROG) \
		$(NET)\
		main.cpp \
		MotionSensor/libMotionSensor.a \
		libs/libI2Cdev.a

$(PROG).o: MotionSensor/libMotionSensor.a libs/libI2Cdev.a

MotionSensor/libMotionSensor.a:
	$(MAKE) -C MotionSensor/ 

libs/libI2Cdev.a:
	$(MAKE) -C libs/I2Cdev

install1:
	$(INSTALL) -m 755 $(PROG) $(DESTDIR)/usr/local/bin/

clean:
	cd MotionSensor && $(MAKE) clean
	cd libs/I2Cdev && $(MAKE) clean
	rm -rf *.o *~ *.mod
	rm -rf $(PROG)
