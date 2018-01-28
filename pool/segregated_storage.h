//  :copyright: (c) 2000, 2001 Stephen Cleary.
//  :copyright: (c) 2017-2018 Alex Huszagh.
//  :license: MIT, see licenses/mit.md for more details.
/**
 *  \addtogroup PyCPP
 *  \brief Fast segregated storage.
 *
 *  Fast segregated storage using free linked-lists with memory chunks.
 */

#pragma once

#include <pycpp/config.h>
#include <pycpp/stl/functional.h>

PYCPP_BEGIN_NAMESPACE

// OBJECTS
// -------


/**
 *  Simple Segregated Storage is the simplest, and probably the fastest,
 *  memory allocation/deallocation algorithm.  It is responsible for
 *  partitioning a memory block into fixed-size chunks: where the block
 *  comes from is determined by the client of the class.
 *
 *  Template class segregated_storage controls access to a free
 *  list of memory chunks. Please note that this is a very simple class,
 *  with preconditions on almost all its functions. It is intended to
 *  be the fastest and smallest possible quick memory allocator - e.g.,
 *  something to use in embedded systems. This class delegates many
 *  difficult preconditions to the user (i.e., alignment issues).
 *
 *  An object of type segregated_storage is empty if its free
 *  list is empty. If it is not empty, then it is ordered if its free
 *  list is ordered. A free list is ordered if repeated calls
 *  to `malloc()` will result in a constantly-increasing sequence of
 *  values, as determined by `less<void*>`.
 *
 *  A member function is order-preserving if the free list maintains
 *  its order orientation (that is, an ordered free list is still
 *  ordered after the member function call).
 */
template <typename SizeType>
class segregated_storage
{
public:
    using size_type = SizeType;

    // Constructors
    constexpr
    segregated_storage():
        first_(nullptr)
    {}

    segregated_storage(const segregated_storage &)= delete;
    segregated_storage& operator=(const segregated_storage &) = delete;

    // Observers
    bool
    empty()
    const
    {
        return first_ == nullptr;
    }

    // Modifiers
    static
    void*
    segregate(
        void* block,
        size_type sz,
        size_type partition_sz,
        void* end = nullptr
    )
    {
        // Get pointer to last valid chunk, preventing overflow on size calculations
        //  The division followed by the multiplication just makes sure that
        //    old == block + partition_sz * i, for some integer i, even if the
        //    block size (sz) is not a multiple of the partition size.
        size_type shift = ((sz - partition_sz) / partition_sz) * partition_sz;
        char* old = static_cast<char*>(block) + shift;
        nextof(old) = end;

        // Handle border case where sz == partition_sz (i.e.,
        // we're handling an array of 1 element)
        if (old == block) {
            return block;
        }

        // Iterate backwards, building a singly-linked list of pointers
        for (char* iter = old - partition_sz; iter != block; old = iter, iter -= partition_sz) {
            nextof(iter) = old;
        }

        // Point the first pointer, too
        nextof(block) = old;
        return block;
    }

    void
    add_block(
        void* block,
        size_type nsz, size_type npartition_sz
    )
    {
        first_ = segregate(block, nsz, npartition_sz, first_);
    }

    void
    add_ordered_block(
        void* block,
        size_type nsz,
        size_type npartition_sz
    )
    {
        void* loc = find_prev(block);

        if (loc == nullptr) {
            add_block(block, nsz, npartition_sz);
        } else {
            nextof(loc) = segregate(block, nsz, npartition_sz, nextof(loc));
        }
    }

    // Allocation
    void*
    allocate()
    {
        void* ret = first_;
        first_ = nextof(first_);
        return ret;
    }

    void
    deallocate(
        void* chunk
    )
    {
        nextof(chunk) = first_;
        first_ = chunk;
    }

    void
    ordered_deallocate(
        void* chunk
    )
    {
        // Find where "chunk" goes in the free list
        void* loc = find_prev(chunk);

        // Place either at beginning or in middle/end.
        if (loc == 0) {
            deallocate(chunk);
        } else {
            nextof(chunk) = nextof(loc);
            nextof(loc) = chunk;
        }
    }

    // Attempts to find a contiguous sequence of n partition_sz-sized
    // chunks. If found, removes them  all from the free list and
    // returns a pointer to the first. If not found, returns 0. It is
    // strongly recommended (but not required) that the free list be
    // ordered, as this algorithm will fail to find a contiguous
    // sequence unless it is contiguous in the free list as well.
    // Order-preserving. O(N) with respect to the size of the free list.
    void*
    allocate_n(
        size_type n,
        size_type partition_size
    )
    {
        if(n == 0) {
            return nullptr;
        }
        void* start = &first_;
        void* iter;
        do {
            if (nextof(start) == 0) {
                return nullptr;
            }
            iter = try_malloc_n(start, n, partition_size);
        } while (iter == nullptr);

        void* ret = nextof(start);
        nextof(start) = nextof(iter);
        return ret;
    }

    void
    deallocate_n(
        void* chunks,
        size_type n,
        size_type partition_size
    )
    {
        if (n != nullptr) {
            add_block(chunks, n * partition_size, partition_size);
        }
    }

    void
    ordered_deallocate_n(
        void* chunks,
        size_type n,
        size_type partition_size
    )
    {
        if (n != 0) {
            add_ordered_block(chunks, n * partition_size, partition_size);
        }
    }

protected:
    void* first_;

    void*
    find_prev(
        void* ptr
    )
    {
        greater<void> gt;
        if (first_ == nullptr || gt(first_, ptr)) {
            return nullptr;
        }

        void* iter = first_;
        while (true) {
            // if we're about to hit the end, or if we've found where "ptr" goes.
            if (nextof(iter) == nullptr || gt(nextof(iter), ptr)) {
                return iter;
            }
            iter = nextof(iter);
        }
    }

    static
    void*&
    nextof(
        void* ptr
    )
    {
        return *(static_cast<void**>(ptr));
    }

private:
    // \pre (n > 0), (start != 0), (nextof(start) != 0)
    // \post (start != 0)
    // The function attempts to find n contiguous chunks
    //  of size partition_size in the free list, starting at start.
    // If it succeeds, it returns the last chunk in that contiguous
    //  sequence, so that the sequence is known by [start, {retval}]
    // If it fails, it does do either because it's at the end of the
    //  free list or hits a non-contiguous chunk.  In either case,
    //  it will return 0, and set start to the last considered
    //  chunk.  You are at the end of the free list if
    //  nextof(start) == 0.  Otherwise, start points to the last
    //  chunk in the contiguous sequence, and nextof(start) points
    //  to the first chunk in the next contiguous sequence (assuming
    //  an ordered free list).
    static
    void*
    try_allocate_n(
        void*& start,
        size_type n,
        size_type partition_size
    )
    {
        void* iter = nextof(start);
        while (--n != 0) {
            void* next = nextof(iter);
            if (next != static_cast<char*>(iter) + partition_size) {
                // next == 0 (end-of-list) or non-contiguous chunk found
                start = iter;
                return nullptr;
            }
            iter = next;
        }
        return iter;
    }
};

PYCPP_END_NAMESPACE
