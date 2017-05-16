#pragma once
#include <boost/iterator/filter_iterator.hpp>
namespace firebird{
    template <class Predicate, class Iterator>
    class filter_container
    {
    public:
        typedef boost::filter_iterator<Predicate, Iterator> FilterIter;

        filter_container(Predicate p, Iterator begin, Iterator end)
            :m_begin(p, begin, end),
            m_end(p, end, end)
        {

        }
        ~filter_container() {}

        FilterIter begin() { return m_begin; }
        FilterIter end()   { return m_end; }
        int szie() {
            int i = 0;
            FilterIter fi = m_begin;
            while(fi != m_end)
            {
                ++i;
                ++fi;
            }

            return i;
        }

    private:
        FilterIter m_begin;
        FilterIter m_end;
    };
}
