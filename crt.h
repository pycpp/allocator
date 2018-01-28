//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief C-runtime allocator.
 *
 *  A shallow wrapper around `malloc`, `realloc`, and `free`. This allocator
 *  has poor performance, and therefore should be used sparingly.
 *
 *  \synopsis
 *      template <typename T>
 *      struct crt_allocator
 *      {
 *          using value_type = T;
 *          using size_type = size_t;
 *          using pointer = value_type*;
 *          using is_always_equal = true_type;
 *
 *          crt_allocator() noexcept;
 *          crt_allocator(const crt_allocator&) noexcept;
 *          template <typename U> crt_allocator(const crt_allocator<U>&) noexcept;
 *          crt_allocator& operator=(const crt_allocator&) noexcept;
 *          template <typename U> crt_allocator& operator=(const crt_allocator<U>&) noexcept;
 *          ~crt_allocator() = default;
 *
 *          pointer allocate(size_t n;
 *          pointer reallocate(pointer ptr, size_type old_size, size_type new_size, size_type count, size_type old_offset = 0, size_type new_offset = 0);
 *          void deallocate(pointer p, size_t n);
 *      };
 *
 *      using crt_resource = resource_adaptor<crt_allocator<byte>>;
 *
 *      template <typename T, typename U>
 *      inline bool operator==(const crt_allocator<T>&, const crt_allocator<U>&) noexcept;
 *
 *      template <typename T, typename U>
 *      inline bool operator!=(const crt_allocator<T>&, const crt_allocator<U>&) noexcept
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
 *  \brief Standard C-runtime memory allocator.
 */
template <typename T>
struct crt_allocator
{
    using value_type = T;
    using size_type = size_t;
    using pointer = value_type*;
    using is_always_equal = true_type;

    // Constructors
    crt_allocator() noexcept = default;
    crt_allocator(const crt_allocator&) noexcept = default;
    crt_allocator& operator=(const crt_allocator&) noexcept = default;
    ~crt_allocator() = default;

    template <typename U>
    crt_allocator(
        const crt_allocator<U>&
    )
    noexcept
    {}

    template <typename U>
    crt_allocator& operator=(const crt_allocator<U>&) noexcept
    {
        return *this;
    }

    // Allocation
    pointer
    allocate(
        size_t n
    )
    {
        void* p = malloc(n * sizeof(value_type));
        if (p == nullptr) {
            throw bad_alloc();
        }
        return static_cast<pointer>(p);
    }

    pointer
    reallocate(
        pointer ptr,
        size_type old_size,
        size_type new_size,
        size_type count,
        size_type old_offset = 0,
        size_type new_offset = 0
    )
    {
        using relocatable = bool_constant<is_relocatable<value_type>::value>;
        return reallocate_impl(ptr, old_size, new_size, count, old_offset, new_offset, relocatable());
    }

    void
    deallocate(
        pointer p,
        size_t n
    )
    {
        free(p);
    }

private:
    pointer
    reallocate_impl(
        pointer ptr,
        size_type old_size,
        size_type new_size,
        size_type count,
        size_type old_offset,
        size_type new_offset,
        true_type
    )
    {
        if (old_offset == 0 && new_offset == 0) {
            // optimize using realloc
            // we ignore the **count** variable, but that shouldn't matter,
            // since they'll be treated as raw bytes anyway.
            void* p = realloc(static_cast<void*>(ptr), new_size * sizeof(value_type));
            if (p == nullptr) {
                throw bad_alloc();
            }
            return static_cast<pointer>(p);
        } else {
            using traits = allocator_traits<crt_allocator>;
            return traits::reallocate_relocate(*this, ptr, old_size, new_size, count, old_offset, new_offset);
        }
    }

    pointer
    reallocate_impl(
        pointer ptr,
        size_type old_size,
        size_type new_size,
        size_type count,
        size_type old_offset,
        size_type new_offset,
        false_type
    )
    {
        // use the default implementation in allocator traits if not relocatable
        using traits = allocator_traits<crt_allocator>;
        return traits::reallocate_move(*this, ptr, old_size, new_size, count, old_offset, new_offset);
    }
};

namespace pmr
{
// ALIAS
// -----

using crt_resource = resource_adaptor<crt_allocator<byte>>;

}   /* pmr */

// SPECIALIZATION
// --------------

template <typename T>
struct is_relocatable<crt_allocator<T>>: true_type
{};

// NON-MEMBER FUNCTIONS
// --------------------

template <typename T, typename U>
inline
bool
operator==(
    const crt_allocator<T>&,
    const crt_allocator<U>&
)
noexcept
{
    return true;
}


template <typename T, typename U>
inline
bool
operator!=(
    const crt_allocator<T>& x,
    const crt_allocator<U>& y
)
noexcept
{
    return !(x == y);
}

PYCPP_END_NAMESPACE
