#pragma once
#include "net2/server_socket_utils.h"
#include "dataStore.h"
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include "MotionSensor.h"
#include <boost/filesystem.hpp>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include "observerModel.h"
#include "sensorData.h"
typedef std::pair<std::string, int> PAIR;
const std::string PATH("/home/pi/mpu/.store");
enum event{
    regRealTime = 3,
    unregRealTime = 4,
    reflesh = 5,
    history = 6
};
class realTimeObserver : public Observer<sd_ptr>
{
public:
    realTimeObserver(boost::shared_ptr<sensor>& s, std::string address) : p_sensor(s)
    {
        p_socket.reset(new boost::asio::ip::udp::socket(m_io_service));
        p_end_point.reset(new boost::asio::ip::udp::endpoint(\
                    boost::asio::ip::address::from_string(address), 5555));
        p_socket->open(p_end_point->protocol());
    }
    ~realTimeObserver()
    {
        unreg();
        std::string msg("bye");
        try
        {
            p_socket->send_to(boost::asio::buffer(msg.c_str(), msg.size()), *p_end_point);
        }
        catch (boost::system::system_error &e)
        {

        }
    }
    void reg()
    {
        //std::cout << "reg" << std::endl;
        p_sensor->Attach((Observer<sd_ptr>*)this);
        isReg = true;
    }
    void unreg()
    {
        if(isReg)
        {
            p_sensor->Detach((Observer<sd_ptr>*)this);
            isReg = false;
        //std::cout << "unreg" << std::endl;
        }
    }
    void Update(sd_ptr& data)
    {
        std::string msg;
        msg.resize(36);
        memcpy(&msg[0], data->accel, 12);
        memcpy(&msg[12], data->gyro, 12);
        memcpy(&msg[24], data->euler, 12);
        /*
        for(int i = 0; i < 3; ++i)
            std::cout << data->accel[i] << '\t';
        std::cout << std::endl;
        for(int i = 0; i < 3; ++i)
            std::cout << data->gyro[i] << '\t';
        std::cout << std::endl;
        */
        try
        {
            p_socket->send_to(boost::asio::buffer(msg.c_str(), msg.size()), *p_end_point);
        }
        catch (boost::system::system_error &e)
        {

        }

    }
private:
    boost::asio::io_service m_io_service;
    boost::shared_ptr<boost::asio::ip::udp::socket> p_socket;
    boost::shared_ptr<boost::asio::ip::udp::endpoint> p_end_point;
    bool isReg = false;
    boost::shared_ptr<sensor> p_sensor;
};
class myServer : public firebird::server_socket_utils{
public:
    myServer(int port, boost::shared_ptr<sensor>& s) : server_socket_utils(port), m_sensor(s){}
protected:
    void handle_close(DWORD id)
    {
        boost::mutex::scoped_lock(m_mutex);
        std::map<DWORD, boost::shared_ptr<realTimeObserver>>::iterator iter = m_map.find(id);
        if(iter != m_map.end())
            m_map.erase(iter);
        std::map<DWORD, std::pair<std::string, data>>::iterator it = m_data_map.find(id);
        if(it != m_data_map.end())
            m_data_map.erase(it);

    }
    void handle_read_data(message& msg, firebird::socket_session_ptr pSession)
    {
        switch(msg.command)
        {
            case regRealTime:
                dealRegRealTime(pSession);
                break;

            case unregRealTime:
                dealUnregRealTime(pSession);
                break;
            case reflesh:
                dealReflesh(pSession);
                break;
            case history:
                dealHistory(msg, pSession);
                break;
            default:
                std::cout << "hello" << std::endl;
                break;
        }
    }
private:
    void dealHistory(message& msg, firebird::socket_session_ptr p)
    {
        using namespace boost::filesystem;
        DWORD id = p->id();
        unsigned int percent = 0;
        std::string data_path;
        std::string tmp(msg.data());
        data_path.resize(tmp.size() - 4);
        memcpy(&percent, &tmp[0], 4);
        memcpy(&data_path[0], &tmp[4], tmp.size() - 4);
        std::map<DWORD, std::pair<std::string, data>>::iterator it = m_data_map.find(id);
        if((it != m_data_map.end() && it->second.first != data_path) || it == m_data_map.end())
        {
            if(exists(path(data_path)))
            {
                std::ifstream in;
                data D;
                in.open(data_path, std::ios::in | std::ios::binary);
                {
                    boost::archive::binary_iarchive ia(in);
                    ia >> D;
                }
                in.close();
                m_data_map[id] = std::make_pair(data_path, D);
            }
            else
            {
                p->async_write("error!");
                return;
            }
        }
        percent = percent % 100;
        int size = m_data_map[id].second.getDataPtr()->size();
        unsigned int share = size/100;
        boost::shared_ptr<std::vector<sd_ptr>> p_data_vector = m_data_map[id].second.getDataPtr();
        std::string response;
        response.resize(4 + 36 * share);
        memcpy(&response[0], &share, 4);
        unsigned int j = 0;
        for(unsigned int i = share*percent; i < share*(percent+1); ++i, ++j)
        {
            for(unsigned short x = 0; x < 3; ++x)
                memcpy(&response[4+36*j+4*x], &(p_data_vector->operator[](i)->accel[x]), 4);
            for(unsigned short x = 0; x < 3; ++x)
                memcpy(&response[16+36*j+4*x], &(p_data_vector->operator[](i)->gyro[x]), 4);
            for(unsigned short x = 0; x < 3; ++x)
                memcpy(&response[28+36*j+4*x], &(p_data_vector->operator[](i)->euler[x]), 4);
        }
        p->async_write(response);

    }
    /*void dealRealTimeContinue(firebird::socket_session_ptr p)
    {
        DWORD id = p->id();
        std::list<sd_ptr> list;
        m_map[id]->swapList(list);
        if(list.size() > 0)
        {
            DWORD packetSize = list.size();
            std::string msg;
            msg.resize(sizeof(DWORD) + packetSize * 36);
            memcpy(&msg[0], &packetSize, sizeof(DWORD));
            for(unsigned long i = 0; i < packetSize; ++i)
            {
                sd_ptr data = list.front();
                list.pop_front();
                memcpy(&msg[4+i*36], data->accel, 12);
                memcpy(&msg[4+12+i*36], data->gyro, 12);
                memcpy(&msg[4+24+i*36], data->euler, 12);
            }
            p->async_write(msg);
            std::cout << msg.size() << std::endl;
        }
        else
        {
            p->async_write("ND");
            std::cout << "no data"<< std::endl;
        }
    }
    */
    void dealUnregRealTime(firebird::socket_session_ptr p)
    {
        DWORD id = p->id();
        std::map<DWORD, boost::shared_ptr<realTimeObserver>>::iterator iter = m_map.find(id);
        if(iter != m_map.end())
            m_map.erase(id);
        std::string msg("OK");
        p->async_write(msg);
        //std::cout << msg.size() << std::endl;
    }
    void dealRegRealTime(firebird::socket_session_ptr p)
    {
        DWORD id = p->id();
        std::string address(p->socket().remote_endpoint().address().to_string());
        boost::shared_ptr<realTimeObserver> tmp(new realTimeObserver(m_sensor, address));
        tmp->reg();
        {
            boost::mutex::scoped_lock(m_mutex);
            m_map[id] = tmp;
        }
        std::string msg("OK");
        p->async_write(msg);
        //std::cout << msg.size() << std::endl;
    }
    void dealReflesh(firebird::socket_session_ptr p)
    {
        using namespace boost::filesystem;
        path workPath(PATH);
        std::string msg;
        directory_iterator end;
        std::vector<PAIR> vectorPair;
        int a = 0;
        for(directory_iterator pos(workPath); pos != end; ++pos)
        {
            path tmp(*pos);
            std::stringstream stream;
            stream << std::string(tmp.filename().string());
            stream >> a;
            vectorPair.push_back(std::make_pair(tmp.string(), a));
        }
        std::sort(vectorPair.begin(), vectorPair.end(), \
                [](const PAIR x, const PAIR y) {return x.second < y.second;});
        std::vector<PAIR>::iterator iter = vectorPair.begin();
        for(;iter != vectorPair.end(); ++iter)
        {
            //std::cout << (*iter).first << std::endl;
            msg = msg + (*iter).first + '\n';
            std::vector<std::string> v;
            for(directory_iterator pos(path((*iter).first)); pos != end; ++pos)
            {
                v.push_back(path(*pos).string());
            }
            std::sort(v.begin(), v.end());
            for(std::vector<std::string>::iterator iter = v.begin(); iter != v.end(); ++iter)
            {
                msg = msg + ' ' + path(*iter).filename().string() + '\n';
            }
        }
        p->async_write(msg);
        //std::cout << msg.size() << std::endl;
    }
    boost::shared_ptr<sensor> m_sensor;
    std::map<DWORD, boost::shared_ptr<realTimeObserver>> m_map;
    std::map<DWORD, std::pair<std::string, data>> m_data_map;
    boost::mutex m_mutex;
};
