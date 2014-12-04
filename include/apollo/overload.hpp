#ifndef APOLLO_OVERLOAD_HPP_INCLUDED
#define APOLLO_OVERLOAD_HPP_INCLUDED

#include <apollo/make_function.hpp>
#include <apollo/detail/variadic_pass.hpp>

//#include <iostream>
#include <memory>
#include <vector>

namespace apollo {

namespace detail {

struct overload_quality {
    std::vector<unsigned> param_conversion_steps;
    int n_lua_args_consumed;
};

class overload_base {
public:
    virtual ~overload_base() {}
    virtual overload_quality calculate_quality(lua_State* L) = 0;
    virtual int invoke(lua_State* L) = 0;
    virtual void push_signature(lua_State* L) = 0; // For error messages.
};

template <typename ConvertedF>
class overload: public overload_base {
public:
    template <typename FArg>
    explicit overload(FArg&& f):
        m_f(std::forward<FArg>(f))
    {}

    overload_quality calculate_quality(lua_State* L) override
    {
        using tuple_size_t = typename std::tuple_size<
            typename ConvertedF::tuple_t>::type;
        std::vector<unsigned> result(tuple_size_t::value - 1, no_conversion);
        int lidx = 1;
        param_conversion_steps_impl(
            L, result, lidx,
            std::integral_constant<std::size_t, 1>(), // 0: result converter
            tuple_size_t());
        return {std::move(result), lidx - 1};
    }

    int invoke(lua_State* L) override
    {
        return invoke_with_tuple(L, m_f.fn(), m_f.converters);
    }

    void push_signature(lua_State* L) override
    {
        lua_pushstring(L, typeid(m_f.fn()).name());
    }

private:
    template <std::size_t Idx>
    void param_conversion_steps_impl(
        lua_State*, std::vector<unsigned>&, int&,
        std::integral_constant<std::size_t, Idx>,
        std::integral_constant<std::size_t, Idx>)
    {
        // Reached end of converter tuple.
    }

    template <std::size_t Idx, typename Size>
    void param_conversion_steps_impl(
        lua_State* L, std::vector<unsigned>& result, int& lidx,
        std::integral_constant<std::size_t, Idx>, Size)
    {
        //std::cout << lidx << " -> " << Idx << ": ";
        result[Idx - 1] = n_conversion_steps_with(
            std::get<Idx>(m_f.converters), L, lidx, &lidx);
        //std::cout << result[Idx - 1] << "\n";
        if (result[Idx - 1] == no_conversion)
            return;
        param_conversion_steps_impl(
            L, result, lidx,
            std::integral_constant<std::size_t, Idx + 1>(), Size());
    }

    ConvertedF m_f;
};

template <typename ConvertedF>
std::unique_ptr<overload_base> make_overload(ConvertedF&& f)
{
    using overload_t = overload<
        typename detail::remove_qualifiers<ConvertedF>::type>;
    return std::unique_ptr<overload_base>(
        new overload_t(std::forward<ConvertedF>(f)));
}


class overloadset {
public:
    using vec = std::vector<std::unique_ptr<overload_base>>;

    explicit overloadset(vec&& ovls)
        : m_overloads(std::move(ovls))
    {}

    void push(lua_State* L); // Postcondition: *this has been moved to L

    overloadset(overloadset&& other)
        : m_overloads(std::move(other.m_overloads))
    {}

private:
    vec m_overloads;
};



} // namespace detail

template <>
struct converter<detail::overloadset>: converter_base<detail::overloadset> {
    static void push(lua_State* L, detail::overloadset&& ovls)
    {
        ovls.push(L);
    }
};

template <typename... ConvertedFs>
detail::overloadset make_overloadset_with(ConvertedFs&&... fs)
{
    detail::overloadset::vec overloads;
    detail::variadic_pass((overloads.push_back(
        detail::make_overload(std::forward<ConvertedFs>(fs))), 0)...);
    return detail::overloadset(std::move(overloads));
}

template <typename... Fs>
detail::overloadset make_overloadset(Fs&&... fs)
{
    detail::overloadset::vec overloads;
    detail::variadic_pass((overloads.push_back(
        detail::make_overload(make_function(std::forward<Fs>(fs)))), 0)...);
    return detail::overloadset(std::move(overloads));
}

} // namespace apollo

#endif // APOLLO_OVERLOAD_HPP_INCLUDED
