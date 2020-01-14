#include <type_traits>
#include <utility>
#include <optional>
#include <tuple>

template<class... Fs>
struct all_t {
    template<class... Args>
    constexpr static bool value_t = (Fs::template value_t<Args...> && ...);

    template<auto... Args>
    constexpr static bool value_v = (Fs::template value_v<Args...> && ...);
};

template<class... Fs>
struct any_t {
    template<class... Args>
    constexpr static bool value_t = (Fs::template value_t<Args...> || ...);

    template<auto... Args>
    constexpr static bool value_v = (Fs::template value_v<Args...> || ...);
};

template<class F>
struct not_t {
    template<class... Args>
    constexpr static bool value_t = !F::template value_t<Args...>;

    template<auto... Args>
    constexpr static bool value_v = !F::template value_v<Args...>;
};

template<class... Fs>
struct not_all_t {
    template<class... Args>
    constexpr static bool value_t = not_t<all_t<Fs...>>::template value_t<Args...>;

    template<auto... Args>
    constexpr static bool value_v = not_t<all_t<Fs...>>::template value_v<Args...>;
};

template<class... Fs>
struct none_t {
    template<class... Args>
    constexpr static bool value_t = not_t<any_t<Fs...>>::template value_t<Args...>;

    template<auto... Args>
    constexpr static bool value_v = not_t<any_t<Fs...>>::template value_v<Args...>;
};

template<auto item>
struct constantly_v {
    template<class... Args>
    constexpr static decltype(item) value_t = item;

    template<auto... Args>
    constexpr static decltype(item) value_v = item;
};

template<class T>
struct constantly_t {
    template<class... Args>
    using type_t = T;

    template<auto... Args>
    using type_v = T;
};

struct identity_t {
    template<class Arg>
    using type_t = Arg;

    template<auto Arg>
    constexpr static decltype(Arg) value_v = Arg;
};

template<class F, class... FirstArgs>
struct partial_v_t {
    template<class... OtherArgs>
    constexpr static decltype(F::template value_t<FirstArgs..., OtherArgs...>)
            value_t = F::template value_t<FirstArgs..., OtherArgs...>;
};

template<class F, auto... FirstArgs>
struct partial_v_v {
    template<auto... OtherArgs>
    constexpr static decltype(F::template value_v<FirstArgs..., OtherArgs...>)
            value_v = F::template value_v<FirstArgs..., OtherArgs...>;
};

template<class F, class... FirstArgs>
struct partial_t_t {
    template<class... OtherArgs>
    using type_t = typename F::template type_t<FirstArgs..., OtherArgs...>;
};


template<class F, auto... FirstArgs>
struct partial_t_v {
    template<auto... OtherArgs>
    using type_v = typename F::template type_v<FirstArgs..., OtherArgs...>;
};

template<template<class...> class F>
struct from_non_template_v_t {
    template<class... Args>
    constexpr static decltype(F<Args...>::value) value_t = F<Args...>::value;
};

template<template<auto...> class F>
struct from_non_template_v_v {
    template<auto... Args>
    constexpr static decltype(F<Args...>::value) value_v = F<Args...>::value;
};

template<template<class...> class F>
struct from_non_template_t_t {
    template<class... Args>
    using type_t = typename F<Args...>::type;
};

template<template<auto...> class F>
struct from_non_template_t_v {
    template<auto... Args>
    using type_v = typename F<Args...>::type;
};

template<class F, class... Args>
struct to_non_template_v_t {
    constexpr static decltype(F::template value_t<Args...>) value = F::template value_t<Args...>;
};

template<class F, auto... Args>
struct to_non_template_v_v {
    constexpr static decltype(F::template value_v<Args...>) value = F::template value_v<Args...>;
};

template<class F, class... Args>
struct to_non_template_t_t {
    using type = typename F::template type_t<Args...>;
};

template<class F, auto... Args>
struct to_non_template_t_v {
    using type = typename F::template type_v<Args...>;
};

template<class T>
struct is_t : partial_v_t<from_non_template_v_t<std::is_assignable>, T &> {
    template<auto item>
    constexpr static bool value_v = is_t<T>::template value_t<decltype(item)>;
};

template<class T>
struct is_same_with_t : partial_v_t<from_non_template_v_t<std::is_same>, T> {
    template<auto item>
    constexpr static bool value_v = is_same_with_t<T>::template value_t<decltype(item)>;
};

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

constexpr auto constantly = [](auto &&item) {
    return [item](const auto &&...) { return item; };
};

constexpr auto identity = [](auto &&item) { return item; };

constexpr auto partial = [](auto &&f, auto &&... first) {
    return [f, first...](auto &&... other) { return f(first..., other...); };
};

template<class T>
constexpr auto is = [](auto x) { return is_t<T>::template value_t<decltype(x)>; };

template<class T>
constexpr auto is_same_with = [](auto x) { return is_same_with_t<T>::template value_t<decltype(x)>; };


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
        } else if constexpr (std::is_invocable_v<decltype(other)>) {
            if constexpr (std::is_void_v<decltype(other())>) {
                return opt ? *opt : (other(), void_tag());
            } else {
                return opt ? *opt : other();
            }
        } else {
            static_assert(constantly_v<false>::value_t<S, decltype(opt)>, "There is no handler for the types.");
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
constexpr auto constexpr_cond_v = [](auto... other) {
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
constexpr auto constexpr_cond_t = [](auto... other) {
    constexpr auto impl = [](auto recur, auto f, auto v, auto... other) {
        if constexpr (std::is_convertible_v<decltype(f), else_return_t>) {
            return v;
        } else if constexpr (decltype(f)::template value_t<Args...>) {
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
struct k2v_t {
    T value;

    explicit k2v_t(T value) : value(std::move(value)) {}
};


template<auto F, class T>
constexpr auto k2v(T &&item) {
    return k2v_t<F, T>(item);
}

template<auto item, decltype(item) cur, class S, class = std::enable_if_t<!std::is_same_v<decltype(item), else_return_t>>, class... Other>
constexpr auto constexpr_select(const k2v_t<cur, S> &pair, const Other &... others) {
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

template<auto a, auto... other>
struct min {
    constexpr static std::common_type_t<decltype(a), decltype(other)...> value =
            a < min<other...>::value ? a : min<other...>::value;
};

template<auto a>
struct min<a> {
    constexpr static decltype(a) value = a;
};

template<auto a, auto... other>
struct max {
    constexpr static std::common_type_t<decltype(a), decltype(other)...> value =
            a > max<other...>::value ? a : max<other...>::value;
};

template<auto a>
struct max<a> {
    constexpr static decltype(a) value = a;
};

template<auto item>
struct struct_for {
    using type = decltype(item);
    constexpr static type value = item;
};

#include <vector>

int main() {
    auto vec = std::vector<int>{2, 1, 3};
    auto trace = [](const auto &item) { std::cout << item << std::endl; };
    auto divider = [=] { trace("---"); };
    auto big_divider = [=] { trace("====="); };
    auto do_nothing = [](auto...) {};
    vec | copy | sort() | reverse | println();
    auto byMinusItem = [](auto t) { return -t; };
    vec | copy | sort(byMinusItem) | println();
    vec | copy | rsort(byMinusItem) | println();
    vec | copy | rsort() | println();
    vec | do_nothing;
    divider();
    auto opt = std::make_optional(vec);
    opt | opt_fun(println());
    opt | do_nothing;
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
    trace(null_int
          | otherwise([=] { return 4; }));
    big_divider();
    {
        auto t = (*opt) | move;
        t | println();
        *opt | println();
        divider();
        opt = t | move;
        t | println();
        *opt | println();
        divider();
        std::pair{t, *opt} | tup_fun(println());
        divider();
    }
    auto id_identity = reduce_params_types<decltype(identity)>(identity);
    trace(typeid(id_identity(identity)).name());
//    id_identity(3); -> compilation error
    auto int_identity = reduce_params_types<int>(identity);
//    int_identity(identity); -> compilation error
    trace(int_identity(3));
    big_divider();
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
    big_divider();
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
    trace(is_same_with_t<int>::value_v<3>);
    trace(is_same_with<int &&>(3));
    trace(is_t<int>::value_v<3>);
    trace(is<int>(3));
    trace(is<int &&>(3));
    trace(is<int *>(static_cast<void *>(nullptr)));
    trace(is<void *>(static_cast<int *>(nullptr)));
    divider();
    constexpr int *some_value = nullptr;
    trace(constexpr_cond_v<some_value>(
            is<int>, 'a',
            is<void *>, "b"));
    trace(constexpr_cond_t<float>(
            is_t<int>(), 'a',
            is_t<void *>(), "b"));
    trace(constexpr_cond_t<float>(
            is_same_with_t<int>(), 'a',
            is_t<void *>(), "b",
            else_return, "bebebe"));
    big_divider();
    trace(all_t<is_t<int>, is_t<long>>::value_v<3>);
    trace(all_t<is_same_with_t<int>, is_t<long>>::value_v<3u>);
    trace(all_t<is_same_with_t<int>, is_same_with_t<long>>::value_v<3u>);
    divider();
    trace(all_t<is_t<int>, is_t<long>>::value_t<int>);
    trace(all_t<is_same_with_t<int>, is_t<long>>::value_t<unsigned>);
    trace(all_t<is_same_with_t<int>, is_same_with_t<long>>::value_t<unsigned>);
    divider();
    trace(any_t<is_t<int>, is_t<long>>::value_v<3>);
    trace(any_t<is_same_with_t<int>, is_t<long>>::value_v<3u>);
    trace(any_t<is_same_with_t<int>, is_same_with_t<long>>::value_v<3u>);
    divider();
    trace(none_t<is_t<int>, is_t<long>>::value_t<int>);
    trace(none_t<is_same_with_t<int>, is_t<long>>::value_t<unsigned>);
    trace(none_t<is_same_with_t<int>, is_same_with_t<long>>::value_t<unsigned>);
    divider();
    trace(not_all_t<is_t<int>, is_t<long>>::value_v<3>);
    trace(not_all_t<is_same_with_t<int>, is_t<long>>::value_v<3u>);
    trace(not_all_t<is_same_with_t<int>, is_same_with_t<long>>::value_v<3u>);
    big_divider();
    trace(identity_t::value_v<3>);
    trace(identity_t::type_t<int>());
    divider();
    trace(constantly_v<3>::value_v<22, false>);
    trace(constantly_v<3>::value_t<int, long>);
    trace(constantly_t<int>::type_v<22, false>());
    trace(constantly_t<int>::type_t<int, long>());
    big_divider();
    trace(constexpr_cond_t<int>(
            all_t<is_t<int>, constantly_v<false>>(), 4,
            is_t<char *>(), '5',
            else_return, "6"
    ));
    big_divider();
    auto selector = [](auto &&... args) { return select(args...); };
    auto select5 = partial(selector, 5);
    trace(select5(
            4, 3,
            5, 6,
            else_return, 7));
    trace(partial_v_v<constantly_v<3>, 5>::value_v<6>);
    trace(partial_t_v<constantly_t<int>, 5>::type_v<6>());
    trace(partial_v_t<constantly_v<3>, int>::value_t<long>);
    trace(partial_t_t<constantly_t<int>, int>::type_t<long>());
    divider();
    trace(from_non_template_v_t<std::is_same>::value_t<int, int>);
    trace(from_non_template_t_t<std::common_type>::type_t<int, unsigned long>());
    trace(from_non_template_v_v<min>::value_v<4, 5, 3>);
    trace(from_non_template_v_v<max>::value_v<4, 5, 3>);
    trace(from_non_template_t_v<struct_for>::type_v<4>());
    divider();
    trace(to_non_template_v_t<from_non_template_v_t<std::is_same>, int, int>::value);
    trace(to_non_template_t_t<from_non_template_t_t<std::common_type>, int, unsigned long>::type());
    trace(to_non_template_v_v<from_non_template_v_v<min>, 4, 5, 3>::value);
    trace(to_non_template_v_v<from_non_template_v_v<max>, 4, 5, 3>::value);
    trace(to_non_template_t_v<from_non_template_t_v<struct_for>, 4>::type());
}