#pragma once
#include "net2/server_socket_utils.h"
#include <boost/thread/mutex.hpp>
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
    realTime = 3,
    realTimeContinue = 4,
    reflesh = 5,
    history = 6
};
class realTimeObserver : public Observer<sd_ptr>
{
public:
    realTimeObserver(boost::shared_ptr<sensor>& s) : p_sensor(s) { }
    ~realTimeObserver()
    {
        unreg();
    }
    void reg()
    {
        std::cout << "reg" << std::endl;
        p_sensor->Attach((Observer<sd_ptr>*)this);
        isReg = true;
    }
    void unreg()
    {
        if(isReg)
        {
            p_sensor->Detach((Observer<sd_ptr>*)this);
            isReg = false;
        std::cout << "unreg" << std::endl;
        }
    }
    void Update(sd_ptr& p)
    {
        boost::mutex::scoped_lock(m_mutex);
        if(m_list.size() < 2000)
            m_list.push_back(p);
    }
    void swapList(std::list<sd_ptr>& l)
    {
        boost::mutex::scoped_lock(m_mutex);
        l.swap(m_list);
    }
private:
    bool isReg = false;
    boost::mutex m_mutex;
    std::list<sd_ptr> m_list;
    boost::shared_ptr<sensor> p_sensor;
};
class myServer : public firebird::server_socket_utils{
public:
    myServer(int port, boost::shared_ptr<sensor>& s) : server_socket_utils(port), m_sensor(s){}
protected:
    void handle_close(DWORD id)
    {
        boost::mutex::scoped_lock(m_mutex);
        m_map.erase(id);

    }
    void handle_read_data(message& msg, firebird::socket_session_ptr pSession)
    {
        switch(msg.command)
        {
            case realTime:
                dealRealTime(pSession);
                break;

            case realTimeContinue:
                dealRealTimeContinue(pSession);
                break;
            case reflesh:
                dealReflesh(pSession);
                break;
            case history:

                break;
            default:
                std::cout << "hello" << std::endl;
                break;
        }
    }
private:
    void dealRealTimeContinue(firebird::socket_session_ptr p)
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
    void dealRealTime(firebird::socket_session_ptr p)
    {
        DWORD id = p->id();
        boost::shared_ptr<realTimeObserver> tmp(new realTimeObserver(m_sensor));
        tmp->reg();
        {
            boost::mutex::scoped_lock(m_mutex);
            m_map[id] = tmp;
        }
        std::string msg("OK");
        p->async_write(msg);
        std::cout << msg.size() << std::endl;
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
        std::cout << msg.size() << std::endl;
    }
    boost::shared_ptr<sensor> m_sensor;
    std::map<DWORD, boost::shared_ptr<realTimeObserver>> m_map;
    boost::mutex m_mutex;
};
