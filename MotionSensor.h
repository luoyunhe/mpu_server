#ifndef _MOTION_SENSOR_H_
#define _MOTION_SENSOR_H_
#include <boost/thread/mutex.hpp>
#include "sensorData.h"
#include "observerModel.h"
#include <list>

#define YAW 0
#define PITCH 1
#define ROLL 2
#define DIM 3
extern float ypr[3]; //yaw, pitch, roll
extern float accel[3];
extern float gyro[3];
extern float temp;
extern float compass[3];

extern int ms_open();
extern int ms_update();
extern int ms_close();
class sensor : public Subject<boost::shared_ptr<sensorData>>
{
    public:
        sensor(){ms_open();}
        ~sensor(){ms_close();}
        void Attach(Observer<sd_ptr>* ob)
        {
            boost::mutex::scoped_lock lock(m_mutex);
            m_observerList.push_back(ob);
        }
        void Detach(Observer<sd_ptr>* ob)
        {
            boost::mutex::scoped_lock lock(m_mutex);
            m_observerList.remove(ob);
        }
        void Notify()
        {
            ms_update();
            boost::shared_ptr<sensorData> tmp(new sensorData(accel, gyro, ypr));
            boost::mutex::scoped_lock lock(m_mutex);
            std::list<Observer<sd_ptr>*>::iterator it = m_observerList.begin();
            while(it != m_observerList.end())
            {
                (*it)->Update(tmp);
                ++it;
            }
        }
    private:
        boost::mutex m_mutex;
        std::list<Observer<sd_ptr>*> m_observerList;
};

#endif
