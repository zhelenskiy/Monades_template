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

struct void_tag {
};

template<class... Fs>
constexpr auto opt_fun(Fs &&... functors) {
    return [functors...](auto &&arg) {
        using Arg = decltype(arg);
        using R = decltype((*std::forward<Arg>(arg) | ... | functors));
        if constexpr (std::is_void_v<R>) {
            return arg ? ((*std::forward<Arg>(arg) | ... | functors), void_tag()) : std::optional<void_tag>();
        } else {
            return arg ? (*std::forward<Arg>(arg) | ... | functors) : std::optional<R>();
        }
    };
}

template<class T, class S, class = int>
struct has_common_type : std::false_type {
};

template<class T, class S>
struct has_common_type<T, S, decltype(std::declval<std::common_type_t<T, S>>(), 0)> : std::true_type {
};

template<class T, class S>
constexpr bool has_common_type_v = has_common_type<T, S>::value;

template<class S>
constexpr auto otherwise(S &&other) {
    return [other](auto &&opt) {
        if constexpr (has_common_type_v<decltype(opt), S>) {
            return opt ? *opt : other;
        } else if constexpr (std::is_void_v<decltype(other())>) {
            return opt ? *opt : (other(), void_tag());
        } else {
            return opt ? *opt : other();
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

struct else_return_t {
    constexpr bool operator()() const { return true; }
} else_return;

template<class F, class V, class = std::enable_if_t<!std::is_same_v<F, else_return_t>>, class... Other>
auto cond(const F &f, V &&v, Other &&... others) {
    return f() ? v : cond(std::forward<Other>(others)...);
}

template<class V>
constexpr auto cond(else_return_t, V &&v) {
    return v;
}

constexpr auto constexpr_cond = [](auto... other) {
    constexpr auto impl = [](auto recur, auto f, auto v, auto... other) {
        if constexpr (std::is_convertible_v<decltype(f), else_return_t>) {
            return v;
        } else if constexpr (f()) {
            return v;
        } else {
            return recur(recur, other...);
        }
    };
    return impl(impl, other...);
};

template<auto... items>
constexpr auto constexpr_cond_args = [](auto... other) {
    constexpr auto impl = [](auto recur, auto f, auto v, auto... other) {
        if constexpr (std::is_convertible_v<decltype(f), else_return_t>) {
            return v;
        } else if constexpr (f(items...)) {
            return v;
        } else {
            return recur(recur, other...);
        }
    };
    return impl(impl, other...);
};

template<class... Args>
constexpr auto constexpr_cond_types = [](auto... other) {
    constexpr auto impl = [](auto recur, auto f, auto v, auto... other) {
        if constexpr (std::is_convertible_v<decltype(f), else_return_t>) {
            return v;
        } else if constexpr (decltype(f)::template value<Args...>) {
            return v;
        } else {
            return recur(recur, other...);
        }
    };
    return impl(impl, other...);
};

template<class T, class S, class = std::enable_if_t<!std::is_same_v<T, else_return_t>>, class... Other>
constexpr auto select(const T &item, const T &cur, S &&value, const Other &... others) {
    return item == cur ? value : select(item, others...);
}

template<class T, class S>
constexpr auto select(const T &, else_return_t, S &&value) {
    return value;
}

template<auto F, class T>
struct cpair_t {
    T value;

    explicit cpair_t(T value) : value(std::move(value)) {}
};


template<auto F, class T>
constexpr auto k2v(T &&item) {
    return cpair_t<F, T>(item);
}

template<auto item, decltype(item) cur, class S, class = std::enable_if_t<!std::is_same_v<decltype(item), else_return_t>>, class... Other>
constexpr auto constexpr_select(const cpair_t<cur, S> &pair, const Other &... others) {
    if constexpr (item == cur) {
        return pair.value;
    } else {
        return constexpr_select<item>(others...);
    }
}

template<auto T, class S>
constexpr auto constexpr_select(else_return_t, S &&pair) {
    return pair;
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

constexpr auto all = [](auto &&...args) {
    return [=](const auto &... subArgs) { return (args(subArgs...) && ...); };
};

constexpr auto any = [](auto &&...args) {
    return [=](const auto &... subArgs) { return (args(subArgs...) || ...); };
};

constexpr auto not_ = [](auto &&f) {
    return [f](auto &&... items) { return !f(std::forward<decltype(items)>(items)...); };
};

constexpr auto not_all = [](auto &&... fs) {
    return not_(all(std::forward<decltype(fs)>(fs)...));
};

constexpr auto none = [](auto &&... fs) {
    return not_(any(std::forward<decltype(fs)>(fs)...));
};

template<class T>
constexpr auto is = [](auto x) { return std::is_convertible_v<decltype(x), T>; };

template<class T>
constexpr auto is_same_with = [](auto x) { return std::is_same_v<decltype(x), T>; };

template<class T>
struct is_t {
    template<class S>
    constexpr static bool value = std::is_convertible_v<S, T>;
};

template<class T>
struct is_same_with_t {
    template<class S>
    constexpr static bool value = std::is_same_v<S, T>;
};


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
    auto trace = [](const auto &item) { std::cout << item << std::endl; };
    auto divider = [=] { trace("---"); };
    auto doNothing = [](auto...) {};
    vec | copy | sort() | reverse | println();
    auto byMinusItem = [](auto t) { return -t; };
    vec | copy | sort(byMinusItem) | println();
    vec | copy | rsort(byMinusItem) | println();
    vec | copy | rsort() | println();
    vec | doNothing;
    divider();
    auto opt = std::make_optional(vec);
    opt | opt_fun(println());
    opt | doNothing;
    std::optional<std::vector<int>>() | opt_fun(println());
    opt | opt_fun(sort(), println());
    auto check_if_empty = [=](auto &&opt) {
        return opt
               | opt_fun([=](auto) { trace("not empty"); })
               | otherwise([=] { trace("empty"); });
    };
    check_if_empty(opt);
    check_if_empty(std::optional<std::vector<int>>());
    divider();
    const std::optional<int> &null_int = std::optional<int>();
    otherwise(3); //clang compiles second one
    otherwise(3); //clang doesn't compile second one
    trace(null_int | otherwise(3));
    trace(null_int
          | otherwise(null_int)
          | otherwise(3));
    trace(null_int
          | otherwise(std::make_optional(4))
          | otherwise(3));
    trace(null_int
          | otherwise([] { return std::make_optional(4); })
          | otherwise(3));
    trace(null_int
          | otherwise([=] { return null_int; })
          | otherwise(3));
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
    trace(typeid(id_identity(identity)).name());
//    id_identity(3); -> compilation error
    auto int_identity = reduce_params_types<int>(identity);
//    int_identity(identity); -> compilation error
    trace(int_identity(3));
    divider();
//    auto constantly = [](auto &&item) { return [item](auto...) { return item; }; };
    constexpr auto falseFun = [](auto...) { return false; };
    constexpr auto trueFun = [](auto...) { return true; };
    trace(cond(
            falseFun, 0,
            trueFun, 2,
            else_return, 3));
    trace(constexpr_cond(
            falseFun, 0,
            trueFun, "hh1"));
    trace(constexpr_cond(
            falseFun, 0,
            falseFun, "hh1",
            else_return, 2ull));
    divider();
    trace(select(4,
                 4, 6,
                 else_return, 3));
    trace(select(5,
                 4, 6,
                 6, 7,
                 else_return, 3));
    trace(constexpr_select<4>(k2v<4>(6)));
    trace(constexpr_select<5>(
            k2v<4>('a'),
            k2v<6>("b"),
            else_return, 3));
    divider();
    trace(all(trueFun, trueFun)(3, 5));
    trace(all(trueFun, identity)(false));
    trace(any(falseFun, falseFun)(3, 5));
    trace(any(falseFun, identity)(true));
    trace(not_all(trueFun, trueFun)(3, 5));
    trace(not_all(trueFun, identity)(false));
    trace(none(falseFun, falseFun)(3, 5));
    trace(none(falseFun, identity)(true));
    divider();
    trace(is_same_with<int>(3));
    trace(is_same_with<int &&>(3));
    trace(is<int>(3));
    trace(is<int &&>(3));
    trace(is<int *>(static_cast<void *>(nullptr)));
    trace(is<void *>(static_cast<int *>(nullptr)));
    divider();
    constexpr int *some_value = nullptr;
    trace(constexpr_cond_args<some_value>(
            is<int>, 'a',
            is<void *>, "b"));
    trace(constexpr_cond_types<float>(
            is_t<int>(), 'a',
            is_t<void *>(), "b"));
    trace(constexpr_cond_types<float>(
            is_same_with_t<int>(), 'a',
            is_t<void *>(), "b",
            else_return, "bebebe"));
}