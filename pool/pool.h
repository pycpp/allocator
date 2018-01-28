//  :copyright: (c) 2000, 2001 Stephen Cleary.
//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Fast pool allocator.
 *
 *  Fork of Boost.Pool, with notable differences from the Boost
 *  implementation. Boost.Pool requires a stateless user allocator,
 *  preventing chained allocators. This fork allows stateful allocator,
 *  and uses empty-base-optimization to avoid overhead for stateless
 *  allocators.
 *
 *  In addition, the singleton model has been entirely re-worked,
 *  allowing both thread-safe and single-threaded allocator
 *  use.
 */

#pragma once

#include <pycpp/config.h>
#include <pycpp/allocator/pool/segregated_storage.h>
#include <pycpp/stl/algorithm.h>
#include <pycpp/stl/cassert.h>
#include <pycpp/stl/cstddef.h>
#include <pycpp/stl/cstdlib.h>
#include <pycpp/stl/exception.h>
#include <pycpp/stl/functional.h>
#include <pycpp/stl/memory.h>
#include <pycpp/stl/numeric.h>
#include <pycpp/stl/new.h>

PYCPP_BEGIN_NAMESPACE


// FUNCTIONS
// ---------

template <typename T>
inline static
T
fast_ceil_division(
    T numerator,
    T denominator
)
{
    bool need_ceil = static_cast<bool>(numerator % denominator);
    return numerator / denominator + need_ceil;
}

// OBJECTS
// -------

template <typename SizeType, typename VoidPtr>
class pod_ptr
{
public:
    using size_type = SizeType;

    static constexpr size_type min_alloc_size = lcm(sizeof(VoidPtr), sizeof(size_type));

    // Constructors
    constexpr
    pod_ptr(
        char* ptr,
        size_type size
    ):
        ptr_(ptr),
        sz_(size)
    {}

    constexpr
    pod_ptr():
        ptr_(nullptr),
        sz_(0)
    {}

    // Properties
    bool
    valid()
    const
    {
        return begin() != nullptr;
    }

    size_type
    total_size()
    const
    {
        return sz_;
    }

    size_type
    element_size()
    const
    {
        return static_cast<size_type>(sz_ - sizeof(size_type) -min_alloc_size);
    }

    size_type&
    next_size()
    const
    {
        return *(static_cast<size_type*>(static_cast<void*>((ptr_next_size()))));
    }

    // Modifiers
    void
    invalidate()
    {
        begin() = nullptr;
    }

    // Iterators
    char*&
    begin()
    {
        return ptr_;
    }

    char*
    begin()
    const
    {
        return ptr_;
    }

    char*
    end()
    const
    {
        return ptr_next_ptr();
    }

    char*&
    next_ptr()
    const
    {
        return *(static_cast<char**>(static_cast<void*>(ptr_next_ptr())));
    }

    pod_ptr
    next()
    const
    {
        return pod_ptr(next_ptr(), next_size());
    }

    void
    next(
      const pod_ptr& x
    )
    const
    {
        next_ptr() = x.begin();
        next_size() = x.total_size();
    }

private:
    char* ptr_;
    size_type sz_;

    char*
    ptr_next_size()
    const
    {
        return (ptr_ + sz_ - sizeof(size_type));
    }

    char*
    ptr_next_ptr()
    const
    {
        return (ptr_next_size() - min_alloc_size);
    }
};

/**
 *  \brief A fast memory allocator that with proper alignment.
 *
 *  Whenever the pool requires memory from the system, it will request
 *  it from the underlying allocator. The amount requested is determined
 *  using a doubling algorithm; that is, each time more system memory
 *  is allocated, the amount of system memory requested is doubled.
 *
 *  Users may control the doubling algorithm by using the following
 *  extensions:
 *
 *  Users may pass an additional constructor parameter to pool.
 *  This parameter is of type size_type,
 *  and is the number of chunks to request from the system
 *  the first time that object needs to allocate system memory.
 *  The default is 32. This parameter may not be 0.
 *
 *  Users may also pass an optional third parameter to pool's
 *  constructor.  This parameter is of type size_type,
 *  and sets a maximum size for allocated chunks.  When this
 *  parameter takes the default value of 0, then there is no upper
 *  limit on chunk size.
 *
 *  Finally, if the doubling algorithm results in no memory
 *  being allocated, the pool will backtrack just once, halving
 *  the chunk size and trying again.
 *
 *  There are essentially two ways to use class pool: the client can
 *  call `allocate()` and `deallocate()` to allocate and free single
 *  chunks of memory, this is the most efficient way to use a pool,
 *  but does not allow for the efficient allocation of arrays of chunks.
 *  Alternatively, the client may call `ordered_allocate()` and
 *  `ordered_deallocate()`, in which case the free list is maintained
 *  in an ordered state, and efficient allocation of arrays of chunks
 *  are possible.  However, this latter option can suffer from poor
 *  performance when large numbers of allocations are performed.
 *
 */
template <typename Allocator, size_t RequestedSize>
class pool:
    protected segregated_storage<typename allocator_traits<Allocator>::size_type>,
    protected Allocator
{
public:
    using allocator_type = Allocator;
    using traits_type = allocator_traits<allocator_type>;
    using size_type = typename traits_type::size_type;
    using difference_type = typename traits_type::difference_type;
    using void_pointer = typename traits_type::void_pointer;
    using storage_type = segregated_storage<size_type>;
    using list_type = pod_ptr<size_type, void_pointer>;

    static constexpr size_type requested_size = RequestedSize;

    // Constructors
    constexpr
    explicit
    pool(
        size_type next_size = 32,
        size_type max_size = 0
    ):
        storage_type(),
        allocator_type(),
        list_(),
        next_size_(next_size),
        start_size_(next_size),
        max_size_(max_size)
    {}

    constexpr
    explicit
    pool(
        const allocator_type& alloc,
        size_type next_size = 32,
        size_type max_size = 0
    ):
        storage_type(),
        allocator_type(alloc),
        list_(),
        next_size_(next_size),
        start_size_(next_size),
        max_size_(max_size)
    {}

    ~pool()
    {
        purge_memory();
    }

    // Observers
    size_type&
    next_size()
    {
        return next_size_;
    }

    const size_type&
    next_size()
    const
    {
        return next_size_;
    }

    size_type&
    start_size()
    {
        return start_size_;
    }

    const size_type&
    start_size()
    const
    {
        return start_size_;
    }

    size_type&
    max_size()
    {
        return max_size_;
    }

    const size_type&
    max_size()
    const
    {
        return max_size_;
    }

    allocator_type
    get_allocator()
    {
        return allocator_type(*this);
    }

    // Modifiers
    bool
    release_memory()
    {
        bool ret = false;

        list_type ptr = list_;
        list_type prev;

        // This is a current & previous iterator pair over the free memory chunk list
        //  Note that "prev_free" in this case does NOT point to the previous memory
        //  chunk in the free list, but rather the last free memory chunk before the
        //  current block.
        void* free_p = storage_type::first_;
        void* prev_free_p = nullptr;

        size_type partition_size = alloc_size();

        // Search through all the all the allocated memory blocks
        while (ptr.valid()) {
            // At this point:
            //  ptr points to a valid memory block
            //  free_p points to either:
            //    0 if there are no more free chunks
            //    the first free chunk in this or some next memory block
            //  prev_free_p points to either:
            //    the last free chunk in some previous memory block
            //    0 if there is no such free chunk
            //  prev is either:
            //    the PODptr whose next() is ptr
            //    !valid() if there is no such PODptr

            // If there are no more free memory chunks, then every remaining
            //  block is allocated out to its fullest capacity, and we can't
            //  release any more memory
            if (free_p == nullptr) {
                break;
            }

            // We have to check all the chunks.  If they are *all* free (i.e., present
            //  in the free list), then we can free the block.
            bool all_chunks_free = true;

            // Iterate 'i' through all chunks in the memory block
            // if free starts in the memory block, be careful to keep it there
            void* saved_free = free_p;
            for (char* i = ptr.begin(); i != ptr.end(); i += partition_size) {
                // If this chunk is not free
                if (i != free_p) {
                    // We won't be able to free this block
                    all_chunks_free = false;

                    // free_p might have travelled outside ptr
                    free_p = saved_free;
                    // Abort searching the chunks; we won't be able to free this
                    //  block because a chunk is not free.
                    break;
                }

                // We do not increment prev_free_p because we are in the same block
                free_p = nextof(free_p);
            }

            // post: if the memory block has any chunks, free_p points to one of them
            // otherwise, our assertions above are still valid

            const list_type next = ptr.next();

            if (!all_chunks_free) {
                if (is_from(free_p, ptr.begin(), ptr.element_size())) {
                    std::less<void*> lt;
                    void* end = ptr.end();
                    do {
                        prev_free_p = free_p;
                        free_p = nextof(free_p);
                    } while (free_p && lt(free_p, end));
                }
                // This invariant is now restored:
                //     free_p points to the first free chunk in some next memory block, or
                //       0 if there is no such chunk.
                //     prev_free_p points to the last free chunk in this memory block.

                // We are just about to advance ptr.  Maintain the invariant:
                // prev is the PODptr whose next() is ptr, or !valid()
                // if there is no such PODptr
                prev = ptr;
            } else {
                // All chunks from this block are free

                // Remove block from list
                if (prev.valid()) {
                    prev.next(next);
                } else {
                    list_ = next;
                }

                // Remove all entries in the free list from this block
                if (prev_free_p != nullptr) {
                    nextof(prev_free_p) = free_p;
                } else {
                    storage_type::first_ = free_p;
                }

                // And release memory
                traits_type::deallocate(*this, ptr.begin());
                ret = true;
            }

            // Increment ptr
            ptr = next;
        }

        next_size_ = start_size_;
        return ret;
    }

    bool
    purge_memory()
    {
        list_type iter = list_;

        if (!iter.valid()) {
            return false;
        }

        do {
            const list_type next = iter.next();
            traits_type::deallocate(*this, iter.begin());
            iter = next;
        } while (iter.valid());

        list_.invalidate();
        storage_type::first_ = nullptr;
        next_size_ = start_size_;

        return true;
    }

    // Allocation
    void*
    allocate()
    {
        if (!store().empty()) {
            return store().allocate();
        }
        return allocate_need_resize();
    }

    void*
    ordered_allocate()
    {
        if (!store().empty()) {
            return store().allocate();
        }
        return ordered_allocate_need_resize();
    }

    void*
    ordered_allocate(
        size_type n
    )
    {
        const size_type partition_size = alloc_size();
        const size_type total_req_size = n * requested_size;
        const size_type num_chunks = fast_ceil_division(total_req_size, partition_size);

        void* ret = store().allocate_n(num_chunks, partition_size);

        if ((ret != nullptr) || (n == 0)) {
            return ret;
        }

        // Not enough memory in our storages; make a new storage,
        next_size_ = std::max(next_size_, num_chunks);
        char* ptr = static_cast<char*>(traits_type::allocate(*this, pod_size()));
        if (ptr == nullptr) {
            if (num_chunks < next_size_) {
                // Try again with just enough memory to do the job, or
                // at least whatever we allocated last time:
                next_size_ >>= 1;
                next_size_ = std::max(next_size_, num_chunks);
                ptr = static_cast<char*>(traits_type::allocate(*this, pod_size()));
            }
            if (ptr == 0) {
                return 0;
            }
        }
        const list_type node(ptr, pod_size());

        // Split up block so we can use what wasn't requested.
        if (next_size_ > num_chunks) {
            void* block = node.begin() + num_chunks * partition_size;
            size_type sz = node.element_size() - num_chunks * partition_size;
            store().add_ordered_block(block, sz, partition_size);
        }

        if (!max_size_) {
            next_size_ <<= 1;
        } else if (next_size_ * partition_size / requested_size < max_size_) {
            next_size_ = std::min(next_size_ << 1, max_size_ * requested_size / partition_size);
        }

        // insert it into the list,
        // handle border case.
        greater<void*> gt;
        if (!list_.valid() || gt(list_.begin(), node.begin())) {
            node.next(list_);
            list_ = node;
        } else {
            list_type prev = list_;

            while (true) {
                // if we're about to hit the end, or if we've found where "node" goes.
                if (prev.next_ptr() == nullptr || gt(prev.next_ptr(), node.begin())) {
                    break;
                }
                prev = prev.next();
            }

            node.next(prev.next());
            prev.next(node);
        }

        return node.begin();
    }

    void deallocate(
        void* chunk
    )
    {
        store().deallocate(chunk);
    }

    void
    ordered_deallocate(
        void* chunk
    )
    {
        store().ordered_deallocate(chunk);
    }

    void
    deallocate(
      void* chunks,
      const size_type n
    )
    {
        size_type partition_size = alloc_size();
        size_type total_req_size = n * requested_size;
        size_type num_chunks = fast_ceil_division(total_req_size, partition_size);

        store().deallocate_n(chunks, num_chunks, partition_size);
    }

    void
    ordered_deallocate(
        void* chunks,
        const size_type n
    )
    {
        const size_type partition_size = alloc_size();
        const size_type total_req_size = n * requested_size;
        size_type num_chunks = fast_ceil_division(total_req_size, partition_size);

        store().ordered_deallocate_n(chunks, num_chunks, partition_size);
    }

    bool
    is_from(
        void* chunk
    )
    const
    {
        return (find_pod(chunk).valid());
    }

protected:
    list_type list_;
    size_type next_size_;
    size_type start_size_;
    size_type max_size_;

    storage_type&
    store()
    {
        return *this;
    }

    const storage_type&
    store()
    const
    {
        return *this;
    }

    list_type
    find_pod(
        void* chunk
    )
    const
    {
        list_type iter = list_;
        while (iter.valid()) {
            if (is_from(chunk, iter.begin(), iter.element_size())) {
                return iter;
            }
            iter = iter.next();
        }
        return iter;
    }

    static
    bool
    is_from(
        void* chunk,
        char* i,
        size_type sizeof_i
    )
    {
        less_equal<void*> le;
        less<void*> lt;
        return le(i, chunk) && lt(chunk, i + sizeof_i);
    }

    size_type alloc_size() const
    {
        size_type s = std::max(requested_size, min_alloc_size);
        size_type rem = s % min_align;
        if (rem) {
            s += min_align - rem;
        }
        assert(s >= min_alloc_size);
        assert(s % min_align == 0);
        return s;
    }

    static
    void*&
    nextof(
        void* ptr
    )
    {
        return *(static_cast<void **>(ptr));
    }

private:
    static constexpr size_type min_alloc_size = lcm(sizeof(void_pointer), sizeof(size_type));
    static constexpr size_type min_align = lcm(alignof(void_pointer), alignof(size_type));

    size_type
    pod_size()
    const
    {
        return static_cast<size_type>(next_size_ * alloc_size() + min_alloc_size + sizeof(size_type));
    }

    void*
    allocate_need_resize()
    {
        size_type partition_size = alloc_size();
        char* ptr = static_cast<char*>(traits_type::allocate(*this, pod_size()));
        if (ptr == nullptr) {
            if (next_size_ > 4) {
                next_size_ >>= 1;
                ptr = static_cast<char*>(traits_type::allocate(*this, pod_size()));
            }
            if (ptr == nullptr) {
                return nullptr;
            }
        }
        const list_type node(ptr, pod_size());

        if (!max_size_) {
            next_size_ <<= 1;
        } else if (next_size_ * partition_size / requested_size < max_size_) {
            next_size_ = std::min(next_size_ << 1, max_size_ * requested_size / partition_size);
        }

        //  initialize it,
        store().add_block(node.begin(), node.element_size(), partition_size);

        //  insert it into the list,
        node.next(list_);
        list_ = node;

        //  and return a chunk from it.
        return store().allocate();
    }

    void*
    ordered_allocate_need_resize()
    {
        size_type partition_size = alloc_size();
        char* ptr = static_cast<char*>(traits_type::allocate(*this, pod_size()));
        if (ptr == nullptr) {
            if (next_size_ > 4) {
                next_size_ >>= 1;
                ptr = static_cast<char*>(traits_type::allocate(*this, pod_size()));
            }
            if (ptr == nullptr) {
                return nullptr;
            }
        }
        const list_type node(ptr, pod_size());

        if (!max_size_) {
            next_size_ <<= 1;
        } else if (next_size_ * partition_size / requested_size < max_size_) {
            next_size_ = std::min(next_size_ << 1, max_size_ * requested_size / partition_size);
        }

        // initialize it,
        store().add_ordered_block(node.begin(), node.element_size(), partition_size);

        // insert it into the list,
        // handle border case
        greater<void*> gt;
        if (!list_.valid() || gt(list_.begin(), node.begin())) {
            node.next(list_);
            list_ = node;
        } else {
            list_type prev = list_;
            while (true) {
                // if we're about to hit the end or
                //  if we've found where "node" goes
                if (prev.next_ptr() == nullptr || gt(prev.next_ptr(), node.begin())) {
                    break;
                }
                prev = prev.next();
            }
            node.next(prev.next());
            prev.next(node);
        }
        //  and return a chunk from it.
        return store().allocate();
    }
};

PYCPP_END_NAMESPACE
