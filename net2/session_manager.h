#include <boost/typeof/typeof.hpp>
#include <boost/random.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/pool/singleton_pool.hpp>
#include "socket_session.h"
#include "filter_container.h"
namespace firebird{
    template<typename T>
        class var_gen_wraper
        {
            public:
                var_gen_wraper(): gen(boost::mt19937((boost::int32_t)std::time(0)),
                        boost::uniform_smallint<>(1, 100)) {}
                typename T::result_type operator() () { return gen(); }
            private:
                T gen;
        };

    struct session_stu
    {
        DWORD id;
        WORD business_type;
        std::string address;
        DWORD app_id;
        socket_session_ptr session;
    };

    struct sid{};
    struct sbusiness_type{};
    struct saddress{};
    struct sapp_id{};

    enum session_idx_member{ session_id = 0, session_business_type, session_address, app_id};
#define CLIENT 0
#define SERVER 1

    typedef boost::multi_index::multi_index_container<
        session_stu,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
            boost::multi_index::tag<sid>, BOOST_MULTI_INDEX_MEMBER(session_stu, DWORD, id)>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<sbusiness_type>, BOOST_MULTI_INDEX_MEMBER(session_stu, WORD, business_type)>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<saddress>, BOOST_MULTI_INDEX_MEMBER(session_stu, std::string, address)>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<sapp_id>, BOOST_MULTI_INDEX_MEMBER(session_stu, DWORD, app_id)>
            >
            > session_set;

#define MULTI_MEMBER_CON(Tag) typename boost::multi_index::index<session_set,Tag>::type&
#define MULTI_MEMBER_ITR(Tag) typename boost::multi_index::index<session_set,Tag>::type::iterator

    struct is_business_type {
        is_business_type(WORD type)
            :m_type(type)
        {

        }
        bool operator()(const session_stu& s)
        {
            return (s.business_type == m_type);
        }

        WORD m_type;
    };

    class session_manager
    {
        public:
            typedef boost::shared_lock<boost::shared_mutex> readLock;
            typedef boost:: unique_lock<boost::shared_mutex> writeLock;

            session_manager(boost::asio::io_service& io_srv, int type, int expires_time);
            ~session_manager();

            void add_session(socket_session_ptr p);
            void update_session(socket_session_ptr p);

            template<typename Tag, typename Member>
                void del_session(Member m)
                {
                    writeLock lock(m_mutex);
                    if (m_sessions.empty())
                    {
                        return ;
                    }

                    MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);
                    //BOOST_AUTO(idx, boost::multi_index::get<Tag>(m_sessions));
                    BOOST_AUTO(iter, idx.find(m));

                    if (iter != idx.end())
                    {
                        idx.erase(iter);
                    }
                }

            //获取容器中的第一个session
            template<typename Tag, typename Member>
                socket_session_ptr get_session(Member m)
                {
                    readLock lock(m_mutex);

                    if (m_sessions.empty())
                    {
                        return socket_session_ptr();
                    }

                    MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);
                    BOOST_AUTO(iter, idx.find(m));
                    return iter != boost::end(idx) ? iter->session : socket_session_ptr();
                }

            //随机获取容器中的session
            template<typename Tag>
                socket_session_ptr get_session_by_business_type(WORD m)
                {
                    typedef filter_container<is_business_type, MULTI_MEMBER_ITR(Tag)> FilterContainer;
                    readLock lock(m_mutex);

                    if (m_sessions.empty())
                    {
                        return socket_session_ptr();
                    }

                    MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);

                    //对容器的元素条件过滤
                    is_business_type predicate(m);
                    FilterContainer fc(predicate, idx.begin(), idx.end());
                    typename FilterContainer::FilterIter iter = fc.begin();

                    if (fc.begin() == fc.end())
                    {
                        return socket_session_ptr();
                    }

                    //typedef boost::variate_generator<boost::mt19937, boost::uniform_smallint<>> var_gen;
                    //typedef boost::details::pool::singleton_default<var_gen_wraper<var_gen>> s_var_gen;
                    ////根据随机数产生session
                    //s_var_gen::object_type &gen = s_var_gen::instance();
                    //int step = gen() % fc.szie();

                    int step = m_next_session % fc.szie();
                    ++m_next_session;

                    for (int i = 0; i < step; ++i)
                    {
                        iter++;
                    }

                    return iter != fc.end() ? iter->session : socket_session_ptr();
                }

            //根据类型和地址取session
            template<typename Tag>
                socket_session_ptr get_session_by_type_ip(WORD m, std::string& ip)
                {
                    typedef filter_container<is_business_type, MULTI_MEMBER_ITR(Tag)> FilterContainer;
                    readLock lock(m_mutex);

                    if (m_sessions.empty())
                    {
                        return socket_session_ptr();
                    }

                    MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);

                    //对容器的元素条件过滤
                    is_business_type predicate(m);
                    FilterContainer fc(predicate, idx.begin(), idx.end());
                    typename FilterContainer::FilterIter iter = fc.begin();

                    if (fc.begin() == fc.end())
                    {
                        return socket_session_ptr();
                    }

                    while (iter != fc.end())
                    {
                        if (iter->session->get_remote_addr().find(ip) != std::string::npos)
                        {
                            break;
                        }

                        iter++;
                    }

                    return iter != fc.end() ? iter->session : socket_session_ptr();
                }

            //根据类型和app_id取session
            template<typename Tag>
                socket_session_ptr get_session_by_type_appid(WORD m, DWORD app_id)
                {
                    typedef filter_container<is_business_type, MULTI_MEMBER_ITR(Tag)> FilterContainer;
                    readLock lock(m_mutex);

                    if (m_sessions.empty())
                    {
                        return socket_session_ptr();
                    }

                    MULTI_MEMBER_CON(Tag) idx = boost::multi_index::get<Tag>(m_sessions);

                    //对容器的元素条件过滤
                    is_business_type predicate(m);
                    FilterContainer fc(predicate, idx.begin(), idx.end());
                    typename FilterContainer::FilterIter iter = fc.begin();

                    if (fc.begin() == fc.end())
                    {
                        return socket_session_ptr();
                    }

                    while (iter != fc.end())
                    {
                        if (iter->session->get_app_id() == app_id)
                        {
                            break;
                        }

                        iter++;
                    }

                    return iter != fc.end() ? iter->session : socket_session_ptr();
                }

        private:
            int m_type;
            int m_expires_time;
            boost::asio::io_service& m_io_srv;
            boost::asio::deadline_timer m_check_tick;
            boost::shared_mutex m_mutex;
            unsigned short m_next_session;

            session_set m_sessions;

            void check_connection();
    };
}
