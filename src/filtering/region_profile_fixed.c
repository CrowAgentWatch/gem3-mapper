/*
 * PROJECT: GEMMapper
 * FILE: region_profile_fixed.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 *
 */

#include "filtering/region_profile_fixed.h"
#include "data_structures/pattern.h"
#include "fm_index/fm_index_search.h"

/*
 * Region Profile Partition
 */
void region_profile_generate_fixed_partition(
    region_profile_t* const region_profile,
    const uint8_t* const key,
    const uint64_t key_length,
    const bool* const allowed_enc,
    const uint64_t min_region_length) {
  // Init
  region_profile_clear(region_profile);
  // Profile Parameters
  uint64_t num_filtering_regions = 0;
  region_search_t* region_search = region_profile->filtering_region;
  // Traverse the key & delimit regions of @region_length
  uint64_t i, region_length = 0;
  region_search->begin = 0;
  for (i=0;i<key_length;++i) {
    // Cut-off
    if (num_filtering_regions >= region_profile->max_regions_allocated) break;
    // Get next character & check allowed
    const uint8_t enc_char = key[i];
    if (!allowed_enc[enc_char]) { // Skip bad characters
      region_search->begin = i+1; // Reset region
      region_length = 0;
    }
    ++region_length; // Add character
    if (region_length == min_region_length) {
      // Close region
      region_search->end = i+1;
      ++region_search;
      ++num_filtering_regions;
      // Start new region
      region_search->begin = i+1;
      region_length = 0;
    }
  }
  // Extend last region
  if (region_length>0 && num_filtering_regions>0) {
    region_search_t* const last_region_search = region_profile->filtering_region + (num_filtering_regions-1);
    if (region_search->begin==last_region_search->end) {
      last_region_search->end = key_length;
    }
  }
  // Close profile
  region_profile->num_filtering_regions = num_filtering_regions;
}
/*
 * Region Profile Schedule (query the region partition into the index)
 */
void region_profile_generate_fixed_query(
    region_profile_t* const region_profile,
    fm_index_t* const fm_index,
    const uint8_t* const key) {
  // Iterate over all regions
  const uint64_t fm_index_length = fm_index_get_length(fm_index);
  REGION_PROFILE_ITERATE(region_profile,region,position) {
    // Init
    uint64_t lo = 0, hi = fm_index_length;
    rank_mquery_t rank_mquery;
    rank_mquery_new(&rank_mquery);
    // Search region
    const int64_t region_begin = region->begin;
    const int64_t region_end = region->end;
    int64_t position;
    region->degree = 0;
    for (position=region_end-1;position>=region_begin;--position) {
      const uint8_t enc_char = key[position];
      region_profile_query_character(fm_index,&rank_mquery,&lo,&hi,enc_char);
      if (hi - lo == 0) {
        region->degree = region_end-position; // FIXME Field nzSteps
        break;
      }
    }
    // Store results
    if (region->degree==0) region->degree = region_end-region_begin; // FIXME Field nzSteps
    region->type = region_standard;
    region->lo = lo;
    region->hi = hi;
  }
}
/*
 * Display/Benchmark
 */
void region_profile_print_benchmark(
    FILE* const stream,
    const region_profile_t* const region_profile,
    fm_index_t* const fm_index,
    const uint8_t* key) {
  REGION_PROFILE_ITERATE(region_profile,region,position) {
    /*
     * <chunk_length> <chunk_string> <lo> <hi> <steps_perfomed_on_search>
     * Eg. 10 ACGTACGTTG 12 134 10
     */
    // Chunk length
    const uint64_t chunk_length = region->end-region->begin;
    fprintf(stream,"%"PRIu64"\t",chunk_length);
    // Chunk string
    uint64_t i;
    for (i=region->begin;i<region->end;++i) {
      fprintf(stream,"%c",dna_decode(key[i]));
    }
    // Results
    uint64_t lo, hi, steps;
    fm_index_bsearch_debug(fm_index,
        key+region->begin,region->end-region->begin,&hi,&lo,&steps);
    fprintf(stream,"\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\n",lo,hi,steps);
  }
  fflush(stream);
}
