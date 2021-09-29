#ifndef EXCEPTION_HPP_INCLUDED
#define EXCEPTION_HPP_INCLUDED

#include <exception>
#include <sstream>
#include <string>

namespace multistage_interconnect {

struct Exception: public std::exception {
    template <typename ...Args>
    Exception(Args && ...args) {
        std::ostringstream oss;
        (oss << ... << args);
        msg_ = oss.str();
    }
    
    const char *what() const noexcept override {
        return msg_.c_str();
    }
private:
    std::string msg_;
};

} // namespace multistage_interconnect

#define MIN_REQUIRE(x)                                                         \
  if (!(x))                                                                    \
    throw Exception{                                                           \
        "\'", #x, "\'", " was expected. [At: ", __FILE__, ":", __LINE__, "]"};

#define MIN_REQUIRE_MSG(x, msg)                                                \
  if (!(x))                                                                    \
    throw Exception{                                                           \
        "\'",     #x,       "\'",     " was expected. Description: ",          \
        (msg),    " [At: ", __FILE__, ":",                                     \
        __LINE__, "]"};

#endif // EXCEPTION_HPP_INCLUDED
