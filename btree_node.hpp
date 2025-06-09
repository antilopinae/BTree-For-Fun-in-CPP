#pragma once

#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace btree {

using std::size_t;

template <typename T, size_t ORDER, typename Alloc = std::allocator<T>>
struct BTreeNode {
    T* keys_;
    BTreeNode** childs_;
    size_t keys_count_;
    bool leaf_;

public:
    BTreeNode(bool leaf, T* keys, BTreeNode** childs);
    ~BTreeNode() {}

    void insertNonFull(T key);
    void splitChild(size_t i, BTreeNode& y);

    template <typename U>
    void traverse(U& u = U());

    BTreeNode* search(T key);

public:
    __attribute__((always_inline)) bool isLeaf() const noexcept { return leaf_; }

    __attribute__((always_inline)) size_t size() const noexcept { return keys_count_; }
};

template <typename T, size_t ORDER, typename Alloc>
BTreeNode<T, ORDER, Alloc>::BTreeNode(bool leaf, T* keys, BTreeNode** childs)
    : keys_(keys), childs_(childs), keys_count_(0), leaf_(leaf) {
}

template <typename T, size_t ORDER, typename Alloc>
template <typename U>
void BTreeNode<T, ORDER, Alloc>::traverse(U& u) {
    size_t i;
    for (i = 0; i < keys_count_; i++) {
        if (leaf_ == false) {
            childs_[i]->traverse(u);
        }
        u(keys_[i]);
    }

    if (leaf_ == false) childs_[i]->traverse(u);
}

template <typename T, size_t ORDER, typename Alloc>
BTreeNode<T, ORDER, Alloc>* BTreeNode<T, ORDER, Alloc>::search(T k) {
    size_t i = 0;
    while (i < keys_count_ && k > keys_[i])
        ++i;

    if (keys_[i] == k) return this;

    if (leaf_) return nullptr;

    return childs_[i]->search(k);
}

}  // namespace btree
