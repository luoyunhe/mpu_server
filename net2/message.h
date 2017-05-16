#pragma once
#include <iostream>
#include <cstring>
typedef unsigned short WORD;
typedef unsigned long DWORD;
struct Context{
    unsigned short cmdVersion;
    std::string remote_ip;
    DWORD session_id;
};
enum commandType{
    heartbeat = 0,
    regist,
    normal
};
class message{
public:
    message(std::string& str)
    {
        unsigned long size = str.size();
        if(size >= 2)
        {
            memcpy(&command, &str[0], 2);
            if(size >= 6)
            {
                memcpy(&business_type, &str[2], 2);
                memcpy(&app_id, &str[4], 2);
                size =  size - 6;
                if(size > 0)
                {
                    m_data.resize(size);
                    memcpy(&m_data[0], &str[6], size);
                }
            }

        }
        }
    std::string& data(){return m_data;}
    WORD command;
    WORD business_type;
    WORD app_id;
    Context& context(){return m_ct;}
private:
    Context m_ct;
    std::string m_data;
};
