/*=============================================================================
    Copyright (c) 2007-2013 Andrey Semashev
	Copyright (c) 2012-2013 Richard Otis

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "libtdb/include/libtdb_pch.hpp"
#include "libtdb/include/logging.hpp"

using namespace journal;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

// The operator puts a human-friendly representation of the severity level to the stream
std::ostream& operator<< (std::ostream& strm, severity_level level)
{
    static const char* strings[] =
    {
    	"debug",
        "routine",
        "warning",
        "critical"
    };

    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

// The operator is used when putting the severity level to log
logging::formatting_ostream& operator<<
(
    logging::formatting_ostream& strm,
    logging::to_log_manip< severity_level, tag::severity > const& manip
)
{
    static const char* strings[] =
    {
    	"debug",
        "routine",
        "warning",
        "critical"
    };

    severity_level level = manip.get();
    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

void init_logging()
{
    // Create a minimal severity table filter
    typedef expr::channel_severity_filter_actor< std::string, severity_level > min_severity_filter;
    min_severity_filter min_severity = expr::channel_severity_filter(channel, severity);

    // Set up the minimum severity levels for different channels
    min_severity["network"] = warning;
    min_severity["optimizer"] = warning;
    min_severity["data"] = routine;

    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< sinks::text_file_backend > backend =
        boost::make_shared< sinks::text_file_backend >(
            keywords::file_name = "tdbread_%5N.log",
            keywords::rotation_size = 5 * 1024 * 1024
        );


    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink_t;
    boost::shared_ptr< file_sink_t > sink(new file_sink_t(backend));

    sink->set_filter(min_severity || severity >= debug); // capture everything to debug

    sink->set_formatter
    (
        expr::format("#%1% <%2%> %3%")
            % channel
            % severity
            % expr::smessage
    );

    core->add_sink(sink);



    boost::shared_ptr< sinks::text_ostream_backend > consolebackend =
        boost::make_shared< sinks::text_ostream_backend >();
    consolebackend->add_stream(
        boost::shared_ptr< std::ostream >(&std::clog, logging::empty_deleter()));

    // Enable auto-flushing after each log record written
    //backend->auto_flush(true);

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > ostream_sink_t;
    boost::shared_ptr< ostream_sink_t > consolesink(new ostream_sink_t(consolebackend));
    consolesink->set_filter(min_severity || severity >= critical); // be selective with the console
    core->add_sink(consolesink);
}
