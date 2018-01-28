//  :copyright: (c) 2000, 2001 Stephen Cleary.
//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: Boost, see licenses/boost.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Fast segregated storage.
 *
 *  Singleton pattern with custom tags to allow multiple types
 *  to access the same pool.
 */

#pragma once

#include <pycpp/allocator/pool/pool.h>
#include <pycpp/stl/atomic.h>
#include <pycpp/stl/cstddef.h>
#include <pycpp/stl/mutex.h>
#include <pycpp/stl/type_traits.h>
#include <pycpp/stl/container/compressed_pair.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

template <
    typename Allocator,
    typename Tag,
    size_t RequestedSize,
    size_t NextSize,
    size_t MaxSize,
    bool ThreadSafe
>
class singleton_pool
{
public:
    static constexpr bool thread_safe = ThreadSafe;

    using tag_type = Tag;
    using allocator_type = Allocator;
    using traits_type = allocator_traits<allocator_type>;
    using size_type = typename traits_type::size_type;
    using pool_type = pool<Allocator, RequestedSize>;
    using mutex_type = conditional_t<thread_safe, mutex, dummy_mutex>;
    using storage_type = aligned_storage_t<sizeof(pool_type), alignof(pool_type)>;
    using data_type = compressed_pair<storage_type, mutex_type>;

    // Constructors
    singleton_pool() = delete;

    // API
    static
    void*
    allocate()
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        return p.allocate();
    }

    static
    void*
    ordered_allocate()
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        return p.ordered_allocate();
    }

    static
    void*
    ordered_allocate(
        size_type n
    )
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        return p.ordered_allocate(n);
    }

    static
    bool
    is_from(
        void* ptr
    )
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        return p.is_from(ptr);
    }

    static
    void
    deallocate(
        void* ptr
    )
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        p.deallocate(ptr);
    }

    static
    void
    ordered_deallocate(
        void* ptr
    )
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        p.ordered_deallocate(ptr);
    }

    static
    void
    deallocate(
        void* ptr,
        size_type n
    )
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        p.deallocate(ptr, n);
    }

    static
    void
    ordered_free(
        void* ptr,
        size_type n
    )
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        p.ordered_free(ptr, n);
    }

    static
    bool
    release_memory()
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        return p.release_memory();
    }

    static
    bool
    purge_memory()
    {
        pool_type& p = get_pool();
        auto lock = lock_guard<mutex_type>(mu());
        return p.purge_memory();
    }

private:
    static data_type data_;

    // Pool
    // This does not actually force initialization of the pool.
    // Only use from `get_pool()`.
    static
    storage_type&
    pool_impl()
    noexcept
    {
        return reinterpret_cast<pool_type&>(get<0>(data_));
    }

    // Mutex
    static
    mutex_type&
    mu()
    {
        return get<1>(data_);
    }

    // Initializer
    static
    pool_type&
    get_pool()
    {
        return get_pool(bool_constant<thread_safe>());
    }

    static
    pool_type&
    get_pool(
        true_type
    )
    {
        static atomic<bool> initalized = ATOMIC_VAR_INIT(false);
        if (!initalized.load()) {
            lock_guard<mutex> lock(mu());
            if (!initalized.load()) {
                new (&pool_impl()) pool_type(NextSize, MaxSize);
                initalized.store(true, memory_order_release);
            }
        }

        return pool_impl();
    }

    static
    pool_type&
    get_pool(
        false_type
    )
    {
        static bool initalized = false;
        if (!initalized) {
            initalized = true;
            new (&pool_impl()) pool_type(NextSize, MaxSize);
        }

        return pool_impl();
    }
};


template <
    typename Allocator,
    typename Tag,
    size_t RequestedSize,
    size_t NextSize,
    size_t MaxSize,
    bool ThreadSafe
>
typename singleton_pool<Allocator, Tag, RequestedSize, NextSize, MaxSize, ThreadSafe>::data_type
singleton_pool<Allocator, Tag, RequestedSize, NextSize, MaxSize, ThreadSafe>::data_;

PYCPP_END_NAMESPACE
