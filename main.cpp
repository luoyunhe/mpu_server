#define __MAIN__
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "MotionSensor.h"
#include "myServer.h"
#include "common.h"
#include "dataStore.h"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#define delay_ms(a) usleep(a*1000)
void update(void);
log4cxx::LoggerPtr firebird_log;
std::string KDS_CODE_INFO("XXX");

void net_thread_run(boost::shared_ptr<sensor> s)
{
    log4cxx::PropertyConfigurator::configureAndWatch("log4cxx2.properties");
    firebird_log = log4cxx::Logger::getLogger("test");
    myServer server(12345, s);
    server.start();
    server.get_io_service().run();
}
int main()
{
    boost::shared_ptr<sensor> s = boost::make_shared<sensor>();
    boost::thread net_thread(boost::bind(&net_thread_run, s));
    dataStore ds(s);
    boost::thread file_io_thread(boost::bind(&dataStore::run, &ds));
	do{
        s->Notify();
        //std::cout << "Notify" << std::endl;
		delay_ms(5);
	}while(1);
    net_thread.join();
    file_io_thread.join();
}
/*
int main() {
	ms_open();
	do{
		ms_update();
		printf("yaw = %2.1f\tpitch = %2.1f\troll = %2.1f\ttemperature = %2.1f\tcompass = %2.1f, %2.1f, %2.1f\n",
		 ypr[YAW], ypr[PITCH],
		 ypr[ROLL],temp,compass[0],compass[1],compass[2]);
		delay_ms(5);
	}while(1);

	return 0;
}*/
