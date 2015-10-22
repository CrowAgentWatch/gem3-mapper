/*
 * PROJECT: GEMMapper
 * FILE: filtering_region.h
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef FILTERING_REGION_H_
#define FILTERING_REGION_H_

#include "essentials.h"
#include "sequence.h"
#include "text_collection.h"
#include "match_scaffold.h"

/*
 * Filtering Region
 */
typedef enum {
  filtering_region_pending=0,               // On-hold state (unverified)
  filtering_region_unverified=1,            // Region still unverified
  filtering_region_verified_discarded=2,    // Region verified but too distant
  filtering_region_accepted=3,              // Region accepted by pre-filters
  filtering_region_accepted_subdominant=4,  // Region accepted by pre-filters but sub-dominant
  filtering_region_aligned=5,               // Region aligned
  filtering_region_aligned_subdominant=6,   // Region aligned but sub-dominant in the end
  filtering_region_aligned_unbounded=7,     // Region aligned usign unbounded alignment
} filtering_region_status_t;
extern const char* filtering_region_status_label[8];
typedef struct {
  /* State */
  filtering_region_status_t status;
  /* Text-trace */
  uint64_t text_trace_offset;
  /* Location */
  uint64_t begin_position;             // Region effective begin position (adjusted to error boundaries)
  uint64_t end_position;               // Region effective end position   (adjusted to error boundaries)
  uint64_t base_position_offset;       // Offset to base filtering position (Begin position without error boundary correction)
  /* Regions Matching */
  match_scaffold_t match_scaffold;     // Matching regions supporting the filtering region (Scaffolding)
  /* Alignment distance */
  uint64_t align_distance;             // Distance (In case of approximate: max-bound)
  uint64_t align_distance_min_bound;   // Approximated distance (min-bound)
  uint64_t align_match_begin_column;   // Match begin column (inclusive)
  uint64_t align_match_end_column;     // Matching column (inclusive)
  /* Footprint */
  uint64_t footprint;                  // Footprint checksum
} filtering_region_t;
typedef struct {
  /* Filtering region */
  filtering_region_t* filtering_region;
  /* Location */
  sequence_end_t sequence_end;
  strand_t strand;
  uint64_t position;
} filtering_region_locator_t;
typedef struct {
  /* Location */
  uint64_t begin_position;
  uint64_t end_position;
} verified_region_t;

/*
 * Sorting
 */
void filtering_region_sort_regions_matching(const filtering_region_t* const filtering_region);
void filtering_region_locator_sort_positions(vector_t* const filtering_region_locators);

/*
 * Display
 */
void filtering_region_print(
    FILE* const stream,filtering_region_t* const region,
    const text_collection_t* const text_collection,const bool print_matching_regions);

#endif /* FILTERING_REGION_H_ */
