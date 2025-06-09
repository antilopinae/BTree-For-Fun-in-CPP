#pragma once

#include <iostream>
#include <memory>

#include "btree_node.hpp"

namespace btree {

using std::size_t;

template <typename T, size_t ORDER, typename Alloc = std::allocator<T>>
class BTree {
    using BNode = BTreeNode<T, ORDER, Alloc>;

    typedef std::allocator_traits<Alloc> alloc_traits;
    using NodeAllocator = typename alloc_traits::template rebind_alloc<BNode>;
    using KeysAllocator = typename alloc_traits::template rebind_alloc<T>;
    using ChildsAllocator = typename alloc_traits::template rebind_alloc<BNode*>;

private:
    NodeAllocator node_alloc_;
    KeysAllocator keys_alloc_;
    ChildsAllocator childs_alloc_;
    BNode* root_;

public:
    explicit BTree(const Alloc& alloc = Alloc())
        : node_alloc_(NodeAllocator(alloc))
        , keys_alloc_(KeysAllocator(alloc))
        , childs_alloc_(ChildsAllocator(alloc))
        , root_(createNode(true)) {}

public:
    BTree(const BTree& other) = delete;
    BTree(BTree&& other) = delete;

public:
    BTree& operator=(const BTree& other) = delete;
    BTree& operator=(BTree&& other) = delete;

public:
    ~BTree() { clear(root_); }

public:
    template <typename U>
    void traverse(U& u = U()) {
        if (root_ != nullptr) root_->traverse(u);
    }

    BNode* search(T key) { return (root_ == nullptr) ? nullptr : root_->search(key); }

    void insert(T key) {
        if (root_ == nullptr) {
            root_ = createNode(true);
            root_->keys_[0] = key;
            root_->keys_count_ = 1;
        } else {
            if (root_->keys_count_ == 2 * ORDER - 1) {
                BNode* s = createNode(false);
                s->childs_[0] = root_;
                splitChild(*s, 0, *root_);
                size_t i = 0;

                if (s->keys_[0] < key) i++;

                insertNonFull(*s->childs_[i], key);
                root_ = s;
            } else
                insertNonFull(*root_, key);
        }
    }

private:
    void insertNonFull(BNode& node, T key) {
        int i = static_cast<int>(node.keys_count_) - 1;

        if (node.leaf_) {
            while (i >= 0 && node.keys_[i] > key) {
                node.keys_[i + 1] = node.keys_[i];
                --i;
            }

            node.keys_[i + 1] = key;
            node.keys_count_ += 1;
        } else {
            while (i >= 0 && node.keys_[i] > key)
                --i;

            if (node.childs_[i + 1]->keys_count_ == 2 * ORDER - 1) {
                splitChild(node, i + 1, *node.childs_[i + 1]);

                if (node.keys_[i + 1] < key) i++;
            }
            insertNonFull(*node.childs_[i + 1], key);
        }
    }

    void splitChild(BNode& node, size_t i, BNode& y) {
        BNode* z = createNode(y.leaf_);
        z->keys_count_ = ORDER - 1;

        // fill node
        for (size_t j = 0; j < ORDER - 1; j++)
            z->keys_[j] = y.keys_[j + ORDER];

        if (!y.leaf_) {
            for (size_t j = 0; j < ORDER; j++)
                z->childs_[j] = y.childs_[j + ORDER];
        }

        y.keys_count_ = ORDER - 1;

        for (int j = node.keys_count_; j >= static_cast<int>(i + 1); j--)
            node.childs_[j + 1] = node.childs_[j];

        node.childs_[i + 1] = z;

        for (int j = static_cast<int>(node.keys_count_); j > static_cast<int>(i); j--)
            node.keys_[j] = node.keys_[j - 1];

        node.keys_[i] = y.keys_[ORDER - 1];
        node.keys_count_ += 1;
    }

private:
    BNode* createNode(bool isLeaf) {
        BNode* node = std::allocator_traits<NodeAllocator>::allocate(node_alloc_, 1);

        T* keys = std::allocator_traits<KeysAllocator>::allocate(keys_alloc_, 2 * ORDER - 1);
        for (size_t i = 0; i < 2 * ORDER - 1; ++i)
            std::allocator_traits<KeysAllocator>::construct(keys_alloc_, &keys[i]);

        BNode** childs = std::allocator_traits<ChildsAllocator>::allocate(childs_alloc_, 2 * ORDER);

        std::allocator_traits<NodeAllocator>::construct(node_alloc_, node, isLeaf, keys, childs);
        return node;
    }

    void deleteNode(BNode* node) {
        for (size_t i = 0; i < node->keys_count_; ++i)
            std::allocator_traits<KeysAllocator>::destroy(keys_alloc_, node->keys_ + i);
        std::allocator_traits<KeysAllocator>::deallocate(keys_alloc_, node->keys_, 2 * ORDER - 1);

        std::allocator_traits<ChildsAllocator>::deallocate(childs_alloc_, node->childs_, 2 * ORDER);

        std::allocator_traits<NodeAllocator>::destroy(node_alloc_, node);
        std::allocator_traits<NodeAllocator>::deallocate(node_alloc_, node, 1);
    }

    void clear(BNode* node) {
        if (!node) return;
        if (!node->isLeaf()) {
            for (size_t i = 0; i <= node->size(); ++i)
                clear(node->childs_[i]);
        }
        deleteNode(node);
    }
};

}  // namespace btree
