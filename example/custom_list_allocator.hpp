#pragma once

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CUSTOM_LIST_HEAP_SIZE (1024 * 1024 * 100)

typedef struct custom_list_allocator_s custom_list_allocator_t;

typedef enum { ALLOC_POLICY_FIRST_FIT, ALLOC_POLICY_BEST_FIT } alloc_policy_t;

typedef struct free_node_s {
    struct free_node_s* next;
    size_t size;
} free_node_t;

struct custom_list_allocator_s {
    uint8_t* heap_memory;
    free_node_t* free_list_head;
    size_t total_size;
    size_t used_size;
    alloc_policy_t policy;
};

custom_list_allocator_t* custom_list_alloc_create(size_t heap_size, alloc_policy_t policy);
void custom_list_alloc_destroy(custom_list_allocator_t* alloc);
void* custom_list_malloc(custom_list_allocator_t* alloc, size_t size);
void custom_list_free(custom_list_allocator_t* alloc, void* ptr);
void custom_list_print_info(const custom_list_allocator_t* alloc);

static inline size_t align_size(size_t size) {
    const size_t alignment = sizeof(void*);
    return (size + alignment - 1) & ~(alignment - 1);
}

static void node_find_first(custom_list_allocator_t* alloc, size_t size, free_node_t** out_prev, free_node_t** out_cur) {
    free_node_t* prev = NULL;
    free_node_t* cur = alloc->free_list_head;
    *out_prev = NULL;
    *out_cur = NULL;

    while (cur) {
        if (cur->size >= size) {
            *out_prev = prev;
            *out_cur = cur;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

static void node_find_best(custom_list_allocator_t* alloc, size_t size, free_node_t** out_prev, free_node_t** out_cur) {
    free_node_t* prev = NULL;
    free_node_t* cur = alloc->free_list_head;
    free_node_t* best_prev = NULL;
    free_node_t* best_cur = NULL;
    size_t min_diff = SIZE_MAX;

    while (cur) {
        if (cur->size >= size) {
            size_t diff = cur->size - size;
            if (diff < min_diff) {
                min_diff = diff;
                best_prev = prev;
                best_cur = cur;
                if (diff == 0) break;
            }
        }
        prev = cur;
        cur = cur->next;
    }
    *out_prev = best_prev;
    *out_cur = best_cur;
}

static void node_find(custom_list_allocator_t* alloc, size_t size, free_node_t** out_prev, free_node_t** out_cur) {
    switch (alloc->policy) {
        case ALLOC_POLICY_FIRST_FIT:
            node_find_first(alloc, size, out_prev, out_cur);
            return;
        case ALLOC_POLICY_BEST_FIT:
            node_find_best(alloc, size, out_prev, out_cur);
            return;
        default:
            *out_prev = NULL;
            *out_cur = NULL;
            break;
    }
}

static void insert_free_node(custom_list_allocator_t* alloc, free_node_t* node_to_insert) {
    free_node_t* prev = NULL;
    free_node_t* curr = alloc->free_list_head;

    while (curr != NULL && curr < node_to_insert) {
        prev = curr;
        curr = curr->next;
    }

    if (prev == NULL) {
        node_to_insert->next = alloc->free_list_head;
        alloc->free_list_head = node_to_insert;
    } else {
        node_to_insert->next = prev->next;
        prev->next = node_to_insert;
    }
}

static void remove_free_node(custom_list_allocator_t* alloc, free_node_t* prev, free_node_t* node_to_remove) {
    if (prev == NULL) {
        alloc->free_list_head = node_to_remove->next;
    } else {
        prev->next = node_to_remove->next;
    }
}

static void coalesce_free_nodes(custom_list_allocator_t* alloc) {
    free_node_t* curr = alloc->free_list_head;

    while (curr != NULL && curr->next != NULL) {
        if ((uint8_t*) curr + curr->size == (uint8_t*) curr->next) {
            curr->size += curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

custom_list_allocator_t* custom_list_alloc_create(size_t heap_size, alloc_policy_t policy) {
    if (heap_size < sizeof(free_node_t) + sizeof(size_t)) {
        return NULL;
    }

    custom_list_allocator_t* alloc = (custom_list_allocator_t*) malloc(sizeof(custom_list_allocator_t));
    if (!alloc) {
        return NULL;
    }

    alloc->heap_memory = (uint8_t*) malloc(heap_size);
    if (!alloc->heap_memory) {
        free(alloc);
        return NULL;
    }

    alloc->total_size = heap_size;
    alloc->used_size = 0;
    alloc->policy = policy;

    alloc->free_list_head = (free_node_t*) alloc->heap_memory;
    alloc->free_list_head->size = heap_size;
    alloc->free_list_head->next = NULL;

    return alloc;
}

void custom_list_alloc_destroy(custom_list_allocator_t* alloc) {
    if (!alloc) return;
    free(alloc->heap_memory);
    free(alloc);
}

void* custom_list_malloc(custom_list_allocator_t* alloc, size_t user_size) {
    if (!alloc || user_size == 0) {
        return NULL;
    }

    size_t actual_requested_size = align_size(user_size + sizeof(size_t));
    if (actual_requested_size < sizeof(free_node_t)) {
        actual_requested_size = sizeof(free_node_t);
    }

    free_node_t* prev_found = NULL;
    free_node_t* found_node = NULL;
    node_find(alloc, actual_requested_size, &prev_found, &found_node);

    if (!found_node) {
        return NULL;
    }

    if (found_node->size >= actual_requested_size + sizeof(free_node_t)) {
        free_node_t* new_free_node = (free_node_t*) ((uint8_t*) found_node + actual_requested_size);
        new_free_node->size = found_node->size - actual_requested_size;

        remove_free_node(alloc, prev_found, found_node);
        insert_free_node(alloc, new_free_node);  // Insert new smaller free block
        // This also handles if new_free_node->next should be set

        found_node->size = actual_requested_size;
    } else {
        remove_free_node(alloc, prev_found, found_node);
    }

    *((size_t*) found_node) = found_node->size;
    alloc->used_size += found_node->size;

    return (void*) ((uint8_t*) found_node + sizeof(size_t));
}

void custom_list_free(custom_list_allocator_t* alloc, void* ptr) {
    if (!alloc || !ptr) {
        return;
    }

    uint8_t* block_start = (uint8_t*) ptr - sizeof(size_t);
    free_node_t* node_to_free = (free_node_t*) block_start;

    node_to_free->size = *((size_t*) block_start);

    alloc->used_size -= node_to_free->size;

    insert_free_node(alloc, node_to_free);
    coalesce_free_nodes(alloc);
}

void custom_list_print_info(const custom_list_allocator_t* alloc) {
    if (!alloc) {
        printf("Allocator not initialized.\n");
        return;
    }
    printf("Custom List Allocator Info:\n");
    printf("  Total Size: %zu bytes\n", alloc->total_size);
    printf("  Used Size:  %zu bytes\n", alloc->used_size);
    printf("  Free Size:  %zu bytes\n", alloc->total_size - alloc->used_size);
    printf("  Policy:     %s\n", alloc->policy == ALLOC_POLICY_FIRST_FIT ? "First-Fit" : "Best-Fit");
    printf("  Free List:\n");

    free_node_t* node = alloc->free_list_head;
    size_t num_free_nodes = 0;
    size_t total_free_mem_in_list = 0;
    while (node) {
        num_free_nodes++;
        printf("    Node %zu: Address=%p, Size=%zu bytes\n", num_free_nodes, (void*) node, node->size);
        total_free_mem_in_list += node->size;
        node = node->next;
    }
    printf("  Number of free blocks: %zu\n", num_free_nodes);
    printf("  Total memory in free list: %zu bytes\n\n", total_free_mem_in_list);
}
