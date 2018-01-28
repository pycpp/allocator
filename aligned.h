//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Aligned C-runtime allocator.
 *
 *  A shallow wrapper around `aligned_alloc`, and `aligned_free`. This
 *  allocator should provide performance improvements for old x86 hardware,
 *  however, that enhancement should disappear for more recent harware.
 *  Furthermore, some hardware requires type-alignment on specific
 *  boundaries, so, for example, a buffer allocated for ints may not
 *  be safely cast to a buffer of uint64_t.
 *
 *  See:
 *      https://lemire.me/blog/2012/05/31/data-alignment-for-speed-myth-or-reality/
 *
 *  \synopsis
 *      template <typename T>
 *      struct aligned_allocator
 *      {
 *          using value_type = T;
 *          using is_always_equal = true_type;
 *
 *          aligned_allocator() noexcept;
 *          aligned_allocator(const aligned_allocator&) noexcept;
 *          template <typename U> aligned_allocator(const aligned_allocator<U>&) noexcept;
 *          aligned_allocator& operator=(const aligned_allocator&) noexcept;
 *          template <typename U> aligned_allocator& operator=(const aligned_allocator<U>&) noexcept;
 *          ~aligned_allocator() = default;
 *
 *          value_type* allocate(size_t n);
 *          pointer reallocate(pointer ptr, size_type old_size, size_type new_size, size_type count, size_type old_offset = 0, size_type new_offset = 0);
 *          void deallocate(value_type* p, size_t n);
 *      };
 *
 *      using aligned_resource = resource_adaptor<aligned_allocator<byte>>;
 *
 *      template <typename T, typename U>
 *      inline bool operator==(const aligned_allocator<T>&, const aligned_allocator<U>&) noexcept;
 *
 *      template <typename T, typename U>
 *      inline bool operator!=(const aligned_allocator<T>&, const aligned_allocator<U>&) noexcept
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
 *  \brief Type-aligned C-runtime memory allocator.
 */
template <typename T>
struct aligned_allocator
{
    using value_type = T;
    using size_type = size_t;
    using pointer = value_type*;
    using is_always_equal = true_type;

    // Constructors
    aligned_allocator() noexcept = default;
    aligned_allocator(const aligned_allocator&) noexcept = default;
    aligned_allocator& operator=(const aligned_allocator&) noexcept = default;
    ~aligned_allocator() = default;

    template <typename U>
    aligned_allocator(
        const aligned_allocator<U>&
    )
    noexcept
    {}

    template <typename U>
    aligned_allocator& operator=(const aligned_allocator<U>&) noexcept
    {
        return *this;
    }

    // Allocation
    pointer
    allocate(
        size_t n
    )
    {
        void* p = aligned_alloc(alignof(value_type), n * sizeof(value_type));
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
        aligned_free(p);
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
            size_t old_bytes = old_size * sizeof(value_type);
            size_t new_bytes = new_size * sizeof(value_type);
            void* p = aligned_realloc(static_cast<void*>(ptr), alignof(value_type), old_bytes, new_bytes);
            if (p == nullptr) {
                throw bad_alloc();
            }
            return static_cast<pointer>(p);
        } else {
            using traits = allocator_traits<aligned_allocator>;
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
        using traits = allocator_traits<aligned_allocator>;
        return traits::reallocate_move(*this, ptr, old_size, new_size, count, old_offset, new_offset);
    }
};

namespace pmr
{
// ALIAS
// -----

using aligned_resource = resource_adaptor<aligned_allocator<byte>>;

}   /* pmr */

// SPECIALIZATION
// --------------

template <typename T>
struct is_relocatable<aligned_allocator<T>>: true_type
{};

// NON-MEMBER FUNCTIONS
// --------------------

template <typename T, typename U>
inline
bool
operator==(
    const aligned_allocator<T>&,
    const aligned_allocator<U>&
)
noexcept
{
    return true;
}


template <typename T, typename U>
inline
bool
operator!=(
    const aligned_allocator<T>& x,
    const aligned_allocator<U>& y
)
noexcept
{
    return !(x == y);
}

PYCPP_END_NAMESPACE
