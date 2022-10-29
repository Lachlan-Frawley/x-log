#pragma once

#include <boost/log/sinks/basic_sink_backend.hpp>

class xlog_journal_backend final :
    public boost::log::sinks::basic_sink_backend<
        boost::log::sinks::combine_requirements<
            boost::log::sinks::synchronized_feeding
        >::type
    >
{
public:
    void consume(const boost::log::record_view& rec);
};
