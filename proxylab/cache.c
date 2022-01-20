/*
 * Name: Laura (Kai Sze) Luk
 * AndrewID: kluk
 *
 * cache.c – A main memory cache implementation for recently accessed
 *           web content.
 *
 * Overall design: A cache struct holding a pointer to a doubly-linked
 *                 list of cache blocks, and the remaining empty bytes.
 *
 * 1. Each new cache block is inserted at head of linked list.
 *
 * 2. Evicted cache blocks are determined with an LRU policy, when cache
 * is full. The LRU policy is managed through the global time_counter variable
 * (defined in proxy.c). The counter is incremented each time a cache is being
 * accessed (cache hit/insertion). The "age" field in the working cache block
 * is updated to the current time_counter.
 *
 * 3. The stored contents/response in the cache is copied into a string
 * function argument that will be passed to the client.
 *
 */

#include "cache.h"
#include "csapp.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Initializes the cache data structure.
 *
 * This function allocates and initializes the fields of the cache struct.
 *
 * Return value – the cache struct
 *
 */
cache_t *init_cache() {
    cache_t *cache = Malloc(sizeof(cache_t));
    cache->head = NULL;
    cache->slots = MAX_CACHE_SIZE;
    return cache;
}

/**
 * @brief Searches through the cache for the request URI.
 *
 * This function steps through each cache block, comparing the URI to that
 * of the client's.
 *
 * Function arguments – the working cache, and the request URI to find
 * Return value – the cache block/node with the matching URI, or NULL
 *
 */
node *search_cache(cache_t *cache, char *req_uri) {
    node *start = cache->head;

    while (start != NULL) {
        if (!strcmp(start->uri, req_uri)) {
            return start;
        }
        start = start->next;
    }
    return NULL;
}

/**
 * @brief Reads the response stored in the current cache block/node.
 *
 * This function copies the response stored inside the current cache node,
 * and stores it in contents argument.
 *
 * Function arguments – the current cache block, string to store the response,
 *                      and the current time count (for LRU)
 * Return value – the length of the response string
 *
 */
size_t read_response(node *curr, char *contents, size_t time) {
    size_t length = curr->size;

    // updates the age (recently accessed)
    curr->age = time;

    memcpy(contents, curr->obj, length);
    return length;
}

/**
 * @brief Adds new block into cache.
 *
 * This function first checks if there is enough space for the new response
 * object. If not, blocks are evicted (LRU policy) until there is.
 *
 * A new cache node/block struct is allocated and initialized. The request
 * URI and response string are copied into their respective fields.
 * New blocks are inserted at the head of the cache list for simplicity.
 *
 * Function arguments – the working cache, request URI, the corresponding
 *                      response string, the length of response str, and the
 *                      current time count (used for LRU)
 *
 */
void add_to_cache(cache_t *cache, char *req_uri, char *response, size_t size,
                  size_t time) {
    int uri_len = strlen(req_uri);

    // checks if enough space in cache
    while (size > cache->slots) {
        node *rem_block = evict_age(cache);
        if (rem_block != NULL) {
            remove_from_cache(cache, rem_block);
        }
    }

    node *elem = Malloc(sizeof(node));

    // copy over request URI (key)
    elem->uri = Calloc(uri_len + 1, sizeof(char));
    strncpy(elem->uri, req_uri, uri_len);

    // copy over response string (value)
    elem->obj = Calloc(size, sizeof(char));
    memcpy(elem->obj, response, size);

    // update relevant fields
    elem->size = size;
    elem->age = time;
    cache->slots -= size;

    // insert into start of cache list
    elem->prev = NULL;
    elem->next = cache->head;
    if (cache->head != NULL) {
        cache->head->prev = elem;
    }
    cache->head = elem;

    return;
}

/**
 * @brief Determines the block to evict in the cache (LRU policy).
 *
 * This function steps through the entire cache list to determine the block
 * with the minimum age (ie. it was least recently used).
 *
 * Function arguments – the working cache
 * Return value – pointer to the block to be evicted
 *
 */
node *evict_age(cache_t *cache) {
    size_t currmin = MIN;
    node *evict = NULL;
    node *start = cache->head;

    while (start != NULL) {
        if (start->age < currmin) {
            currmin = start->age;
            evict = start;
        }
        start = start->next;
    }
    return evict;
}

/**
 * @brief Removes the designated cache block from the cache.
 *
 * This function traverses the cache linked list to find the node to be
 * evicted.
 * Cases on if the block is at the head, the middle, or the end, to manipulate
 * the prev/next pointers correctly.
 * The block and its strings are freed.
 *
 * Function arguments – the working cache, and the node/block to remove
 *
 */
void remove_from_cache(cache_t *cache, node *remove) {
    node *start = cache->head;

    while (start != NULL) {

        // found cache block
        if (start == remove) {

            // case 1: block is at the front
            if (start->prev == NULL) {
                cache->head = start->next;
                if (start->next != NULL) {
                    start->next->prev = NULL;
                    start->next = NULL;
                }
                // case 2: block is at the end
            } else if (start->next == NULL) {
                start->prev->next = NULL;
                start->prev = NULL;

                // case 3: block is in the middle
            } else {
                start->prev->next = start->next;
                start->next->prev = start->prev;
                start->prev = NULL;
                start->next = NULL;
            }
            cache->slots += start->size;

            // free fields, and cache block
            Free(start->uri);
            Free(start->obj);
            Free(start);
            return;
        }
        start = start->next;
    }
    return;
}

/**
 * @brief Frees the cache struct, along with all cache blocks.
 *
 * This function frees all cache blocks still in the cache.
 * Each block's request URI and response strings are freed.
 * The cache struct is freed at the end.
 *
 * Function arguments – the working cache
 *
 */
void free_cache(cache_t *cache) {
    node *prev;
    node *start = cache->head;

    while (start != NULL) {
        prev = start;
        start = start->next;
        Free(prev->uri);
        Free(prev->obj);
        Free(prev);
    }
    Free(cache);

    return;
}
