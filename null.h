//  :copyright: (c) 2017 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Null resource allocator.
 *
 *  A memory allocator that does not allocate anything..
 *
 *  \synopsis
 *      template <typename T>
 *      struct null_allocator
 *      {
 *          using value_type = T;
 *          using is_always_equal = true_type;
 *
 *          null_allocator() noexcept;
 *          null_allocator(const null_allocator&) noexcept;
 *          template <typename U> null_allocator(const null_allocator<U>&) noexcept;
 *          null_allocator& operator=(const null_allocator&) noexcept;
 *          template <typename U> null_allocator& operator=(const null_allocator<U>&) noexcept;
 *          ~null_allocator() = default;
 *
 *          value_type* allocate(size_t n);
 *          void deallocate(value_type* p, size_t n);
 *      };
 *
 *      using null_resource = resource_adaptor<null_allocator<byte>>;
 *
 *      template <typename T, typename U>
 *      inline bool operator==(const null_allocator<T>&, const null_allocator<U>&) noexcept;
 *
 *      template <typename T, typename U>
 *      inline bool operator!=(const null_allocator<T>&, const null_allocator<U>&) noexcept
 */

#pragma once

#include <pycpp/stl/cassert.h>
#include <pycpp/stl/cstddef.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/memory_resource.h>
#include <pycpp/stl/new.h>
#include <pycpp/stl/type_traits.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

/**
 *  \brief Null resource allocator.
 */
template <typename T>
struct null_allocator
{
    using value_type = T;
    using size_type = size_t;
    using pointer = value_type*;
    using is_always_equal = true_type;

    // Constructors
    null_allocator() noexcept = default;
    null_allocator(const null_allocator&) noexcept = default;
    null_allocator& operator=(const null_allocator&) noexcept = default;
    ~null_allocator() = default;

    template <typename U>
    null_allocator(
        const null_allocator<U>&
    )
    noexcept
    {}

    template <typename U>
    null_allocator& operator=(const null_allocator<U>&) noexcept
    {
        return *this;
    }

    // Allocation
    pointer
    allocate(
        size_t n
    )
    {
        throw bad_alloc();
    }

    void
    deallocate(
        pointer p,
        size_t n
    )
    {
        assert(p == nullptr || n == 0);
    }
};

namespace pmr
{
// ALIAS
// -----

using null_resource = resource_adaptor<null_allocator<byte>>;

}   /* pmr */

// SPECIALIZATION
// --------------

template <typename T>
struct is_relocatable<null_allocator<T>>: true_type
{};

// NON-MEMBER FUNCTIONS
// --------------------

template <typename T, typename U>
inline
bool
operator==(
    const null_allocator<T>&,
    const null_allocator<U>&
)
noexcept
{
    return true;
}


template <typename T, typename U>
inline
bool
operator!=(
    const null_allocator<T>& x,
    const null_allocator<U>& y
)
noexcept
{
    return !(x == y);
}

PYCPP_END_NAMESPACE
