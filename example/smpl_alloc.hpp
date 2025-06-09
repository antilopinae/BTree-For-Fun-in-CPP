#pragma once

#include <cstddef>
#include <iostream>
#include <new>
#include <stdexcept>

#ifdef USE_CUSTOM_LIST_ALLOCATOR
#include "custom_list_allocator.hpp"
#endif

template <typename T>
struct smpl_alloc {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind {
        using other = smpl_alloc<U>;
    };

#ifdef USE_CUSTOM_LIST_ALLOCATOR
    custom_list_allocator_t* custom_alloc_instance_ = nullptr;

    explicit smpl_alloc(custom_list_allocator_t* alloc_instance = nullptr) noexcept : custom_alloc_instance_(alloc_instance) {}
#else
    smpl_alloc() noexcept {}
#endif

    template <typename U>
    smpl_alloc(const smpl_alloc<U>& other) noexcept {
#ifdef USE_CUSTOM_LIST_ALLOCATOR
        custom_alloc_instance_ = other.custom_alloc_instance_;
#endif
    }

    smpl_alloc(const smpl_alloc& other) noexcept {
#ifdef USE_CUSTOM_LIST_ALLOCATOR
        custom_alloc_instance_ = other.custom_alloc_instance_;
#endif
    }

    pointer allocate(size_type n) {
        if (n > static_cast<size_type>(-1) / sizeof(value_type)) {
            throw std::bad_alloc();
        }
#if defined(MALLOC_SYSTEM_DEFAULT)
        return static_cast<pointer>(::operator new(n * sizeof(value_type)));
#elif defined(USE_CUSTOM_LIST_ALLOCATOR)
        if (!custom_alloc_instance_) {
            throw std::runtime_error("Custom list allocator instance not set for smpl_alloc during allocate");
        }
        void* mem = custom_list_malloc(custom_alloc_instance_, n * sizeof(value_type));
        if (!mem) throw std::bad_alloc();
        return static_cast<pointer>(mem);
#else
        throw std::runtime_error(
            "No allocation strategy defined for smpl_alloc (MALLOC_SYSTEM_DEFAULT or USE_CUSTOM_LIST_ALLOCATOR)");
#endif
    }

    void deallocate(pointer p, size_type n) noexcept {
        (void) n;
#if defined(MALLOC_SYSTEM_DEFAULT)
        ::operator delete(p);
#elif defined(USE_CUSTOM_LIST_ALLOCATOR)
        if (!custom_alloc_instance_) {
            // This is a severe issue, deallocating without a valid allocator instance
            // For robustness, one might log this or assert in debug, but not throw from noexcept
            return;
        }
        custom_list_free(custom_alloc_instance_, p);
#else
        // No deallocation strategy
#endif
    }
};

template <typename T, typename U>
bool operator==(const smpl_alloc<T>& lhs, const smpl_alloc<U>& rhs) {
#ifdef USE_CUSTOM_LIST_ALLOCATOR
    return lhs.custom_alloc_instance_ == rhs.custom_alloc_instance_;
#else
    return true;
#endif
}

template <typename T, typename U>
bool operator!=(const smpl_alloc<T>& lhs, const smpl_alloc<U>& rhs) {
    return !(lhs == rhs);
}
