/*
 * PROJECT: GEMMapper
 * FILE: filtering_candidates.h
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef FILTERING_CANDIDATES_H_
#define FILTERING_CANDIDATES_H_

#include "essentials.h"
#include "interval_set.h"
#include "locator.h"
#include "filtering_region.h"
#include "filtering_region_cache.h"

/*
 * Candidate Position
 */
typedef struct {
  // Source Region
  uint64_t source_region_offset;
  locator_interval_t* locator_interval;
  // Decode data
  uint64_t decode_distance;
  uint64_t decode_sampled_pos;
  // Region location
  uint64_t region_index_position;
  uint64_t region_text_position;
  // Final position
  uint64_t begin_position;       // Region effective begin position (adjusted to error boundaries)
  uint64_t end_position;         // Region effective end position (adjusted to error boundaries)
  uint64_t base_position_offset; // Offset to base filtering position (Begin position without error boundary correction)
} filtering_position_t;

/*
 * Filtering Candidates
 */
typedef struct {
  /* Region Buffer */
  vector_t* regions_buffer;                   // Regions Buffer (region_t)
  /* Candidates */
  vector_t* filtering_positions;              // Candidate positions (filtering_position_t)
  vector_t* filtering_regions;                // Candidate regions (filtering_region_t)
  vector_t* discarded_regions;                // Discarded regions (filtering_region_t)
  vector_t* verified_regions;                 // Verified regions (verified_region_t)
  /* Cache */
  filtering_region_cache_t filtering_region_cache;
} filtering_candidates_t;

/*
 * Setup
 */
void filtering_candidates_init(filtering_candidates_t* const filtering_candidates);
void filtering_candidates_clear(filtering_candidates_t* const filtering_candidates);
void filtering_candidates_destroy(filtering_candidates_t* const filtering_candidates);

/*
 * Accessors
 */
uint64_t filtering_candidates_get_num_candidate_regions(const filtering_candidates_t* const filtering_candidates);
uint64_t filtering_candidates_count_candidate_regions(
    filtering_candidates_t* const filtering_candidates_end,
    const filtering_region_status_t filtering_region_status);

void filtering_candidates_set_all_regions_pending(filtering_candidates_t* const filtering_candidates);
void filtering_candidates_set_all_regions_unverified(filtering_candidates_t* const filtering_candidates);

/*
 * Adding candidate positions
 */
void filtering_candidates_add_interval(
    filtering_candidates_t* const filtering_candidates,
    const uint64_t interval_lo,const uint64_t interval_hi,
    const uint64_t region_begin_pos,const uint64_t region_end_pos,
    const uint64_t region_errors,mm_stack_t* const mm_stack);
void filtering_candidates_add_interval_set(
    filtering_candidates_t* const filtering_candidates,interval_set_t* const interval_set,
    const uint64_t region_begin_pos,const uint64_t region_end_pos,mm_stack_t* const mm_stack);
void filtering_candidates_add_interval_set_thresholded(
    filtering_candidates_t* const filtering_candidates,interval_set_t* const interval_set,
    const uint64_t region_begin_pos,const uint64_t region_end_pos,
    const uint64_t max_error,mm_stack_t* const mm_stack);

/*
 * Sorting
 */
void filtering_positions_sort_positions(vector_t* const filtering_positions);
void filtering_regions_sort_align_distance(vector_t* const filtering_regions);
void verified_regions_sort_positions(vector_t* const verified_regions);

/*
 * Display
 */
void filtering_candidates_print_regions(
    FILE* const stream,filtering_candidates_t* const filtering_candidates,
    const text_collection_t* const text_collection,const bool print_base_regions,
    const bool print_matching_regions);

#endif /* FILTERING_CANDIDATES_H_ */
