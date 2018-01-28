//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Null memory resource allocator.
 *
 *  A memory allocator that does not allocate anything..
 *
 *  \synopsis
 *      template <typename T>
 *      struct null_memory_allocator
 *      {
 *          using value_type = T;
 *          using is_always_equal = true_type;
 *
 *          null_memory_allocator() noexcept;
 *          null_memory_allocator(const null_memory_allocator&) noexcept;
 *          template <typename U> null_memory_allocator(const null_memory_allocator<U>&) noexcept;
 *          null_memory_allocator& operator=(const null_memory_allocator&) noexcept;
 *          template <typename U> null_memory_allocator& operator=(const null_memory_allocator<U>&) noexcept;
 *          ~null_memory_allocator() = default;
 *
 *          value_type* allocate(size_t n);
 *          void deallocate(value_type* p, size_t n);
 *      };
 *
 *      using null_memory_resource = resource_adaptor<null_memory_allocator<byte>>;
 *
 *      template <typename T, typename U>
 *      inline bool operator==(const null_memory_allocator<T>&, const null_memory_allocator<U>&) noexcept;
 *
 *      template <typename T, typename U>
 *      inline bool operator!=(const null_memory_allocator<T>&, const null_memory_allocator<U>&) noexcept
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
struct null_memory_allocator
{
    using value_type = T;
    using size_type = size_t;
    using pointer = value_type*;
    using is_always_equal = true_type;

    // Constructors
    null_memory_allocator() noexcept = default;
    null_memory_allocator(const null_memory_allocator&) noexcept = default;
    null_memory_allocator& operator=(const null_memory_allocator&) noexcept = default;
    ~null_memory_allocator() = default;

    template <typename U>
    null_memory_allocator(
        const null_memory_allocator<U>&
    )
    noexcept
    {}

    template <typename U>
    null_memory_allocator& operator=(const null_memory_allocator<U>&) noexcept
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

// SPECIALIZATION
// --------------

template <typename T>
struct is_relocatable<null_memory_allocator<T>>: true_type
{};

// NON-MEMBER FUNCTIONS
// --------------------

template <typename T, typename U>
inline
bool
operator==(
    const null_memory_allocator<T>&,
    const null_memory_allocator<U>&
)
noexcept
{
    return true;
}


template <typename T, typename U>
inline
bool
operator!=(
    const null_memory_allocator<T>& x,
    const null_memory_allocator<U>& y
)
noexcept
{
    return !(x == y);
}

PYCPP_END_NAMESPACE
