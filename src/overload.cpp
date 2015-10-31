// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/overload.hpp>

//#include <iostream>

namespace {

using apollo::detail::overload;
using apollo::detail::overload_base;
using apollo::detail::overload_quality;
using ovl_with_quality = std::pair<overload_base*, overload_quality>;
using overload_vec = apollo::detail::overloadset::vec;


// Precondition: lhs and rhs must be viable.
static bool is_better_overload(
    overload_quality const& lhs, overload_quality const& rhs)
{
    if (lhs.n_lua_args_consumed < rhs.n_lua_args_consumed)
        return false;
    auto const n = std::min(
        lhs.param_conversion_steps.size(), rhs.param_conversion_steps.size());

    bool lhs_has_better =
        lhs.n_lua_args_consumed > rhs.n_lua_args_consumed;

    for (std::size_t i = 0; i < n; ++i) {
        auto lhs_steps = lhs.param_conversion_steps[i];
        auto rhs_steps = rhs.param_conversion_steps[i];
        if (lhs_steps > rhs_steps)
            return false;
        if (lhs_steps < rhs_steps)
            lhs_has_better = true;
    }
    return lhs_has_better;
}

static std::vector<ovl_with_quality> select_best_overloads(
    lua_State* L, overload_vec& overloads)
{
    std::vector<ovl_with_quality> best;

    for (auto& ovl: overloads) {
        auto quality = ovl->calculate_quality(L);
        bool is_viable = std::find(
                quality.param_conversion_steps.begin(),
                quality.param_conversion_steps.end(),
                apollo::no_conversion)
            == quality.param_conversion_steps.end();
        if (!is_viable)
            continue;

        //std::cout << "Viable: ";
        //ovl->push_signature(L);
        //std::cout << lua_tostring(L, -1) << "\n";
        //lua_pop(L, 1);

        auto n_better = static_cast<std::size_t>(std::count_if(
            best.begin(), best.end(),
            [quality](ovl_with_quality const& elem) {
                return is_better_overload(quality, elem.second);
            }));

        if (n_better == best.size()) { // Automatically handles empty best.
            best.clear();
            best.emplace_back(ovl.get(), std::move(quality));
            //puts("Cleared viables, because this one is better.");
        } else if (n_better > 0) {
            best.emplace_back(ovl.get(), std::move(quality));
        } else {
            if (
                best.end() == std::find_if(
                    best.begin(), best.end(),
                    [quality](ovl_with_quality const& elem) {
                        return is_better_overload(elem.second, quality);
                    })
               ) {
                best.emplace_back(ovl.get(), std::move(quality));
            }
        }
    }
    //std::cout << best.size() << " candidates selected.\n";
    return best;
}

int dispatch_overload(lua_State* L)
{
    overload_base* f = nullptr;
    {
        auto& overloads = *static_cast<overload_vec*>(
            lua_touserdata(L, lua_upvalueindex(1)));
        BOOST_ASSERT(!overloads.empty());

        auto candidates = select_best_overloads(L, overloads);

        if (candidates.size() == 1) {
            f = candidates.front().first;
        } else if (candidates.empty()) {
            lua_checkstack(L, static_cast<int>(1 + overloads.size() * 3));
            lua_pushliteral(
                L,
                "No overload is viable for the given arguments. Overloads:\n");
            for (auto& c: overloads) {
                lua_pushliteral(L, "  ");
                c->push_signature(L);
                lua_pushliteral(L, "\n");
            }
            lua_concat(L, static_cast<int>(1 + overloads.size() * 3));
        } else {
            lua_checkstack(L, static_cast<int>(2 + overloads.size() * 3));
            lua_pushliteral(
                L, "Ambiguous call to overloaded function, candidates:\n");
            for (auto& c: candidates) {
                lua_pushliteral(L, "  ");
                c.first->push_signature(L);
                lua_pushliteral(L, "\n");
            }
            lua_pushfstring(L, "  (in total, %d candidates of %d overloads)",
                static_cast<int>(candidates.size()),
                static_cast<int>(overloads.size()));
            lua_concat(L, static_cast<int>(2 + candidates.size() * 3));
        }
    }
    if (f)
        return f->invoke(L);
    return lua_error(L);
}

} // anonymous namespace

APOLLO_API void apollo::detail::overloadset::push(lua_State* L)
{
    push_gc_object(L, std::move(m_overloads));
    lua_pushcclosure(L, raw_function::caught<&dispatch_overload>(), 1);
}
