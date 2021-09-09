
#ifndef LOGGER_SETUP_HPP
#define LOGGER_SETUP_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#pragma GCC diagnostic pop

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

void logger_setup(){
    logging::add_file_log("sample.log");
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );
}

#endif /* LOGGER_SETUP_HPP */