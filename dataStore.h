#pragma once
#include "sensorData.h"
#include "observerModel.h"
#include <vector>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/make_shared.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <fstream>
#include "MotionSensor.h"
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#define BOOST_DATE_TIME_SOURCE
const int VECTOR_LEN = 10000;
const std::string store_path("/home/pi/mpu/.store");
void formatTimeStr(std::string& str)
{
    int pos = str.find("T");
    str.replace(pos, 1, std::string("-"));
    str.replace(pos + 3, 0, std::string(":"));
    str.replace(pos + 6, 0, std::string(":"));
    return;
}
class data
{
public:
    data()
    {
        p_data_vector.reset(new std::vector<sd_ptr>);
        p_data_vector->reserve(VECTOR_LEN);
    }
    boost::shared_ptr<std::vector<sd_ptr>>& getDataPtr()
    {
        return p_data_vector;
    }
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        (void)version;
        ar & p_data_vector;
    }
    boost::shared_ptr<std::vector<sd_ptr>> p_data_vector;

};
class dataStore : public Observer<sd_ptr>
{
public:
    dataStore(boost::shared_ptr<sensor>& s) : p_sensor(s)
    {
        using namespace boost::filesystem;
        m_vector.reserve(VECTOR_LEN);
        path dir(store_path);
        boost::format formater("%d");
        int a = 1;
        formater % a;
        path tmpPath = dir / formater.str();
        while(exists(tmpPath))
        {
            ++a;
            formater % a;
            tmpPath = dir / formater.str();
        }
        create_directories(tmpPath);
        m_workPath = tmpPath;
    }
    ~dataStore()
    {
        if(isReg)p_sensor->Detach((Observer<sd_ptr>*)this);
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
        }
    }
    void Update(sd_ptr& p)
    {
        m_vector.push_back(p);
        //std::cout << m_vector.size() << std::endl;
        if(m_vector.size() == VECTOR_LEN)
        {
            boost::mutex::scoped_lock lock(m_mutex);
            m_data.getDataPtr()->swap(m_vector);
            m_cond.notify_one();
            m_endTime = boost::posix_time::to_iso_string(\
                    boost::posix_time::second_clock::local_time());
            formatTimeStr(m_endTime);
        }
    }
    void run()
    {
        reg();
        m_startTime = boost::posix_time::to_iso_string(\
                boost::posix_time::second_clock::local_time());
        formatTimeStr(m_startTime);

        while(1)
        {
            {
                boost::mutex::scoped_lock lock(m_mutex);
                while(m_data.getDataPtr()->size() != VECTOR_LEN)
                {
                    m_cond.wait(lock);
                }
                m_startTime = m_startTime + "---" + m_endTime;
                boost::filesystem::path tmpPath = m_workPath / m_startTime;
                m_startTime.swap(m_endTime);
                std::ofstream out;
                out.open(tmpPath.string(), std::ios::out | std::ios::binary | std::ios::trunc);
                {
                    boost::archive::binary_oarchive oa(out);
                    oa << m_data;
                }
                out.close();
                m_data.getDataPtr().reset(new std::vector<sd_ptr>);
                m_data.getDataPtr()->reserve(VECTOR_LEN);
            }
        }
    }
private:
    std::vector<sd_ptr> m_vector;
    boost::mutex m_mutex;
    boost::condition m_cond;
    data m_data;
    boost::shared_ptr<sensor> p_sensor;
    bool isReg = false;
    boost::filesystem::path m_workPath;
    std::string m_startTime;
    std::string m_endTime;
};
