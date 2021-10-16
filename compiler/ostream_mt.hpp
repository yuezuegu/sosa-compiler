/**
 * @file ostream_mt.hpp
 * @brief Provides wrappers to make ostream's thread-safe.
 * @author Canberk Sonmez <canberk.sonmez@epfl.ch>
 * 
 */

#ifndef OSTREAM_MT
#define OSTREAM_MT

#include <ios>
#include <sstream>
#include <mutex>

struct ostream_mt {
    ostream_mt(std::ostream &os): os_{os} {
    }

    auto operator()() {
        return wrapper{*this};
    }

private:
    struct wrapper: std::ostringstream {
        wrapper(ostream_mt &osmt): osmt_{osmt} {
        }

        wrapper(wrapper &&w): std::ostringstream{std::move(w)}, osmt_{w.osmt_} {
        }

        ~wrapper() {
            std::unique_lock<std::mutex> lock{osmt_.mtx_};
            osmt_.os_ << this->str();
            osmt_.os_.flush();
        }
    private:
        ostream_mt &osmt_;
    };

    std::ostream &os_;
    std::mutex mtx_;
};

extern ostream_mt cout_mt;

#endif // OSTREAM_MT
