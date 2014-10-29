#ifndef APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED
#define APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED

namespace apollo {

namespace detail {

template <typename T>
class ref_binder {
public:
    using bound_t = T&;

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
    explicit operator T& () const { return get(); }

    bool owns_object() const { return m_is_owner; }

private:
    T* m_ptr;
    bool m_is_owner;
};
} // namespace detail


template <typename T>
T&& unwrap_bound_ref(T&& v) { return std::forward<T>(v); }

template <typename T>
T& unwrap_bound_ref(detail::ref_binder<T> const& rv) { return rv.get(); }

template <typename T>
T& unwrap_bound_ref(detail::ref_binder<T>& rv) { return rv.get(); }

template <typename T>
T& unwrap_bound_ref(detail::ref_binder<T>&& rv) { return rv.get(); }

} // namespace apollo

#endif // APOLLO_DETAIL_REF_BINDER_HPP_INCLUDED
