#ifndef LIST_DMASLOFF_LIST_H
#define LIST_DMASLOFF_LIST_H

#include <iostream>
#include "stack_allocator.h"

template <typename T, typename Alloc = std::allocator<T>>
class List {
  private:
    struct _base_node {
        _base_node()
            : prev_(this), next_(this){};
        _base_node(_base_node* prev, _base_node* next)
            : prev_(prev), next_(next){};
        _base_node* prev_ = this;
        _base_node* next_ = this;
    };

    struct _node : _base_node {
        _node(const T& value, _base_node* prev, _base_node* next)
            : _base_node(prev, next), value_(value){};
        _node(const T& value)
            : value_(value){};
        _node()
            : value_(T()) {}

        T value_;
    };

    template <bool is_const>
    class _list_iterator {
      public:
        using type = T;
        using const_type = typename std::add_const<T>::type;

        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = type;
        using pointer = typename std::conditional<is_const, const_type*, type*>::type;
        using reference = typename std::conditional<is_const, const_type&, type&>::type;

        using iterator = _list_iterator<is_const>;
        using iterator_reference = _list_iterator<is_const>&;
        using iterator_const_reference = const _list_iterator<is_const>&;
        using const_iterator = _list_iterator<true>;
        using const_iterator_reference = _list_iterator<true>&;
        using const_iterator_const_reference = const _list_iterator<true>&;

        friend List<T, Alloc>;
        friend _list_iterator<true>;
        friend _list_iterator<false>;

        explicit _list_iterator(_base_node* ptr)
            : _node_pointer(ptr){};

        explicit _list_iterator(const _base_node* ptr)
            // Sorry, const list has const field _base_node, but
            // pointers in iterator are not const, even if the deque is const
            // because of increment and decrement operators
            : _node_pointer(const_cast<_base_node*>(ptr)){};  //NOLINT

        template <bool is_const1>
        explicit _list_iterator(const _list_iterator<is_const1>& another)
            : _node_pointer(another._node_pointer){};

        template <bool is_const1>
        bool operator==(const _list_iterator<is_const1>& it1) const {
            return _node_pointer == it1._node_pointer;
        }

        template <bool is_const1>
        bool operator!=(const _list_iterator<is_const1>& it1) const {
            return _node_pointer != it1._node_pointer;
        }

        iterator_reference operator++() {
            _node_pointer = _node_pointer->next_;
            return *this;
        }

        iterator operator++(int) {
            iterator copy = *this;
            ++(*this);
            return copy;
        }

        iterator_reference operator--() {
            _node_pointer = _node_pointer->prev_;
            return *this;
        }

        iterator operator--(int) {
            iterator copy = *this;
            --(*this);
            return copy;
        }

        reference operator*() const {
            return static_cast<_node*>(_node_pointer)->value_;
        }

        operator const_iterator() const {
            return const_iterator(_node_pointer);
        }

      private:
        _base_node* _node_pointer;
    };

  public:
    using type = T;
    using const_type = typename std::add_const<T>::type;
    using reference = type&;
    using const_reference = const_type&;
    using iterator = _list_iterator<false>;
    using const_iterator = _list_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using node_allocator = typename std::allocator_traits<Alloc>::template rebind_alloc<_node>;

    List() = default;

    explicit List(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            try {
                default_push_back();
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(size_t size, const T& value) {
        for (size_t i = 0; i < size; ++i) {
            try {
                push_back(value);
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    explicit List(const Alloc& alloc)
        : _alloc(alloc){};

    List(size_t size, const Alloc& alloc)
        : _alloc(alloc) {
        for (size_t i = 0; i < size; ++i) {
            try {
                default_push_back();
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(size_t size, const T& value, const Alloc& alloc)
        : _alloc(alloc) {
        for (size_t i = 0; i < size; ++i) {
            try {
                push_back(value);
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(const List& another)
        : _alloc(std::allocator_traits<Alloc>::select_on_container_copy_construction(another._alloc)) {
        for (const auto& obj : another) {
            try {
                push_back(obj);
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List& operator=(const List& another) {
        List copy(std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value ? another._alloc : _alloc);
        for (const auto& obj : another) {
            try {
                copy.push_back(obj);
            } catch (...) {
                copy.clear();
                throw;
            }
        }
        Swap(copy);
        return *this;
    }

    Alloc get_allocator() const {
        return static_cast<Alloc>(_alloc);
    }

    size_t size() const {
        return _list_size;
    }

    void push_front(const T& value) {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, value);
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        node_ptr->next_ = _virtual_node.next_;
        _virtual_node.next_->prev_ = node_ptr;
        node_ptr->prev_ = &_virtual_node;
        _virtual_node.next_ = node_ptr;
        ++_list_size;
    }

    void push_back(const T& value) {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, value);
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        node_ptr->prev_ = _virtual_node.prev_;
        _virtual_node.prev_->next_ = node_ptr;
        node_ptr->next_ = &_virtual_node;
        _virtual_node.prev_ = node_ptr;
        ++_list_size;
    }

    void pop_front() {
        auto ptr = static_cast<_node*>(_virtual_node.next_);
        _virtual_node.next_ = _virtual_node.next_->next_;
        _virtual_node.next_->prev_ = &_virtual_node;
        std::allocator_traits<node_allocator>::destroy(_alloc, ptr);
        std::allocator_traits<node_allocator>::deallocate(_alloc, ptr, 1);
        --_list_size;
    }

    void pop_back() {
        auto ptr = static_cast<_node*>(_virtual_node.prev_);
        _virtual_node.prev_ = _virtual_node.prev_->prev_;
        _virtual_node.prev_->next_ = &_virtual_node;
        std::allocator_traits<node_allocator>::destroy(_alloc, ptr);
        std::allocator_traits<node_allocator>::deallocate(_alloc, ptr, 1);
        --_list_size;
    }

    void insert(const_iterator iter, const T& value) {
        if (iter._node_pointer == &_virtual_node) {
            push_back(value);
        } else {
            _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
            try {
                std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, value);
            } catch (...) {
                std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
                throw;
            }
            node_ptr->next_ = iter._node_pointer;
            node_ptr->prev_ = iter._node_pointer->prev_;
            iter._node_pointer->prev_->next_ = node_ptr;
            iter._node_pointer->prev_ = node_ptr;
            ++_list_size;
        }
    }

    void erase(const_iterator iter) {
        iter._node_pointer->prev_->next_ = iter._node_pointer->next_;
        iter._node_pointer->next_->prev_ = iter._node_pointer->prev_;
        std::allocator_traits<node_allocator>::destroy(_alloc, static_cast<_node*>(iter._node_pointer));
        std::allocator_traits<node_allocator>::deallocate(_alloc, static_cast<_node*>(iter._node_pointer), 1);
        --_list_size;
    }

    void clear() {
        while (_list_size > 0) {
            pop_back();
        }
    }

    iterator begin() {
        return iterator(_virtual_node.next_);
    }
    const_iterator begin() const {
        return const_iterator(_virtual_node.next_);
    }
    const_iterator cbegin() const {
        return const_iterator(_virtual_node.next_);
    }
    iterator end() {
        return iterator(&_virtual_node);
    }
    const_iterator end() const {
        return const_iterator(&_virtual_node);
    }
    const_iterator cend() const {
        return const_iterator(&_virtual_node);
    }
    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }
    reverse_iterator rend() {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crend() const {
        return const_reverse_iterator(begin());
    }

    ~List() {
        while (_list_size != 0) {
            pop_back();
        }
    }

  private:
    [[no_unique_address]] node_allocator _alloc = Alloc();
    _base_node _virtual_node = _base_node();
    size_t _list_size = 0;

    void Swap(List<T, Alloc>& another) {
        std::swap(_alloc, another._alloc);
        std::swap(_virtual_node, another._virtual_node);
        std::swap(_virtual_node.next_->prev_, another._virtual_node.next_->prev_);
        std::swap(_virtual_node.prev_->next_, another._virtual_node.prev_->next_);
        std::swap(_list_size, another._list_size);
    }

    void default_push_back() {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr);
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        _virtual_node.prev_->next_ = node_ptr;
        node_ptr->prev_ = _virtual_node.prev_;
        _virtual_node.prev_ = node_ptr;
        node_ptr->next_ = &_virtual_node;
        ++_list_size;
    }
};

#endif  // LIST_DMASLOFF_LIST_H