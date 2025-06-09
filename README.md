# BTree For Fun in CPP (C++)

This project provides a C++ implementation of a B-Tree data structure, featuring support for custom memory allocators. It serves as an exploration of B-Tree mechanics and the integration of custom memory management techniques within complex data structures.

The repository includes the B-Tree core logic and example custom allocators to demonstrate their usage.

## Features

*   **Generic B-Tree**: Templated by key type (`T`) and `ORDER`.
*   **Custom Allocator Support**: Fully integrated with C++ allocators, allowing for fine-grained memory control or use with custom memory pools (demonstrated via `smpl_alloc.hpp`).
*   **Core B-Tree Operations**:
    *   Insertion of keys.
    *   Searching for keys.
    *   Traversal of keys (in-order).
*   **Example Usage**: `btree_list_malloc.cpp` and `btree_stack_malloc.cpp` demonstrate how to use the B-Tree with `smpl_alloc` backed by custom C-style memory managers.

## Building and Running Examples

The project is structured with header files for the B-Tree and allocators, and example `main` files.

1.  **Setup**: C++17 or newer (due to `std::allocator_traits`).

2.  **Compilation**:
See instruction in the top of `btree_custom_alloc_example.cpp`.

3.  **Running**:
    ```bash
    ./btree_example_list
    ```

It is not intended as a production-ready, highly optimized B-Tree library but rather as a clear and understandable implementation of the core concepts.

## License
[MIT License](LICENSE)
