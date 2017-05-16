#pragma once
#include "net2/server_socket_utils.h"
#include "MotionSensor.h"
class myServer : public firebird::server_socket_utils{
public:
    myServer(int port, boost::shared_ptr<sensor>& s) : server_socket_utils(port), m_sensor(s){}
protected:
    void handle_read_data(message& msg, firebird::socket_session_ptr pSession)
    {
        std::cout << "hello" << std::endl;
    }
private:
    boost::shared_ptr<sensor> m_sensor;
};
