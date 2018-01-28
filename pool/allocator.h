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

// REFERENCE COUNTED

// The reference-counted variants used a shared_ptr to manage
// the underlying pool. Synchronization to the pool and
// shared_ptr use similar semantics, if running single-threaded
// code, then no locks are ever introduced.
// These have the added benefits of simplifying allocator use
// with local variables, and allow inheriting an allocator
// for allocator chaining.

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
    size_t MaxSize = 0
>
class pool_allocator
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using value_type = T;
    using pointer = T*;
    using allocator_type = Allocator;
    using traits_type = allocator_traits<Allocator>;
    using is_always_equal = true_type;
    using size_type = typename traits_type::size_type;
    using mutex_type = conditional_t<thread_safe, mutex, dummy_mutex>;
    using pool_type = pool<Allocator, sizeof(T)>;
    using pair_type = compressed_pair<pool_type, mutex_type>;
    using storage_type = shared_ptr<pair_type, thread_safe>;

    template <typename U>
    struct rebind { using other = pool_allocator<U, allocator_type, thread_safe, NextSize, MaxSize>; };

    // Constructors
    pool_allocator():
        data_(PYCPP_NAMESPACE::make_shared<pair_type>(pool_type(NextSize, MaxSize)))
    {}

    pool_allocator(
        const allocator_type& alloc
    ):
        data_(PYCPP_NAMESPACE::allocate_shared<pair_type>(alloc, pool_type(NextSize, MaxSize)))
    {}

    pool_allocator(const pool_allocator&) noexcept = default;
    pool_allocator& operator=(const pool_allocator&) noexcept = default;
    pool_allocator(pool_allocator&&) noexcept = default;
    pool_allocator& operator=(pool_allocator&&) noexcept = default;

    template <typename U>
    pool_allocator(
        const typename pool_allocator::template rebind<U>&
    ):
        pool_allocator()
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
        lock_guard<mutex_type> lock(mu());
        pointer p = static_cast<pointer>(pool_impl().ordered_allocate(n));
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
        lock_guard<mutex_type> lock(mu());
        pool_impl().ordered_deallocate(p, n);
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

private:
    storage_type data_;

    pool_type&
    pool_impl()
    noexcept
    {
        return get<0>(*data_);
    }

    mutex_type&
    mu()
    noexcept
    {
        return get<1>(*data_);
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
    size_t MaxSize = 0
>
class fast_pool_allocator
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using value_type = T;
    using pointer = T*;
    using allocator_type = Allocator;
    using traits_type = allocator_traits<Allocator>;
    using is_always_equal = true_type;
    using size_type = typename traits_type::size_type;
    using mutex_type = conditional_t<thread_safe, mutex, dummy_mutex>;
    using pool_type = pool<Allocator, sizeof(T)>;
    using pair_type = compressed_pair<pool_type, mutex_type>;
    using storage_type = shared_ptr<pair_type, thread_safe>;

    template <typename U>
    struct rebind { using other = fast_pool_allocator<U, allocator_type, thread_safe, NextSize, MaxSize>; };

    // Constructors
    fast_pool_allocator():
        data_(PYCPP_NAMESPACE::make_shared<pair_type>(pool_type(NextSize, MaxSize)))
    {}

    fast_pool_allocator(
        const allocator_type& alloc
    ):
        data_(PYCPP_NAMESPACE::allocate_shared<pair_type>(alloc, pool_type(NextSize, MaxSize)))
    {}

    fast_pool_allocator(const fast_pool_allocator&) noexcept = default;
    fast_pool_allocator& operator=(const fast_pool_allocator&) noexcept = default;
    fast_pool_allocator(fast_pool_allocator&&) noexcept = default;
    fast_pool_allocator& operator=(fast_pool_allocator&&) noexcept = default;

    template <typename U>
    fast_pool_allocator(
        const typename fast_pool_allocator::template rebind<U>&
    ):
        fast_pool_allocator()
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
        lock_guard<mutex_type> lock(mu());
        pointer p;
        if (n == 1) {
            p = static_cast<pointer>(pool_impl().allocate(n));
        } else {
            p = static_cast<pointer>(pool_impl().ordered_allocate(n));
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
        lock_guard<mutex_type> lock(mu());
        if (n == 1) {
            pool_impl().deallocate(p);
        } else {
            pool_impl().deallocate(p, n);
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

private:
    storage_type data_;

    pool_type&
    pool_impl()
    noexcept
    {
        return *get<0>(*data_);
    }

    mutex_type&
    mu()
    noexcept
    {
        return get<1>(*data_);
    }
};

// SINGLETON

// These single pattern APIs are for stateful allocators
// that must not have any internal state: they rely
// solely on the global, yet use a static resource
// that can only be accessed through the global
// allocator. This may be useful for some optimizations.
// Otherwise, I highly recommend using the non-singleton variants.

// Empty class to use as a tag for the singleton_pool_allocator.
struct singleton_pool_allocator_tag
{};

// Empty class to use as a tag for the fast_singleton_pool_allocator
struct fast_singleton_pool_allocator_tag
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
    typename Tag = singleton_pool_allocator_tag
>
class singleton_pool_allocator
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
    struct rebind { using other = singleton_pool_allocator<U, allocator_type, thread_safe, NextSize, MaxSize, tag_type>; };

    // Constructors
    constexpr
    singleton_pool_allocator()
    noexcept
    {}

    singleton_pool_allocator(const singleton_pool_allocator&) noexcept = default;
    singleton_pool_allocator& operator=(const singleton_pool_allocator&) noexcept = default;
    singleton_pool_allocator(singleton_pool_allocator&&) noexcept = default;
    singleton_pool_allocator& operator=(singleton_pool_allocator&&) noexcept = default;

    template <typename U>
    constexpr
    singleton_pool_allocator(
        const typename singleton_pool_allocator::template rebind<U>&
    )
    noexcept
    {}

    template <typename U>
    singleton_pool_allocator&
    operator=(
        const typename singleton_pool_allocator::template rebind<U>&
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
        const singleton_pool_allocator&,
        const singleton_pool_allocator&
    )
    noexcept
    {
        return true;
    }

    friend
    bool
    operator!=(
        const singleton_pool_allocator& x,
        const singleton_pool_allocator& y
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
    typename Tag = fast_singleton_pool_allocator_tag
>
class fast_singleton_pool_allocator
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
    struct rebind { using other = fast_singleton_pool_allocator<U, allocator_type, thread_safe, NextSize, MaxSize, tag_type>; };

    // Constructors
    constexpr
    fast_singleton_pool_allocator()
    noexcept
    {}

    fast_singleton_pool_allocator(const fast_singleton_pool_allocator&) noexcept = default;
    fast_singleton_pool_allocator& operator=(const fast_singleton_pool_allocator&) noexcept = default;
    fast_singleton_pool_allocator(fast_singleton_pool_allocator&&) noexcept = default;
    fast_singleton_pool_allocator& operator=(fast_singleton_pool_allocator&&) noexcept = default;

    template <typename U>
    constexpr
    fast_singleton_pool_allocator(
        const typename fast_singleton_pool_allocator::template rebind<U>&
    )
    noexcept
    {}

    template <typename U>
    fast_singleton_pool_allocator&
    operator=(
        const typename fast_singleton_pool_allocator::template rebind<U>&
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
        const fast_singleton_pool_allocator&,
        const fast_singleton_pool_allocator&
    )
    noexcept
    {
        return true;
    }

    friend
    bool
    operator!=(
        const fast_singleton_pool_allocator& x,
        const fast_singleton_pool_allocator& y
    )
    noexcept
    {
        return !(x == y);
    }
};

PYCPP_END_NAMESPACE
