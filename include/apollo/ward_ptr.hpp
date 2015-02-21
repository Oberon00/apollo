// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_WARD_PTR_HPP_INCLUDED
#define APOLLO_WARD_PTR_HPP_INCLUDED WEAK_REF_HPP_INCLUDED

#include <boost/noncopyable.hpp>
#include <boost/assert.hpp>

#include <cassert>
#include <stdexcept>
#include <type_traits>


namespace apollo {

class bad_ward_ptr: public std::logic_error
{
public:
    bad_ward_ptr(): std::logic_error("attempt to use an invalid ward_ptr") { }
};

template <typename T>
class enable_ward_ptr_from_this;

namespace detail {
struct ward_ptr_connection: private boost::noncopyable {
    ward_ptr_connection(void* r): n_refs(0), referenced(r) { }

    void unref() {
        BOOST_ASSERT(n_refs > 0);
        if (--n_refs == 0 && !referenced)
            delete this;
    }

    void invalidate() {
        BOOST_ASSERT(referenced);
        referenced = nullptr;
        if (n_refs == 0)
            delete this;
    }

    std::size_t n_refs;
    void* referenced;
};

template<typename T>
detail::ward_ptr_connection* get_connection(enable_ward_ptr_from_this<T>* r);

} // namespace detail

template <typename T>
class ward_ptr {
public:

    ward_ptr():
        m_offset(0),
        m_connection(new detail::ward_ptr_connection(nullptr))
    {
        ++m_connection->n_refs;
    }

    ward_ptr(enable_ward_ptr_from_this<T>* t);


    template<typename U>
    ward_ptr(enable_ward_ptr_from_this<U>* u);

    ward_ptr(ward_ptr const& rhs):
        m_offset(rhs.m_offset),
        m_connection(rhs.m_connection)
    {
        ++m_connection->n_refs;
    }

    ward_ptr& operator= (ward_ptr const& rhs)
    {
        m_connection->unref();
        m_connection = rhs.m_connection;
        m_offset = rhs.m_offset;
        ++m_connection->n_refs;
        return *this;
    }

    ~ward_ptr()
    {
        m_connection->unref();
    }

    bool operator== (ward_ptr const& rhs) const { return rhs.get() == get(); }
    bool operator!= (ward_ptr const& rhs) const { return rhs.get() != get(); }

    T& operator* () const { return *validate(); }
    T* operator-> () const { return validate(); }
    T* get() const {  return m_connection->referenced ? deref() : nullptr; }

    bool operator! () const { return !valid(); }
    bool valid() const { return m_connection->referenced != nullptr; }

private:
    T* deref() const
    {
        return reinterpret_cast<T*>(
            reinterpret_cast<intptr_t>(m_connection->referenced) + m_offset);
    }

    T* validate() const
    {
        if (!m_connection->referenced)
            throw bad_ward_ptr();
        return deref();
    }

    std::ptrdiff_t m_offset;
    detail::ward_ptr_connection* m_connection;
};


template <typename T>
T* get_pointer(ward_ptr<T> const& r)
{
    return r.get();
}

template <typename T>
class enable_ward_ptr_from_this {
public:
    typedef T referenced_type;

    enable_ward_ptr_from_this():
      m_connection(nullptr)
    { }

    ward_ptr<T> ref()
    {
        ensure_connection();
        return ward_ptr<T>(this);
    }


    template<typename U>
    ward_ptr<U> ref()
    {
        return ward_ptr<U>(this);
    }

    ~enable_ward_ptr_from_this()
    {
        if (m_connection)
            m_connection->invalidate();
    }

private:
    friend detail::ward_ptr_connection* detail::get_connection<T>(enable_ward_ptr_from_this<T>* r);

    void ensure_connection()
    {
        if (!m_connection)
            m_connection = new detail::ward_ptr_connection(static_cast<T*>(this));
    }

    detail::ward_ptr_connection* m_connection;
};

template <typename T>
bool is_valid_ward_ptr(ward_ptr<T> ref)
{
    return ref.valid();
}

namespace detail {
template <typename T>
detail::ward_ptr_connection* get_connection(enable_ward_ptr_from_this<T>* r)
{
    if (!r)
        return new ward_ptr_connection(nullptr);
    r->ensure_connection();
    return r->m_connection;
}
} // namespace detail

template<typename T>
template<typename U>
ward_ptr<T>::ward_ptr(enable_ward_ptr_from_this<U>* u)
{
    static_assert(std::is_convertible<T*, U*>::value, "Incompatible pointers!");
    m_connection = detail::get_connection(u);
    BOOST_ASSERT(m_connection->referenced == static_cast<U*>(u));
    T* t = static_cast<T*>(static_cast<U*>(u));
    m_offset =
        reinterpret_cast<intptr_t>(t) -
        reinterpret_cast<intptr_t>(m_connection->referenced);
    BOOST_ASSERT(m_offset == 0);
    BOOST_ASSERT(!u || dynamic_cast<T*>(static_cast<U*>(u)));
    ++m_connection->n_refs;
}

template<typename T>
ward_ptr<T>::ward_ptr(enable_ward_ptr_from_this<T>* t):
    m_offset(0),
    m_connection(detail::get_connection(t))
{
    BOOST_ASSERT(m_connection->referenced == static_cast<T*>(t));
    ++m_connection->n_refs;
}

} // namespace apollo

#endif // APOLLO_WARD_PTR_HPP_INCLUDED
