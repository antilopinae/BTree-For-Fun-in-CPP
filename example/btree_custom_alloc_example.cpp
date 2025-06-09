/* Define ONE of these
 * #define MALLOC_SYSTEM_DEFAULT
 * #define USE_CUSTOM_LIST_ALLOCATOR
 *
 * g++ btree_custom_alloc_example.cpp -o btree_test -std=c++17 -DMALLOC_SYSTEM_DEFAULT
 *
 * OR USE:
 *
 * g++ btree_custom_alloc_example.cpp -o btree_test -std=c++17 -DUSE_CUSTOM_LIST_ALLOCATOR
 *
 */

#include "../btree.hpp"
#include "smpl_alloc.hpp"
#include <chrono>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#ifdef USE_CUSTOM_LIST_ALLOCATOR
custom_list_allocator_t* g_my_custom_list_alloc = nullptr;
#endif

struct TraverseCounter {
    size_t count = 0;
    size_t sum = 0;
    void operator()(uint32_t key) {
        count++;
        sum += key;
    }
};

void perform_btree_operations(const std::string& allocator_name,
                              btree::BTree<uint32_t, 64, smpl_alloc<uint32_t>>& btree_instance,
                              size_t num_insertions) {
    std::cout << "\n--- Operations with " << allocator_name << " ---" << std::endl;
    std::cout << "B-Tree Order: " << 64 << std::endl;
    std::cout << "Number of insertions: " << num_insertions << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (uint32_t i = 0; i < num_insertions; ++i) {
        btree_instance.insert(i % 1000);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    std::cout << "Insertion finished in: " << elapsed_seconds.count() << " seconds." << std::endl;

    TraverseCounter counter;
    std::cout << "Traversing B-Tree..." << std::endl;
    start_time = std::chrono::high_resolution_clock::now();

    btree_instance.traverse(counter);

    end_time = std::chrono::high_resolution_clock::now();
    elapsed_seconds = end_time - start_time;

    std::cout << "Traversal finished in: " << elapsed_seconds.count() << " seconds." << std::endl;
    std::cout << "Keys traversed (functor calls): " << counter.count << std::endl;

    uint32_t search_key = num_insertions / 2 % 1000;
    std::cout << "Searching for key: " << search_key << std::endl;

    start_time = std::chrono::high_resolution_clock::now();
    auto* found_node = btree_instance.search(search_key);
    end_time = std::chrono::high_resolution_clock::now();

    elapsed_seconds = end_time - start_time;
    std::cout << "Search finished in: " << elapsed_seconds.count() << " seconds." << std::endl;
    if (found_node) {
        std::cout << "Key " << search_key << " found in a node with " << found_node->size() << " keys." << std::endl;
    } else {
        std::cout << "Key " << search_key << " NOT found." << std::endl;
    }
}

int main() {
    const size_t num_elements_to_insert = 100000;
    const size_t btree_order = 64;

#if defined(MALLOC_SYSTEM_DEFAULT)
    std::cout << "Using System Allocator (via smpl_alloc)" << std::endl;
    smpl_alloc<uint32_t> system_smp_alloc;
    btree::BTree<uint32_t, btree_order, smpl_alloc<uint32_t>> btree_sys(system_smp_alloc);

    perform_btree_operations("System Allocator", btree_sys, num_elements_to_insert);

#elif defined(USE_CUSTOM_LIST_ALLOCATOR)
    std::cout << "Using Custom List Allocator (via smpl_alloc)" << std::endl;
    g_my_custom_list_alloc = custom_list_alloc_create(CUSTOM_LIST_HEAP_SIZE, ALLOC_POLICY_FIRST_FIT);

    if (!g_my_custom_list_alloc) {
        std::cerr << "Failed to create custom list allocator." << std::endl;
        return 1;
    }

    {
        smpl_alloc<uint32_t> custom_smp_alloc(g_my_custom_list_alloc);
        btree::BTree<uint32_t, btree_order, smpl_alloc<uint32_t>> btree_custom(custom_smp_alloc);

        perform_btree_operations("Custom List Allocator (First-Fit)", btree_custom, num_elements_to_insert);
    }

    std::cout << "\nFinal allocator state AFTER B-Tree destruction:" << std::endl;
    custom_list_print_info(g_my_custom_list_alloc);

    custom_list_alloc_destroy(g_my_custom_list_alloc);
    g_my_custom_list_alloc = nullptr;

#else
    std::cout << "No specific allocator defined for smpl_alloc. Please define MALLOC_SYSTEM_DEFAULT or USE_CUSTOM_LIST_ALLOCATOR."
              << std::endl;
#endif

    std::cout << "\nProgram finished successfully." << std::endl;
    return 0;
}
