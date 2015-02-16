#ifndef APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED
#define APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED

#include <utility> // std::move, std::forward

namespace apollo {

namespace detail {

template <typename T>
class ref_binder {
public:
    using bound_t = T&;

    explicit ref_binder(T& v)
        : m_ptr(&v), m_is_owner(false)
    {}

    ref_binder(T&& v)
        : m_ptr(new T(std::move(v))), m_is_owner(true)
    {}

    ref_binder(T* ptr, bool is_owner)
        : m_ptr(ptr), m_is_owner(is_owner)
    {}

    ref_binder(ref_binder&& other)
        : m_ptr(other.m_ptr), m_is_owner(other.m_is_owner)
    {
        other.m_is_owner = false;
        other.m_ptr = nullptr;
    }

    ref_binder(ref_binder const&) = delete;
    ref_binder& operator= (ref_binder const&) = delete;

    ~ref_binder()
    {
        if (m_is_owner)
            delete m_ptr;
    }

    T& get() const { return *m_ptr; }

    bool owns_object() const { return m_is_owner; }

private:
    T* m_ptr;
    bool m_is_owner;
};
} // namespace detail


template <typename T>
T&& unwrap_ref(T&& v) { return std::forward<T>(v); }

template <typename T>
T& unwrap_ref(detail::ref_binder<T> const& rv) { return rv.get(); }

template <typename T>
T& unwrap_ref(detail::ref_binder<T>& rv) { return rv.get(); }

template <typename T>
T& unwrap_ref(detail::ref_binder<T>&& rv) { return rv.get(); }

} // namespace apollo

#endif // APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED
