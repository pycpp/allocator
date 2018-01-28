//  :copyright: (c) 2000, 2001 Stephen Cleary.
//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: Boost, see licenses/boost.md for more details.

#pragma once

#include <pycpp/config.h>
#include <pycpp/alloctor/pool/singleton.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/new.h>
#include <pycpp/stl/type_traits.h>
// TODO: singleton_pool

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------

/**
 *  \brief Allocator based on an underlying pool.
 *
 *  \warning The underlying singleton_pool used by the this allocator
 *  constructs a pool instance that **is never freed**. This means that
 *  memory allocated by the allocator can be still used after main() has
 *  completed, but may mean that some memory checking programs
 *  will complain about leaks.
  */
template <typename T, typename Allocator, typename Mutex>
class pool_allocator
{
public:
    template <typename U>
    struct rebind { using other = pool_allocator<U, Allocator, Mutex>; };

    using value_type = T;
    using pointer = T*;
    using mutex_type = Mutex;
    using is_always_equal = true_type;
    // TODO: let allocator_traits handle this...

//    pool_allocator()
//    { /*! Results in default construction of the underlying singleton_pool IFF an
//       instance of this allocator is constructed during global initialization (
//         required to ensure construction of singleton_pool IFF an
//         instance of this allocator is constructed during global
//         initialization. See ticket #2359 for a complete explanation at
//         http://svn.boost.org/trac/boost/ticket/2359) .
//       */
//      singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
//                     NextSize, MaxSize>::is_from(0);
//    }
//
//    // default copy constructor.
//
//    // default assignment operator.
//
//    // not explicit, mimicking std::allocator [20.4.1]
//    template <typename U>
//    pool_allocator(const pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> &)
//    { /*! Results in the default construction of the underlying singleton_pool, this
//         is required to ensure construction of singleton_pool IFF an
//         instance of this allocator is constructed during global
//         initialization. See ticket #2359 for a complete explanation
//         at http://svn.boost.org/trac/boost/ticket/2359 .
//       */
//      singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
//                     NextSize, MaxSize>::is_from(0);
//    }

    // TODO: implement..
    bool
    operator==(const pool_allocator&)
    const
    {
        return true;
    }

    pointer
    allocate(
        size_type n,
        const void* hint = nullptr
    )
    {
//      const pointer ret = static_cast<pointer>(
//          singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
//              NextSize, MaxSize>::ordered_malloc(n) );
//      if ((ret == 0) && n)
//        boost::throw_exception(std::bad_alloc());
//      return ret;
    }

    void
    deallocate(
        pointer ptr,
        size_type n
    )
    {
        // TODO: implement
//        singleton_pool<pool_allocator_tag, sizeof(T), UserAllocator, Mutex,
//          NextSize, MaxSize>::ordered_free(ptr, n);
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
template <typename T, typename Allocator, typename Mutex>
class pool_allocator
{
public:
    template <typename U>
    struct rebind { using other = pool_allocator<U, Allocator, Mutex>; };

    using value_type = T;
    using pointer = T*;
    using mutex_type = Mutex;
    using is_always_equal = true_type;
    // TODO: let allocator_traits handle this...

//    fast_pool_allocator()
//    {
//      //! Ensures construction of the underlying singleton_pool IFF an
//      //! instance of this allocator is constructed during global
//      //! initialization. See ticket #2359 for a complete explanation
//      //! at http://svn.boost.org/trac/boost/ticket/2359 .
//      singleton_pool<fast_pool_allocator_tag, sizeof(T),
//                     UserAllocator, Mutex, NextSize, MaxSize>::is_from(0);
//    }

//    template <typename U>
//    fast_pool_allocator(
//        const fast_pool_allocator<U, UserAllocator, Mutex, NextSize, MaxSize> &)
//    {
//      //! Ensures construction of the underlying singleton_pool IFF an
//      //! instance of this allocator is constructed during global
//      //! initialization. See ticket #2359 for a complete explanation
//      //! at http://svn.boost.org/trac/boost/ticket/2359 .
//      singleton_pool<fast_pool_allocator_tag, sizeof(T),
//                     UserAllocator, Mutex, NextSize, MaxSize>::is_from(0);
//    }

    // TODO: implement..
    bool
    operator==(const pool_allocator&)
    const
    {
        return true;
    }

    pointer
    allocate(
        size_type n,
        const void* hint = nullptr
    )
    {
        // TODO: implement...
    }

    void
    deallocate(
        pointer ptr,
        size_type n
    )
    {
        // TODO: implement
    }
};

PYCPP_END_NAMESPACE
