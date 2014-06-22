#include "libi0/stddef.h"

// General data structures
// edge struct
#define EDGE_SIZE (2)
#define EDGE_OFFSET_SRC (0)
#define EDGE_OFFSET_DST (1)

// vertex struct
#define VERTEX_SIZE (3)
#define VERTEX_OFFSET_INDEGREE (0)
#define VERTEX_OFFSET_OUTDEGREE (1)
#define VERTEX_OFFSET_IN_NEIGHBORS (2)

// vertex value
#define VERTEX_VALUE_SIZE (1)

// max version of values
// MAX_VERSION > MAX_ITER for PageRank
// In general, it can be reused
#define VERTEX_VALUE_MAX_VERSION (32)

// store the input in the persistent memory
#define EDGES_ADDRESS (AMR_P_OFFSET_BEGIN)
// store at most 70M points
#define EDGES_MAX_LEN ((1024 * 1024 * 70) * (EDGE_SIZE))
// maximum number of vertices: 5M
#define VERTICES_MAX_CNT (1024 * 1024 * 5)
#define VERTICES_ADDRESS (EDGES_ADDRESS + EDGES_MAX_LEN * sizeof_long)
// maximum number of in_neighbers
#define IN_NEIGHBORS_ADDRESS (VERTICES_ADDRESS + VERTICES_MAX_CNT * VERTEX_SIZE * sizeof_long)
#define IN_NEIGHBORS_MAX_LEN (EDGES_MAX_LEN / EDGE_SIZE + 1)

// partitions
#define PARTITION_SIZE (4)
#define PARTITION_OFFSET_BEGIN (0)
#define PARTITION_OFFSET_END (1)
#define PARTITION_OFFSET_IN_BEGIN (2)
#define PARTITION_OFFSET_IN_END (3)

// number of tasks
#define NUM_TASKS (30)

// maximum concurrent tasks
#define MAX_CONCURRENT_TASKS 64

// Task status
#define TASK_STATUS_CREATED 0
#define TASK_STATUS_FINISHED 1

#define STICKY_CHECK_MAGIC_NUM (0x8829eff20)

#define CACHE_MAGIC_NUM DEJAVU_MEM_BEGIN
#define CACHE_VERTICES (DEJAVU_MEM_BEGIN + sizeof_long)
#define CACHE_IN_NEIGHBORS (CACHE_VERTICES + (VERTICES_MAX_CNT * VERTEX_SIZE)*sizeof_long)
#define CACHE_END (CACHE_IN_NEIGHBORS + (EDGES_MAX_LEN/2+1)*sizeof_long)

#define PRE_VALUE (PHASING_CACHE_ADDR)
#define CUR_VALUE (PHASING_CACHE_ADDR + VERTICES_MAX_CNT * VERTEX_VALUE_SIZE * sizeof_long)

// max iteration
#define MAX_ITER 30
// for debugging
// #define MAX_ITER 5

// Page Rank specific ones
#define PR_alpha (0.15)
#define PR_initial (1.0)

// cache static data
#define ENABLE_CACHING 1

// so that cache only the parts needed is enough
#define STATIC_TASK_ASSIGNMENT 1

// whether print out topk
#define PRINT_TOPK 1
#define TOPK_K 100

// program specific macros
#define TASKING_INFO 1
#define noDEBUG 1
#define noDEBUG_READINPUT 1
#define noDEBUG_BUILDING 1
#define noDEBUG_BUILDING_RESULT 1
#define noDEBUG_TASKING 1
#define noDEBUG_PR_CALC 1
#define noDEBUG_TOPK 1
