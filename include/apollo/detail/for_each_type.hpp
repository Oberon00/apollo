#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/type_traits/is_same.hpp>

namespace apollo {
namespace detail {

namespace mpl = boost::mpl;

template <typename I, typename End, typename F>
void for_each_type_i(F f, boost::false_type)
{
    f.template apply<typename mpl::deref<I>::type>();
    using Next = typename mpl::next<I>::type;
    using AtEnd = typename boost::is_same<Next, End>::type;
    for_each_type_i<Next, End>(f, AtEnd());
}

template <typename, typename, typename F>
void for_each_type_i(F, boost::true_type)
{
    // Reached end: do nothing.
}

template <typename Seq, typename F>
void for_each_type(F f)
{
    using Beg = typename mpl::begin<Seq>::type;
    using End = typename mpl::end<Seq>::type;
    using AtEnd = typename boost::is_same<Beg, End>::type;
    for_each_type_i<Beg, End>(f, AtEnd());
}

}
} // namespace apollo::detail
