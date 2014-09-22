#ifndef APOLLO_INSTANCE_HOLDER_HPP_INCLUDED
#define APOLLO_INSTANCE_HOLDER_HPP_INCLUDED APOLLO_INSTANCE_HOLDER_HPP_INCLUDED

#include <boost/get_pointer.hpp>

namespace apollo {
namespace detail {

struct class_info;

class instance_holder {
public:
    virtual ~instance_holder() {}
    virtual void* get() = 0; // Get a pointer to the instance.
    virtual class_info const* type() const = 0; // The instance's class.
};

template <typename Ptr>
class ptr_instance_holder: public instance_holder {
public:
    ptr_instance_holder(Ptr&& ptr, class_info const& cls) // Move ptr
        : m_instance(std::move(ptr))
        , m_type(&cls)
    {}

    ptr_instance_holder(Ptr const& ptr, class_info const& cls) // Copy ptr
        : m_instance(ptr)
        , m_type(&cls)
    {}

    void* get() override
    {
        using boost::get_pointer;
        return get_pointer(m_instance);
    }

    class_info const* type() const override {
        return m_type;
    }

private:
    Ptr m_instance;
    class_info const* m_type;
};

}
} // namespace apollo::detail

#endif // APOLLO_INSTANCE_HOLDER_HPP_INCLUDED
