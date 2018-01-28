//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief new/delete allocator.
 *
 *  A shallow wrapper around `new` and `delete`. This allocator
 *  has poor performance, and cannot efficient reallocate buffers,
 *  and therefore should be used exceedingly rarely.
 *
 *  \synopsis
 *      template <typename T>
 *      struct new_delete_allocator
 *      {
 *          using value_type = T;
 *          using size_type = size_t;
 *          using pointer = value_type*;
 *          using is_always_equal = true_type;
 *
 *          new_delete_allocator() noexcept;
 *          new_delete_allocator(const new_delete_allocator&) noexcept;
 *          template <typename U> new_delete_allocator(const new_delete_allocator<U>&) noexcept;
 *          new_delete_allocator& operator=(const new_delete_allocator&) noexcept;
 *          template <typename U> new_delete_allocator& operator=(const new_delete_allocator<U>&) noexcept;
 *          ~new_delete_allocator() = default;
 *
 *          pointer allocate(size_t n;
 *          pointer reallocate(pointer ptr, size_type old_size, size_type new_delete_size, size_type count, size_type old_offset = 0, size_type new_delete_offset = 0);
 *          void deallocate(pointer p, size_t n);
 *      };
 *
 *      template <typename T, typename U>
 *      inline bool operator==(const new_delete_allocator<T>&, const new_delete_allocator<U>&) noexcept;
 *
 *      template <typename T, typename U>
 *      inline bool operator!=(const new_delete_allocator<T>&, const new_delete_allocator<U>&) noexcept
 */

#pragma once

#include <pycpp/stl/cstddef.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/memory_resource.h>
#include <pycpp/stl/new.h>
#include <pycpp/stl/type_traits.h>
#include <pycpp/stl/utility.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

/**
 *  \brief Standard new/delete allocator.
 */
template <typename T>
struct new_delete_allocator
{
    using value_type = T;
    using size_type = size_t;
    using pointer = value_type*;
    using is_always_equal = true_type;

    // Constructors
    new_delete_allocator() noexcept = default;
    new_delete_allocator(const new_delete_allocator&) noexcept = default;
    new_delete_allocator& operator=(const new_delete_allocator&) noexcept = default;
    ~new_delete_allocator() = default;

    template <typename U>
    new_delete_allocator(
        const new_delete_allocator<U>&
    )
    noexcept
    {}

    template <typename U>
    new_delete_allocator& operator=(const new_delete_allocator<U>&) noexcept
    {
        return *this;
    }

    // Allocation
    pointer
    allocate(
        size_t n
    )
    {
        return static_cast<pointer>(operator new(n * sizeof(value_type)));
    }

    void
    deallocate(
        pointer p,
        size_t n
    )
    {
        operator delete(p);
    }
};

// SPECIALIZATION
// --------------

template <typename T>
struct is_relocatable<new_delete_allocator<T>>: true_type
{};

// NON-MEMBER FUNCTIONS
// --------------------

template <typename T, typename U>
inline
bool
operator==(
    const new_delete_allocator<T>&,
    const new_delete_allocator<U>&
)
noexcept
{
    return true;
}


template <typename T, typename U>
inline
bool
operator!=(
    const new_delete_allocator<T>& x,
    const new_delete_allocator<U>& y
)
noexcept
{
    return !(x == y);
}

PYCPP_END_NAMESPACE
