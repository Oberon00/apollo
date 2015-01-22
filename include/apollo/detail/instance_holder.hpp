#ifndef APOLLO_INSTANCE_HOLDER_HPP_INCLUDED
#define APOLLO_INSTANCE_HOLDER_HPP_INCLUDED APOLLO_INSTANCE_HOLDER_HPP_INCLUDED

#include <apollo/smart_ptr.hpp>

#include <boost/get_pointer.hpp>

namespace apollo { namespace detail {

struct class_info;

class instance_holder {
public:
    virtual ~instance_holder() {}
    virtual void* get() = 0; // Get a pointer to the instance.
    virtual class_info const& type() const = 0; // The instance's class.
    virtual bool is_const() const = 0;
};

template <typename T>
class value_instance_holder: public instance_holder {
public:
    value_instance_holder(T&& val, class_info const& cls) // Move val
        : m_instance(std::move(val))
        , m_type(&cls)
    {}

    value_instance_holder(T const& val, class_info const& cls) // Copy val
        : m_instance(val)
        , m_type(&cls)
    {}

    value_instance_holder(value_instance_holder&&) = delete;

    void* get() override
    {
        return const_cast<void*>(
            static_cast<void const*>(std::addressof(m_instance)));
    }

    bool is_const() const override
    {
        return std::is_const<T>::value;
    }

    class_info const& type() const override
    {
        return *m_type;
    }

private:
    T m_instance;
    class_info const* m_type;
};

template <typename Ptr>
class ptr_instance_holder: public instance_holder {
    using ptr_traits = pointer_traits<Ptr>;
public:
    ptr_instance_holder(Ptr&& ptr, class_info const& cls) // Move ptr
        : m_instance(std::move(ptr))
        , m_type(&cls)
    {}

    ptr_instance_holder(Ptr const& ptr, class_info const& cls) // Copy ptr
        : m_instance(ptr)
        , m_type(&cls)
    {}

    ptr_instance_holder(ptr_instance_holder&&) = delete;

    void* get() override
    {
        using boost::get_pointer;
        return const_cast<void*>(
            static_cast<void const*>(get_pointer(m_instance)));

    }

    bool is_const() const override
    {
        return ptr_traits::is_const;
    }

    class_info const& type() const override
    {
        return *m_type;
    }

    Ptr& get_outer_ptr()
    {
        return m_instance;
    }

private:
    Ptr m_instance;
    class_info const* m_type;
};

} } // namespace apollo::detail

#endif // APOLLO_INSTANCE_HOLDER_HPP_INCLUDED
