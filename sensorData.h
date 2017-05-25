#pragma once
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
struct sensorData
{
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        (void)version;
        ar & accel;
        ar & gyro;
        ar & euler;
    }
    sensorData(){}
    sensorData(float* a, float* g, float* e)
    {
        for(int i=0; i < 3; ++i)
            accel[i] = *(a + i);
        for(int i=0; i < 3; ++i)
            gyro[i] = *(g + i);
        for(int i=0; i < 3; ++i)
            euler[i] = *(e + i);
    }
    sensorData(const sensorData& s)
    {
        for(int i=0; i < 3; ++i)
        {
            this->accel[i] = s.accel[i];
            this->gyro[i] = s.gyro[i];
            this->euler[i] = s.euler[i];
        }
    }
    float accel[3];
    float gyro[3];
    float euler[3];
};
typedef boost::shared_ptr<sensorData> sd_ptr;
