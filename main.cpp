#include <type_traits>
#include <utility>
#include <optional>
#include <tuple>

template<class T, class F, class = std::enable_if_t<std::is_invocable_v<F, T>>>
constexpr decltype(auto) operator|(T &&operand, F &&functor) {
    return functor(std::forward<T>(operand));
}

template<class T, class F, class = std::enable_if_t<std::is_invocable_v<F, T>>>
constexpr decltype(auto) operator|(const T &operand, F &&functor) {
    return functor(operand);
}

template<class... Fs>
constexpr auto opt_fun(Fs &&... functors) {
    return [functors...](auto &&arg) {
        using Arg = decltype(arg);
        using R = decltype((*std::forward<Arg>(arg) | ... | functors));
        if constexpr (std::is_void_v<R>) {
            if (arg) {
                (*std::forward<Arg>(arg) | ... | functors);
            }
        } else {
            return arg ? (*std::forward<Arg>(arg) | ... | functors) : std::optional<R>();
        }
    };
}

template<class... Fs>
constexpr auto tup_fun(Fs &&... functors) {
    return [functors...](auto &&arg) {
        auto apply_functors = [&](auto &&item) { return (std::forward<decltype(item)>(item) | ... | functors); };
        return std::apply([&](auto &&... items) { return std::make_tuple(apply_functors(items)...); },
                          std::forward<decltype(arg)>(arg));
    };
}

template<class... Params, class F>
constexpr auto reduce_params_types(F &&functor) {
    return [functor](Params... params) { return functor(std::forward<Params>(params)...); };
}

constexpr auto copy = [](auto item) { return item; };
constexpr auto move = [](auto &&item) { return std::move(item); }; // NOLINT(bugprone-move-forwarding-reference)

#include <algorithm>

constexpr auto reverse = [](auto &&cont) {
    std::reverse(cont.begin(), cont.end());
    return cont;
};


template<class KeyFinder>
constexpr auto sort(const KeyFinder &keyFinder) {
    return [=](auto &&cont) {
        std::sort(cont.begin(), cont.end(), [&](const auto &a, const auto &b) { return keyFinder(a) < keyFinder(b); });
        return cont;
    };
}

constexpr auto sort() {
    return [](auto &&cont) {
        return cont | sort([](const auto &item) { return item; });
    };
}

template<class... Args>
constexpr auto rsort(Args &&... args) {
    return [=](auto &&cont) {
        return cont | sort(args...) | reverse;
    };
}

#include <iostream>

auto print(const std::string &sep = " ", std::ostream &out = std::cout) {
    return [&out, sep](const auto &container) {
        auto iter = container.cbegin();
        if (iter != container.cend()) {
            out << *iter;
            for (++iter; iter != container.cend(); ++iter) {
                out << sep << *iter;
            }
        }
        return container;
    };
}

auto println(const std::string &sep = " ", std::ostream &out = std::cout, bool flush = true) {
    return [=, &out](const auto &container) {
        container | print(sep, out);
        if (flush) {
            out << std::endl;
        } else {
            out << '\n';
        }
        return container;
    };
}

#include <vector>

int main() {
    auto vec = std::vector<int>{2, 1, 3};
    auto divider = [] { std::cout << "---" << std::endl; };
    auto doNothing = [](auto...) {};
    vec | copy | sort() | reverse | println();
    vec | copy | sort([](auto t) { return -t; }) | println();
    vec | copy | rsort([](auto t) { return -t; }) | println();
    vec | copy | rsort() | println();
    vec | doNothing;
    divider();
    auto opt = std::make_optional(vec);
    opt | opt_fun(println());
    opt | doNothing;
    std::optional<std::vector<int>>() | opt_fun(println());
    opt | opt_fun(sort(), println());
    opt | opt_fun([](auto x) { return (std::cout << "not empty\n", x); }) || (std::cout << "empty\n");
    std::optional<std::vector<int>>()
    | opt_fun([](auto x) { return (std::cout << "not empty\n", x); })
    || (std::cout << "empty\n");
    divider();
    {
        auto t = (*opt) | move;
        t | println();
        *opt | println();
        divider();
        opt = t | move;
        t | println();
        *opt | println();
        divider();
        std::pair{*opt, t} | tup_fun(println());
        divider();
    }
    auto identity = [](auto &&item) { return item; };
    auto id_identity = reduce_params_types<decltype(identity)>(identity);
    std::cout << typeid(id_identity(identity)).name() << std::endl;
//    id_identity(3); -> compilation error
    auto int_identity = reduce_params_types<int>(identity);
//    int_identity(identity); -> compilation error
    std::cout << int_identity(3) << std::endl;
}