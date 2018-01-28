//  :copyright: (c) 2000, 2001 Stephen Cleary.
//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: Boost, see licenses/boost.md for more details.

#pragma once

#include <pycpp/config.h>
#include <pycpp/allocator/pool/singleton.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/new.h>
#include <pycpp/stl/type_traits.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

// Empty class to use as a tag for the pool_allocator.
struct pool_allocator_tag
{};

// Empty class to use as a tag for the fast_pool_allocator
struct fast_pool_allocator_tag
{};

/**
 *  \brief Allocator based on an underlying pool.
 *
 *  \warning The underlying singleton_pool used by the this allocator
 *  constructs a pool instance that **is never freed**. This means that
 *  memory allocated by the allocator can be still used after main() has
 *  completed, but may mean that some memory checking programs
 *  will complain about leaks.
  */
template <
    typename T,
    typename Allocator= allocator<T>,
    bool ThreadSafe = true,
    size_t NextSize = 32,
    size_t MaxSize = 0,
    typename Tag = pool_allocator_tag
>
class pool_allocator
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using value_type = T;
    using pointer = T*;
    using tag_type = Tag;
    using allocator_type = Allocator;
    using traits_type = allocator_traits<Allocator>;
    using is_always_equal = true_type;
    using size_type = typename traits_type::size_type;
    using singleton_type = singleton_pool<
        Allocator,
        Tag,
        sizeof(T),
        NextSize,
        MaxSize,
        ThreadSafe
    >;

    template <typename U>
    struct rebind { using other = pool_allocator<U, allocator_type, thread_safe, NextSize, MaxSize, tag_type>; };

    // Constructors
    constexpr
    pool_allocator()
    noexcept
    {}

    pool_allocator(const pool_allocator&) noexcept = default;
    pool_allocator& operator=(const pool_allocator&) noexcept = default;
    pool_allocator(pool_allocator&&) noexcept = default;
    pool_allocator& operator=(pool_allocator&&) noexcept = default;

    template <typename U>
    constexpr
    pool_allocator(
        const typename pool_allocator::template rebind<U>&
    )
    noexcept
    {}

    template <typename U>
    pool_allocator&
    operator=(
        const typename pool_allocator::template rebind<U>&
    )
    noexcept
    {
        return *this;
    }

    // Allocation
    pointer
    allocate(
        size_type n
    )
    {
        pointer p = static_cast<pointer>(singleton_type::ordered_allocate(n));
        if (p == nullptr && n != 0) {
            throw bad_alloc();
        }
        return p;
    }

    void
    deallocate(
        pointer p,
        size_type n
    )
    {
        singleton_type::ordered_deallocate(p, n);
    }

    // Relative operators
    friend
    bool
    operator==(
        const pool_allocator&,
        const pool_allocator&
    )
    noexcept
    {
        return true;
    }

    friend
    bool
    operator!=(
        const pool_allocator& x,
        const pool_allocator& y
    )
    noexcept
    {
        return !(x == y);
    }
};


/**
 *  \brief Fast allocator based on an underlying pool.
 *
 *  Optimized for the construction of single objects.
 *
 *  \warning The underlying singleton_pool used by the this allocator
 *  constructs a pool instance that **is never freed**. This means that
 *  memory allocated by the allocator can be still used after main() has
 *  completed, but may mean that some memory checking programs
 *  will complain about leaks.
  */
template <
    typename T,
    typename Allocator= allocator<T>,
    bool ThreadSafe = true,
    size_t NextSize = 32,
    size_t MaxSize = 0,
    typename Tag = fast_pool_allocator_tag
>
class fast_pool_allocator
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using value_type = T;
    using pointer = T*;
    using tag_type = Tag;
    using allocator_type = Allocator;
    using traits_type = allocator_traits<Allocator>;
    using is_always_equal = true_type;
    using size_type = typename traits_type::size_type;
    using singleton_type = singleton_pool<
        Allocator,
        Tag,
        sizeof(T),
        NextSize,
        MaxSize,
        ThreadSafe
    >;

    template <typename U>
    struct rebind { using other = fast_pool_allocator<U, allocator_type, thread_safe, NextSize, MaxSize, tag_type>; };

    // Constructors
    constexpr
    fast_pool_allocator()
    noexcept
    {}

    fast_pool_allocator(const fast_pool_allocator&) noexcept = default;
    fast_pool_allocator& operator=(const fast_pool_allocator&) noexcept = default;
    fast_pool_allocator(fast_pool_allocator&&) noexcept = default;
    fast_pool_allocator& operator=(fast_pool_allocator&&) noexcept = default;

    template <typename U>
    constexpr
    fast_pool_allocator(
        const typename fast_pool_allocator::template rebind<U>&
    )
    noexcept
    {}

    template <typename U>
    fast_pool_allocator&
    operator=(
        const typename fast_pool_allocator::template rebind<U>&
    )
    noexcept
    {
        return *this;
    }

    // Allocation
    pointer
    allocate(
        size_type n
    )
    {
        pointer p;
        if (n == 1) {
            p = static_cast<pointer>(singleton_type::allocate(n));
        } else {
            p = static_cast<pointer>(singleton_type::ordered_allocate(n));
        }
        if (p == nullptr && n != 0) {
            throw bad_alloc();
        }
        return p;
    }

    void
    deallocate(
        pointer p,
        size_type n
    )
    {
        if (n == 1) {
            singleton_type::deallocate(p);
        } else {
            singleton_type::deallocate(p, n);
        }
    }

    // Relative operators
    friend
    bool
    operator==(
        const fast_pool_allocator&,
        const fast_pool_allocator&
    )
    noexcept
    {
        return true;
    }

    friend
    bool
    operator!=(
        const fast_pool_allocator& x,
        const fast_pool_allocator& y
    )
    noexcept
    {
        return !(x == y);
    }
};

PYCPP_END_NAMESPACE
