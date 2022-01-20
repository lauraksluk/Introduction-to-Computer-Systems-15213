/*
 * Name: Laura (Kai Sze) Luk
 * AndrewID: kluk
 *
 * cache.h – Header file for the web-content cache implementation
 *           for the proxy.
 *
 * An overarching cache struct, containing the empty spaces remaining,
 * and a pointer to a doubly linked-list of cache blocks.
 * Each cache block holds prev/next pointers in the cache, the
 * request URI (ie. key), the contents of the response (ie. value),
 * an age, and the length of the response string.
 *      - the smaller the age is, the least recently used that block was
 *      - length is recorded for accurate binary data transmission
 *
 */

#include "csapp.h"

// maximum cache and object sizes
#define MAX_CACHE_SIZE (1024 * 1024)
#define MAX_OBJECT_SIZE (100 * 1024)

// dummy minimum value (assumes age per cache block doesn't exceed it)
#define MIN 100000000

// cache block/node struct
typedef struct cache_node {
    struct cache_node *prev;
    struct cache_node *next;
    char *uri;
    char *obj;
    size_t age;  // for LRU
    size_t size; // length of response string
} node;

// cache struct
typedef struct {
    node *head;   // pointer to start of cache
    size_t slots; // remaining empty slots (bytes)
} cache_t;

// function declarations
cache_t *init_cache();
node *search_cache(cache_t *cache, char *req_uri);
size_t read_response(node *curr, char *contents, size_t time);
void add_to_cache(cache_t *cache, char *req_uri, char *response, size_t size,
                  size_t time);
node *evict_age(cache_t *cache);
void remove_from_cache(cache_t *cache, node *remove);
void free_cache(cache_t *cache);
