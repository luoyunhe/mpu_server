#pragma once
#include "socket_session.h"
#include "session_manager.h"
#include <boost/format.hpp>
//#include <firebird/message/message.hpp>
namespace firebird{
    using boost::asio::ip::tcp;

    class server_socket_utils
    {
    private:
        boost::asio::io_service m_io_srv;
        boost::asio::io_service::work m_work;
        tcp::acceptor m_acceptor;

        void handle_accept(socket_session_ptr session, const boost::system::error_code& error);

        void close_callback(socket_session_ptr session);
        void read_data_callback(const boost::system::error_code& e,
            socket_session_ptr session, message& msg);

    protected:
        virtual void handle_read_data(message& msg, socket_session_ptr pSession) = 0;

    public:
        server_socket_utils(int port);
        ~server_socket_utils(void);

        void start();
        boost::asio::io_service& get_io_service() { return m_io_srv; }

        session_manager m_manager;
    };
}
