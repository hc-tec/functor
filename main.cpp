#include <iostream>
#include <tuple>
#include <type_traits>

template <typename Functor>
struct FunctorTraits;

template <typename R, typename... Args>
struct FunctorTraits<R (*)(Args...)> {
    using ReturnType = R;
};

template <typename Functor>
using MakeFunctorTraits = FunctorTraits<std::decay_t<Functor>>;

template <typename>
struct BindUnwrapTraits {
    template <typename T>
    static T&& Unwrap(T&& o) { return std::forward<T>(o); }
};

template <typename T>
using Unwrapper = BindUnwrapTraits<std::decay_t<T>>;

template <typename T>
auto Unwrap(T&& o) -> decltype(Unwrapper<T>::Unwrap(std::forward<T>(o))) {
    return Unwrapper<T>::Unwrap(std::forward<T>(o));
}

template <typename ReturnType>
struct InvokeHelper {
    template <typename Functor, typename... RunArgs>
    static inline ReturnType MakeItSo(Functor&& functor, RunArgs&&... args) {
        return std::forward<Functor>(functor)(
                std::forward<RunArgs>(args)...);
    }
};

template <typename Functor, typename... Args>
struct Bind {

    using FunctorTraits = MakeFunctorTraits<Functor>;
    using ResultType = typename FunctorTraits::ReturnType;

    Functor&& functor_;
    std::tuple<Args...> args_;

    explicit Bind(Functor&& functor, Args&&... args)
        : functor_(functor),
          args_(args...) {};

    template<typename... UnBoundArgs>
    ResultType Run(UnBoundArgs&&... unbound_args) {

        static constexpr size_t num_bound_args =
                std::tuple_size<decltype(args_)>::value;

        return RunImpl(
                std::forward<Functor>(functor_),
                args_,
                std::make_index_sequence<num_bound_args>(),
                std::forward<UnBoundArgs>(unbound_args)...);
    }

    template <typename BoundArgsTuple, typename... UnBoundArgs, size_t... indices>
    ResultType RunImpl(Functor&& functor,
                       BoundArgsTuple&& bound,
                       std::index_sequence<indices...>,
                       UnBoundArgs&&... unbound_args) {

        return InvokeHelper<ResultType>::MakeItSo(
                std::forward<Functor>(functor_),
                Unwrap(std::get<indices>(std::forward<BoundArgsTuple>(bound)))...,
                std::forward<UnBoundArgs>(unbound_args)...
                );
    }

};

struct Task {
    int id;
    std::string name;
    void show() {
        std::cout << "Task: " << id << "\nname: " << name << std::endl;
    }
};

int Foo(int& a, Task& b, int c) {
    std::cout << a << " c " << c << std::endl;
    b.show();
    return c;
}

int main() {

    std::string str("123456");
    Task task{123, str};

    Task& task1 = task;

    Bind bind = Bind(Foo, 123, static_cast<Task &&>(task1));
    int a = bind.Run(3);
    int b = bind.Run(5);
    std::cout << "返回值a:" << a << std::endl;
    std::cout << "返回值b:" << b << std::endl;

    return 0;
}