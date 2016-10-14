/*
 *  GEM-Mapper v3 (GEM3)
 *  Copyright (c) 2011-2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 *  This file is part of GEM-Mapper v3 (GEM3).
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * PROJECT: GEM-Mapper v3 (GEM3)
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 *   Region-Profile data structure provides support to store a key partition
 *   for a candidate generation stage within a filtering process
 */

#ifndef REGION_PROFILE_H_
#define REGION_PROFILE_H_

#include "utils/essentials.h"
#include "fm_index/fm_index.h"

/*
 * Constants
 */
#define REGION_MAX_REGIONS_FACTOR   10

// Degree to filter region
#define REGION_FILTER_NONE        0
#define REGION_FILTER_DEGREE_ZERO 1
#define REGION_FILTER_DEGREE_ONE  2
#define REGION_FILTER_DEGREE_TWO  3

typedef struct {
  // Region Profile
  uint64_t region_th;                  // Max. number of candidates allowed per region
  uint64_t max_steps;                  // Max. number of characters to explore to improve the region
  uint64_t dec_factor;                 // Decreasing factor per step in region exploration
  // Experimental
  uint64_t region_length;              // Region length (used in experimental tests)
} region_profile_model_t;
// Filtering regions
typedef struct {
  // Ranges of the region [begin,end)
  uint64_t begin;
  uint64_t end;
  // Filtering error
  uint64_t degree; // Degree assigned to this region
  // Region exact-search (candidates)
  uint64_t hi;
  uint64_t lo;
  // NS-Search limits
  uint64_t max;
  uint64_t min;
} region_search_t;
// Region Profile
typedef struct {
  uint64_t id;
  uint64_t value;
} region_locator_t;
typedef struct {
  /* Regions */
  region_search_t* filtering_region; // Filtering regions
  uint64_t max_expected_regions;     // Maximum regions expected (limit for static cases)
  uint64_t num_filtering_regions;    // Total number of filtering regions
  uint64_t num_filtered_regions;     // Total number of filtered regions
  /* Profile */
  uint64_t pattern_length;           // Length of the pattern
  uint64_t total_candidates;         // Total number of candidates (from exact matching-regions)
  bool candidates_limited;           // Limited number of candidates (due to selection)
  uint64_t max_region_length;        // Largest region length
  double kmer_frequency;
  /* MM */
  mm_stack_t* mm_stack;
} region_profile_t;

/*
 * Setup
 */
void region_profile_init(
    region_profile_t* const region_profile,
    const uint64_t pattern_length);
void region_profile_clear(
    region_profile_t* const region_profile);
void region_profile_inject_mm(
    region_profile_t* const region_profile,
    mm_stack_t* const mm_stack);

void region_profile_model_init(
    region_profile_model_t* const region_profile_model);

/*
 * Allocator
 */
void region_profile_allocate_regions(
    region_profile_t* const region_profile,
    const uint64_t num_regions);

/*
 * Accessors
 */
uint64_t region_get_num_regions(region_profile_t* const region_profile);
bool region_profile_has_exact_matches(region_profile_t* const region_profile);

/*
 * Region Query
 */
void region_profile_query_character(
    fm_index_t* const fm_index,
    rank_mquery_t* const rank_mquery,
    uint64_t* const lo,
    uint64_t* const hi,
    const uint8_t enc_char);
void region_profile_extend_last_region(
    region_profile_t* const region_profile,
    fm_index_t* const fm_index,
    const uint8_t* const key,
    const bool* const allowed_enc);

/*
 * Region Search Prepare
 */
void region_profile_fill_gaps(
    region_profile_t* const region_profile,
    const uint8_t* const key,
    const uint64_t key_length,
    const bool* const allowed_enc,
    const uint64_t num_wildcards);
void region_profile_merge_small_regions(
    region_profile_t* const region_profile,
    const uint64_t proper_length);
uint64_t region_profile_compute_max_complete_strata(
    region_profile_t* const region_profile);
void region_profile_compute_error_limits(
    region_profile_t* const region_profile,
    const uint64_t max_complete_strata,
    const uint64_t max_search_error);

/*
 * kmer Frequency
 */
void region_profile_compute_kmer_frequency(
    region_profile_t* const region_profile,
    fm_index_t* const fm_index,
    const uint8_t* const key,
    const uint64_t key_length,
    const bool* const allowed_enc);

/*
 * Sort
 */
void region_profile_sort_by_candidates(region_profile_t* const region_profile);
void region_profile_sort_by_position(region_profile_t* const region_profile);

/*
 * Display
 */
void region_profile_print_region(
    FILE* const stream,
    region_search_t* const region,
    const uint64_t position,
    const bool display_error_limits);
void region_profile_print(
    FILE* const stream,
    const region_profile_t* const region_profile,
    const bool display_error_limits);

/*
 * Iterator
 */
#define REGION_PROFILE_ITERATE(region_profile,region,position) \
  const uint64_t num_filtering_regions = region_profile->num_filtering_regions; \
  region_search_t* region = region_profile->filtering_region; \
  uint64_t position; \
  for (position=0;position<num_filtering_regions;++position,++region)

#endif /* REGION_PROFILE_H_ */
