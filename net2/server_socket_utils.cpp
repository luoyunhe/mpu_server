#include "server_socket_utils.h"
namespace firebird{
    server_socket_utils::server_socket_utils(int port)
        :m_work(m_io_srv),
        m_acceptor(m_io_srv, tcp::endpoint(tcp::v4(), port)),
        m_manager(m_io_srv, SERVER, 3)
    {
        //m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        //// 关闭连接前留0秒给客户接收数据
        //m_acceptor.set_option(boost::asio::ip::tcp::acceptor::linger(true, 0));
        //m_acceptor.set_option(boost::asio::ip::tcp::no_delay(true));
        //m_acceptor.set_option(boost::asio::socket_base::keep_alive(true));
        //m_acceptor.set_option(boost::asio::socket_base::receive_buffer_size(16384));
    }

    server_socket_utils::~server_socket_utils(void)
    {
    }

    void server_socket_utils::start()
    {
        try{
            socket_session_ptr new_session(new socket_session(m_io_srv));
            m_acceptor.async_accept(new_session->socket(),
                boost::bind(&server_socket_utils::handle_accept, this, new_session,
                boost::asio::placeholders::error));
        }
        catch(std::exception& e)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[" << e.what() << "]");
        }
        catch(...)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[未知异常]");
        }
    }

    void server_socket_utils::handle_accept(socket_session_ptr session, const boost::system::error_code& error)
    {
        if (!error)
        {
            try{
                socket_session_ptr new_session(new socket_session(m_io_srv));
                m_acceptor.async_accept(new_session->socket(),
                    boost::bind(&server_socket_utils::handle_accept, this, new_session,
                    boost::asio::placeholders::error));

                if (session != NULL)
                {
                    //注册关闭回调函数
                    session->installCloseCallBack(boost::bind(&server_socket_utils::close_callback, this, _1));
                    //注册读到数据回调函数
                    session->installReadDataCallBack(boost::bind(&server_socket_utils::read_data_callback, this, _1, _2, _3));

                    boost::format fmt("%1%:%2%");
                    fmt % session->socket().remote_endpoint().address().to_string();
                    fmt % session->socket().remote_endpoint().port();
                    std::string tmp = fmt.str();
                    session->set_remote_addr(tmp);

                    session->start();
                    m_manager.add_session(session);
                }
            }
            catch(std::exception& e)
            {
                LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[" << e.what() << "]");
            }
            catch(...)
            {
                LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[未知异常]");
            }

        }
    }

    void server_socket_utils::close_callback(socket_session_ptr session)
    {
        LOG4CXX_DEBUG(firebird_log, "close_callback");
        try{
            m_manager.del_session<sid>(session->id());
            handle_close(session->id());
        }
        catch(std::exception& e)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[" << e.what() << "]");
        }
        catch(...)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[未知异常]");
        }

    }

    void server_socket_utils::read_data_callback(const boost::system::error_code& e,
        socket_session_ptr session, message& msg)
    {
        try{
            LOG4CXX_DEBUG(firebird_log, "command =[" << msg.command << "],["
                << msg.business_type << "],[" << msg.data() << "]");

            if (msg.command == heartbeat)
            {//心跳
                session->async_write(msg);
            }
            else if (msg.command == regist)
            {//注册
                session->set_business_type(msg.business_type);
                session->set_app_id(msg.app_id);
                m_manager.update_session(session);

                session->async_write(msg);
                LOG4CXX_FATAL(firebird_log, "远程地址:[" << session->get_remote_addr() << "],服务器类型:[" <<
                    session->get_business_type() << "],服务器ID:[" << session->get_app_id() << "]注册成功!");
            }
            else if (msg.command >= normal)
            {//业务数据
                handle_read_data(msg, session);
            }
            else
            {
                LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "收到非法消息包!");
            }
        }
        catch(std::exception& e)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[" << e.what() << "]");
        }
        catch(...)
        {
            LOG4CXX_ERROR(firebird_log, KDS_CODE_INFO << "socket异常:[未知异常]");
        }
    }
}
