/*
 * PROJECT: GEMMapper
 * FILE: sparse_array_locator.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef SPARSE_ARRAY_LOCATOR_H_
#define SPARSE_ARRAY_LOCATOR_H_

#include "utils/essentials.h"
#include "utils/segmented_vector.h"
#include "stats/stats_vector.h"

/*
 * Sparse Array Locator
 */
typedef struct {
  /* Meta-Data */
  uint64_t total_length;     // Total length of the bitmap
  uint64_t total_size;       // Total Size (Bytes)
  uint64_t num_mayor_blocks; // Total Mayor Blocks
  uint64_t num_minor_blocks; // Total Minor Blocks
  /* Data Stored */
  uint64_t* mayor_counters;  // MAYOR_COUNTERS := [64MayorCounter]...[64MayorCounter]
  uint64_t* bitmap;          // MINOR_BLOCKS   := [48Bitmap][16MinorCounter]...[48Bitmap][16MinorCounter]
  /* MM */
  mm_t* mm;
  /* Space Range [Idx_start,Idx_end) */
  uint64_t first_block_offset; // First block offset
  uint64_t idx_offset;         // Global offset
} sparse_array_locator_t;
/*
 * Sparse Array Locator Builder
 */
typedef struct {
  /* Position & current Block */
  uint64_t position;
  uint64_t current_block;
  uint64_t current_count;
  /* Counting */
  uint64_t minor_counter_accumulated;
  uint64_t mayor_counter_accumulated;
  /* Memory */
  svector_t* minor_blocks;
  svector_iterator_t minor_blocks_iterator;
  vector_t* mayor_counters;
} sparse_array_locator_builder_t;
/*
 * Sparse Array Locator Stats
 */
typedef struct {
  /* Locator Stats */
  uint64_t locator_length;
  uint64_t marked_positions;
  /* Bitmaps */
  stats_vector_t* ones_density;
} sparse_array_locator_stats_t;

/*
 * Loader/Setup
 */
sparse_array_locator_t* sparse_array_locator_read(fm_t* const restrict file_manager);
sparse_array_locator_t* sparse_array_locator_read_mem(mm_t* const restrict memory_manager);
void sparse_array_locator_delete(sparse_array_locator_t* const restrict locator);

/*
 * Accessors
 */
uint64_t sparse_array_locator_get_size(sparse_array_locator_t* const restrict locator);

bool sparse_array_locator_is_marked(
    sparse_array_locator_t* const restrict locator,
    const uint64_t position);
uint64_t sparse_array_locator_get_erank(
    sparse_array_locator_t* const restrict locator,
    const uint64_t position);
bool sparse_array_locator_get_erank_if_marked(
    sparse_array_locator_t* const restrict locator,
    const uint64_t position,
    uint64_t* const restrict erank);
bool sparse_array_locator_get_erank__marked(
    sparse_array_locator_t* const restrict locator,
    const uint64_t position,
    uint64_t* const restrict erank);

/*
 * Builder
 */
// Static builder
sparse_array_locator_t* sparse_array_locator_new(
    const uint64_t idx_begin,
    const uint64_t idx_end);
void sparse_array_locator_mark(
    sparse_array_locator_t* const restrict locator,
    const uint64_t position);
void sparse_array_locator_write(
    fm_t* const restrict file_manager,
    sparse_array_locator_t* const restrict locator);
void sparse_array_locator_merge__write(
    fm_t* const restrict file_manager,
    sparse_array_locator_t** const restrict locator,
    const uint64_t num_locators);
// Dynamic Builder
sparse_array_locator_builder_t* sparse_array_locator_builder_new(mm_slab_t* const restrict mm_slab);
void sparse_array_locator_builder_delete(
    sparse_array_locator_builder_t* const restrict locator_builder);
void sparse_array_locator_builder_next(
    sparse_array_locator_builder_t* const restrict locator_builder,const bool mark_position);
void sparse_array_locator_builder_write(
    fm_t* const restrict file_manager,sparse_array_locator_builder_t* const restrict locator_builder);

/*
 * Display
 */
void sparse_array_locator_print(
    FILE* const restrict stream,
    const sparse_array_locator_t* const restrict locator,
    const bool display_content);

/*
 * Stats // TODO Enhance
 */
sparse_array_locator_stats_t* sparse_array_locator_stats_new();
void sparse_array_locator_stats_delete(
    sparse_array_locator_stats_t* const restrict sparse_array_locator_stats);
void sparse_array_locator_stats_calculate(
    sparse_array_locator_stats_t* const restrict sparse_array_locator_stats,
    sparse_array_locator_t* const restrict locator);
void sparse_array_locator_stats_print(
    FILE* const restrict stream,
    const char* const restrict sparse_array_locator_stats_tag,
    sparse_array_locator_stats_t* const restrict locator_stats);

#endif /* SPARSE_ARRAY_LOCATOR_H_ */
