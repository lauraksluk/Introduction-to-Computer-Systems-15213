// Laura (Kai Sze) Luk
// Email: kluk@andrew.cmu.edu
// AndrewID: kluk
//
// This program is a cache simulator that takes in parameters s, E, b
// for the cache and an address trace file, from the command line.
// It outputs the number of hits, misses, evictions,
// dirty bytes in the cache, and dirty bytes that were evicted.

#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MIN 100000000

// Struct to store all cache information to be updated in simulation.
// Time used to update LRU field in cache.
typedef struct {
    int time;
    long hits;
    long misses;
    long evicts;
    long dirtyEvicted;
} cacheInformation;

// Struct organization for lines in sets, making up cache.
// Struct holding bookkeeping information for a line in a set, and cache.
typedef struct {
    int valid;
    int dirty;
    int LRU;
    unsigned long int tag;
} setLine;

typedef struct {
    setLine *lines;
} cacheSet;

typedef struct {
    cacheSet *sets;
    int E;
    int s;
    int b;
    int B;
} cache;

// Creates new cache with parameters S, E, B.
cache *newCache(int S, int E, int B) {
    cache *new = malloc(sizeof(cache));
    if (new == NULL)
        return NULL;

    new->E = E;
    new->B = B;

    cacheSet *allSets = malloc(S * sizeof(cacheSet));
    if (allSets == NULL)
        return NULL;

    new->sets = allSets;
    for (int i = 0; i < S; i++) {

        setLine *linesInSet = calloc(E, sizeof(setLine));
        if (linesInSet == NULL)
            return NULL;

        new->sets[i].lines = linesInSet;
    }
    return new;
}

// Determines the line index inside a set that should be evicted.
// Function takes in current set and number of lines in the set.
// Uses LRU policy. Assumes LRU counters are not larger than 100000000.
int evict(cacheSet currSet, int E) {
    int min = MIN;
    int minIndex = 0;
    for (int i = 0; i < E; i++) {
        if (currSet.lines[i].LRU < min) {
            min = currSet.lines[i].LRU;
            minIndex = i;
        }
    }
    return minIndex;
}

// Determines number of dirty bytes in cache after simulation is over.
// Function takes in current working cache, and the number of sets in cache.
long dirtyInCache(cache *currCache, int S) {
    long result = 0;
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < currCache->E; j++) {
            if ((currCache->sets[i].lines[j].dirty) == 1) {
                result += (currCache->B);
            }
        }
    }
    return result;
}

// Frees data structures used in cache simulation.
// Function takes in current working cache, the cache information struct,
// and the number of sets in the cache.
void freeCache(cache *currCache, cacheInformation *cacheInfo, int S) {
    for (int i = 0; i < S; i++) {
        if (currCache->sets[i].lines != NULL) {
            free(currCache->sets[i].lines);
        }
    }
    if (currCache->sets != NULL)
        free(currCache->sets);
    if (currCache != NULL)
        free(currCache);
    if (cacheInfo != NULL)
        free(cacheInfo);
}

// Obtains tag from address trace.
// Function takes in input address and the current working cache.
unsigned long int getTag(unsigned long int addr, cache *currCache) {
    int setBits = currCache->s;
    int blockBits = currCache->b;
    int shift = setBits + blockBits;
    return ((addr >> shift) << shift) >> shift;
}

// Obtains set index from address trace.
// Function takes in input address and the current working cache.
unsigned int getSetIndex(unsigned long int addr, cache *currCache) {
    int setBits = currCache->s;
    int mask = (0x1U << setBits) - 1;
    int blockBits = currCache->b;
    unsigned int index = ((addr >> blockBits) << blockBits) >> blockBits;
    return index & mask;
}

// Adds line into cache during a miss.
// Function takes in current set, the input tag, the line index, and
// a flag indicating if the instruction was a store.
void addLine(cacheSet currSet, unsigned long int tag, int index, int sflag,
             int time) {
    currSet.lines[index].tag = tag;
    currSet.lines[index].LRU = time;
    currSet.lines[index].valid = 1;
    if (sflag == 1) {
        currSet.lines[index].dirty = 1;
    } else {
        currSet.lines[index].dirty = 0;
    }
    return;
}

// Runs cache simulation. Updates current cache info and current cache,
// with address from trace and a flag.
// Flag- informs function if instruction was a store.
void run(cacheInformation *cacheInfo, cache *currCache, unsigned long int addr,
         int sflag) {
    cacheInfo->time += 1;

    // set index to -1 to track empty space in cache.
    int emptyLineIndex = -1;

    // obtains tag and set index from address.
    unsigned long int traceTag = getTag(addr, currCache);
    unsigned int setIndex = getSetIndex(addr, currCache);

    cacheSet currSet = currCache->sets[setIndex];

    for (int i = 0; i < currCache->E; i++) {

        // hit (valid bit && tag matches).
        if (currSet.lines[i].valid == 1) {
            if (currSet.lines[i].tag == traceTag) {

                (cacheInfo->hits)++;
                currSet.lines[i].LRU = cacheInfo->time;

                // update dirty bit if store
                // (a load into a store remains dirty).
                if (sflag == 1) {
                    currSet.lines[i].dirty = 1;
                }
                return;
            }

            // find empty space (valid bit == 0).
        } else {
            if (emptyLineIndex == -1)
                emptyLineIndex = i;
            break;
        }
    }
    // miss (no return from hit).
    (cacheInfo->misses)++;

    // Empty spot available.
    if (emptyLineIndex != -1) {
        // add into cache.
        addLine(currSet, traceTag, emptyLineIndex, sflag, cacheInfo->time);
        return;

        // Full cache, need to evict.
    } else {
        (cacheInfo->evicts)++;
        int evictedLine = evict(currSet, currCache->E);

        // update dirty bytes evicted, if applicable.
        if ((currSet.lines[evictedLine].dirty) == 1) {
            cacheInfo->dirtyEvicted += (currCache->B);
        }
        // replace new line into cache.
        addLine(currSet, traceTag, evictedLine, sflag, cacheInfo->time);
        return;
    }
    return;
}

// Main function.
// Function takes in number of arguments and a pointer to the arguments.
// Obtains command line arguments, runs simulation, prints output.
int main(int argc, char *argv[]) {
    char opt, instr;
    char *trace;
    int numSets, blockSize, setBits, numLines, blockBits;
    unsigned long int addr;

    // obtains s, E, b parameters and address trace file.
    while ((opt = getopt(argc, argv, "s:E:b:t:")) > 0) {
        switch (opt) {
        case 's':
            setBits = atoi(optarg);
            break;
        case 'E':
            numLines = atoi(optarg);
            break;
        case 'b':
            blockBits = atoi(optarg);
            break;
        case 't':
            trace = optarg;
            break;
        default:
            printf("incorrect arguments\n");
            return 0;
        }
    }
    // calculate cache parameters and allocate/update cache.
    numSets = 1 << setBits;
    blockSize = 1 << blockBits;

    cache *currCache = newCache(numSets, numLines, blockSize);
    if (currCache == NULL)
        return 0;

    currCache->s = setBits;
    currCache->b = blockBits;

    // allocate cache information struct for simulation.
    // calloc was used to initialize fields to 0.
    cacheInformation *cacheInfo = calloc(1, sizeof(cacheInformation));
    if (cacheInfo == NULL)
        return 0;

    FILE *traceFile = fopen(trace, "r");
    if (traceFile == NULL) {
        printf("invalid trace file\n");
        return 0;
    }

    // parse through address trace file and runs each instruction.
    while (fscanf(traceFile, "%c %lx,%*d ", &instr, &addr) == 2) {

        switch (instr) {
        case 'L':
            // runs simulation (sflag = 0 because load).
            run(cacheInfo, currCache, addr, 0);
            break;
        case 'S':
            // runs simulation (sflag = 1 because store).
            run(cacheInfo, currCache, addr, 1);
            break;
        default:
            printf("invalid cache instruction");
            return 0;
        }
    }

    // calculates dirty bytes in cache after simulation is done.
    long dirty = dirtyInCache(currCache, numSets);

    // prints output summary.
    printSummary(cacheInfo->hits, cacheInfo->misses, cacheInfo->evicts, dirty,
                 cacheInfo->dirtyEvicted);

    // free cache data structures.
    freeCache(currCache, cacheInfo, numSets);
    fclose(traceFile);
    return 0;
}