#include "session_manager.h"

namespace firebird{
    session_manager::session_manager(boost::asio::io_service& io_srv, int type, int expires_time)
        :m_io_srv(io_srv), m_check_tick(io_srv), m_type(type), m_expires_time(expires_time),m_next_session(0)
    {
        check_connection();
    }

    session_manager::~session_manager()
    {

    }

    //检查服务器所有session的连接状态
    void session_manager::check_connection()
    {
        try{
            writeLock lock(m_mutex);

            session_set::iterator iter = m_sessions.begin();
            while (iter != m_sessions.end())
            {
                LOG4CXX_DEBUG(firebird_log, "循环");
                if (CLIENT == m_type)//客户端的方式
                {
                    if (!iter->session->socket().is_open())//已断开，删除已断开的连接
                    {
                        LOG4CXX_INFO(firebird_log, "重新连接[" << iter->address << "]");
                        iter->session->close(); //通过关闭触发客户端重连
                    }
                    else{//连接中，发送心跳
                        message msg;
                        msg.command = heartbeat;
                        msg.business_type = iter->session->get_business_type();
                        msg.app_id = iter->session->get_app_id();
                        msg.data() = "H";

                        iter->session->async_write(msg);
                        iter->session->set_op_time();
                    }
                }
                else if (SERVER == m_type)//服务器的方式
                {
                    if (!iter->session->socket().is_open())//已断开，删除已断开的连接
                    {
                        LOG4CXX_INFO(firebird_log, KDS_CODE_INFO << "删除已关闭的session:[" << iter->session->get_remote_addr() << "]");
                        iter = m_sessions.erase(iter);
                        continue;
                    }
                    else{//连接中，设定每30秒检查一次
                        if (iter->session->is_timeout()) //如果session已长时间没操作，则关闭
                        {
                            LOG4CXX_INFO(firebird_log, KDS_CODE_INFO << "删除已超时的session:[" << iter->session->get_remote_addr() << "]");
                            iter->session->close();//通过关闭触发删除session
                        }
                    }

                    iter->session->set_op_time();
                }
                else{
                    LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "unknown manager_type");
                }
                ++iter;
            }

            LOG4CXX_DEBUG(firebird_log, "定时检查");
            m_check_tick.expires_from_now(boost::posix_time::seconds(m_expires_time));
            m_check_tick.async_wait(boost::bind(&session_manager::check_connection, this));
        }
        catch(std::exception& e)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "[" << e.what() << "]");
        }
        catch(...)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "unknown exception.");
        }
    }

    void session_manager::add_session(socket_session_ptr p)
    {
        writeLock lock(m_mutex);
        session_stu stuSession;
        stuSession.id = p->id();
        stuSession.business_type = 0;
        stuSession.address = p->get_remote_addr();
        stuSession.app_id = p->get_app_id();
        stuSession.session = p;
        m_sessions.insert(stuSession);
    }

    void session_manager::update_session(socket_session_ptr p)
    {
        writeLock lock(m_mutex);
        if (m_sessions.empty())
        {
            return ;
        }

        MULTI_MEMBER_CON(sid) idx = boost::multi_index::get<sid>(m_sessions);
        BOOST_AUTO(iter, idx.find(p->id()));

        if (iter != idx.end())
        {
            const_cast<session_stu&>(*iter).business_type = p->get_business_type();
            const_cast<session_stu&>(*iter).app_id = p->get_app_id();
        }
    }
}
