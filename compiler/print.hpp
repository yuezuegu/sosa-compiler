#ifndef PRINT_HPP_INCLUDED
#define PRINT_HPP_INCLUDED

#include <type_traits>
#include <utility>

struct print_t {
    template <typename OutputStream>
    auto operator()(OutputStream &&os) {
        return wrapper<OutputStream>{std::forward<OutputStream>(os)};
    }
private:
    template <typename OutputStream>
    struct wrapper {
        template <typename A>
        explicit wrapper(A &&a): os_{std::forward<A>(a)} {
        }

        template <typename ...Args>
        auto &operator()(Args && ...args) {
            impl(std::forward<Args>(args)...);
            return *this;
        }

        template <typename ...Args>
        auto &ln(Args && ...args) {
            (*this)(std::forward<Args>(args)..., "\n");
            return *this;
        }
    private:
        OutputStream os_;

        template <typename Head, typename ...Rest>
        void impl(Head &&head, Rest && ...rest) {
            os_ << head;
            impl(std::forward<Rest>(rest)...);
        }

        template <typename Head>
        void impl(Head &&head) {
            os_ << head;
        }

        void impl() {
        }
    };
};

struct fake_print_functor {
    template <typename ...Args>
    auto &operator()(Args && ...args) {
        impl(std::forward<Args>(args)...);
        return *this;
    }

    template <typename ...Args>
    auto &ln(Args && ...args) {
        impl(std::forward<Args>(args)...);
        return *this;
    }

private:
    template <typename Head, typename ...Rest>
    void impl(Head &&head, Rest && ...rest) {
        (void) head;
        impl(std::forward<Rest>(rest)...);
    }

    template <typename Head>
    void impl(Head &&head) {
        (void) head;
    }

    void impl() {
    }
};

inline print_t print;

#endif // PRINT_HPP_INCLUDED
