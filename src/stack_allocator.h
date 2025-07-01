#ifndef LIST_DMASLOFF_STACK_ALLOCATOR_H
#define LIST_DMASLOFF_STACK_ALLOCATOR_H

#include <iostream>

template <size_t N>
class StackStorage {
  public:
    StackStorage() = default;
    StackStorage(const StackStorage<N>& another) = delete;
    StackStorage& operator=(const StackStorage&) = delete;

    char buffer[N];
    size_t shift = 0;
};

template <typename T, size_t N>
class StackAllocator {
  public:
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;
    using propagate_on_container_copy_assignment = std::true_type;

    StackAllocator() = default;
    StackAllocator(StackStorage<N>& storage)
        : ptr(&storage){};

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& another)
        : ptr(another.ptr){};

    template <typename U>
    StackAllocator& operator=(const StackAllocator<U, N>& another) {
        ptr = another.ptr;
        return *this;
    }

    StackAllocator& select_on_container_copy_construction() {
        return *this;
    }

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

    pointer allocate(size_t n) {
        size_t space_left = N - ptr->shift;
        void* stack_pointer = reinterpret_cast<void*>(ptr->buffer + ptr->shift);
        void* new_pointer = std::align(alignof(T), n * sizeof(T), stack_pointer, space_left);
        ptr->shift = N + n * sizeof(T) - space_left;
        return reinterpret_cast<pointer>(new_pointer);
    }

    template <typename... Args>
    void construct(pointer stack_ptr, const Args&... args) {
        new (stack_ptr) T(args...);
    }

    void destroy(pointer stack_ptr) {
        stack_ptr->~T();
    }

    void deallocate(T* dealloc_pointer, size_t n) {
        std::ignore = dealloc_pointer + n;
    }

    template <typename U, size_t N1>
    bool operator==(const StackAllocator<U, N1>& another) {
        return ptr == another.ptr;
    }

    template <typename U, size_t N1>
    bool operator!=(const StackAllocator<U, N1>& another) {
        return ptr != another.ptr;
    }

    StackStorage<N>* ptr = nullptr;
};

#endif  //LIST_DMASLOFF_STACK_ALLOCATOR_H