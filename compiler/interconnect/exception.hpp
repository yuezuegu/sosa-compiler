#ifndef EXCEPTION_HPP_INCLUDED
#define EXCEPTION_HPP_INCLUDED

#include <exception>
#include <sstream>
#include <string>

namespace multistage_interconnect {

struct _string_builder {
    std::ostringstream oss;
    auto str() { return oss.str(); }

    template <typename A, typename ...AA>
    _string_builder &build(A &&a, AA && ...aa) {
        oss << a;
        return build(aa...);
    }

    template <typename A>
    _string_builder &build(A &&a) {
        oss << a;
        return *this;
    }
};

struct Exception : public std::exception {
  template <typename... Args> Exception(Args &&...args) {
    msg_ = _string_builder{}.build(args...).str();
  }

  const char *what() const noexcept override { return msg_.c_str(); }

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
