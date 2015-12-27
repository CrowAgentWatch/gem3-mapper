/*
 * PROJECT: GEMMapper
 * FILE: approximate_search_generate_candidates.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#include "approximate_search_generate_candidates.h"
#include "approximate_search_control.h"
#include "filtering_candidates_process.h"
#include "filtering_candidates_process_buffered.h"
#include "filtering_candidates_verify.h"
#include "filtering_candidates_align.h"
#include "region_profile.h"
#include "region_profile_adaptive.h"
#include "region_profile_schedule.h"
#include "nsearch.h"

/*
 * Debug
 */
#define DEBUG_REGION_SCHEDULE_PRINT GEM_DEEP_DEBUG

/*
 * Profile
 */
#define PROFILE_LEVEL PMED

/*
 * Benchmark
 */
#ifdef CUDA_BENCHMARK_GENERATE_DECODE_CANDIDATES
FILE* benchmark_decode_candidates = NULL;
#endif

/*
 * Constants
 */
#define ZERO_ERRORS 0
#define ONE_ERROR   1
#define TWO_ERRORS  2

/*
 * Generate Candidates
 */
uint64_t approximate_search_generate_region_candidates(
    fm_index_t* const fm_index,uint8_t* const key,region_search_t* const region,
    const uint64_t filtering_threshold,filtering_candidates_t* const filtering_candidates,
    interval_set_t* const intervals_result,mm_stack_t* const mm_stack) {
  bool perform_search = true;
  uint64_t candidates;
  // Filter up to n errors (n>=2)
  while (region->degree >= REGION_FILTER_DEGREE_TWO) {
    PROFILE_START(GP_AS_GENERATE_CANDIDATES_SEARCH_D2,PROFILE_LEVEL);
    PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D2);
    if (perform_search) {
      interval_set_clear(intervals_result);
      neighborhood_search(fm_index,key+region->begin,region->end-region->begin,
          region->degree-1,intervals_result,mm_stack);
      perform_search = false;
    }
    PROFILE_STOP(GP_AS_GENERATE_CANDIDATES_SEARCH_D2,PROFILE_LEVEL);
    candidates = interval_set_count_intervals_length(intervals_result);
    if (candidates <= filtering_threshold) {
      PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D2_HIT);
      PROF_ADD_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D2_HIT_CANDIDATES,candidates);
      filtering_candidates_add_interval_set(filtering_candidates,intervals_result,region->begin,region->end,mm_stack);
      return region->degree; // Return filtered-degree (errors-allowed)
    }
    --(region->degree);
  }
  // Filter up to 1 errors
  if (region->degree == REGION_FILTER_DEGREE_ONE) {
    PROFILE_START(GP_AS_GENERATE_CANDIDATES_SEARCH_D1,PROFILE_LEVEL);
    PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D1);
    if (perform_search) {
      interval_set_clear(intervals_result);
      neighborhood_search(fm_index,key+region->begin,region->end-region->begin,ONE_ERROR,intervals_result,mm_stack);
      perform_search = false;
    }
    PROFILE_STOP(GP_AS_GENERATE_CANDIDATES_SEARCH_D1,PROFILE_LEVEL);
    candidates = interval_set_count_intervals_length_thresholded(intervals_result,ONE_ERROR);
    if (candidates <= filtering_threshold) {
      PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D1_HIT);
      PROF_ADD_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D1_HIT_CANDIDATES,candidates);
      filtering_candidates_add_interval_set_thresholded(filtering_candidates,
          intervals_result,region->begin,region->end,ONE_ERROR,mm_stack);
      return REGION_FILTER_DEGREE_ONE; // Return filtered-degree (errors-allowed)
    }
    --(region->degree);
  }
  // Filter exact
  if (region->degree == REGION_FILTER_DEGREE_ZERO) {
    PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D0_HIT);
    PROF_ADD_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D0_HIT_CANDIDATES,region->hi-region->lo);
    filtering_candidates_add_interval(filtering_candidates,
        region->lo,region->hi,region->begin,region->end,ZERO_ERRORS,mm_stack);
    return REGION_FILTER_DEGREE_ZERO; // Return filtered-degree (errors-allowed)
  }
  /*
   * Otherwise, ignore the region
   */
  return 0; // Return filtered-degree (errors-allowed)
}
void approximate_search_generate_exact_candidates(approximate_search_t* const search,matches_t* const matches) {
  PROFILE_START(GP_AS_GENERATE_CANDIDATES,PROFILE_LEVEL);
  // Parameters
  region_profile_t* const region_profile = &search->region_profile;
  filtering_candidates_t* const filtering_candidates = search->filtering_candidates;
  mm_stack_t* const mm_stack = search->mm_stack;
  // Pattern
  pattern_t* const pattern = &search->pattern;
  const uint64_t num_wildcards = pattern->num_wildcards;
  // Generate candidates for each region
  uint64_t errors_allowed = 0; // Number of errors allowed/generated/applied so far
  PROF_ADD_COUNTER(GP_AS_GENERATE_CANDIDATES_NUM_ELEGIBLE_REGIONS,region_profile->num_filtering_regions);
  REGION_LOCATOR_ITERATE(region_profile,region,position) {
    PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_PROCESSED);
    // Generate exact-candidates for the region
    if (region->degree == REGION_FILTER_DEGREE_ZERO) {
      PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D0_HIT);
      PROF_ADD_COUNTER(GP_AS_GENERATE_CANDIDATES_SEARCH_D0_HIT_CANDIDATES,region->hi-region->lo);
      filtering_candidates_add_interval(filtering_candidates,
          region->lo,region->hi,region->begin,region->end,ZERO_ERRORS,mm_stack);
      ++errors_allowed; // Increase errors-allowed
    }
  }
  // Set the minimum number of mismatches required
  approximate_search_update_mcs(search,errors_allowed + num_wildcards);
  region_profile->errors_allowed = errors_allowed;
  PROFILE_STOP(GP_AS_GENERATE_CANDIDATES,PROFILE_LEVEL);
}
void approximate_search_generate_inexact_candidates(
    approximate_search_t* const search,const bool dynamic_scheduling,
    const bool verify_ahead,matches_t* const matches) {
  PROFILE_START(GP_AS_GENERATE_CANDIDATES,PROFILE_LEVEL);
  /*
   * Filters all the regions up to the scheduled degree
   *  - Dynamic Scheduling: Assigns a filtering degree to each region as the search goes on
   *  - Verify Ahead: Verifies candidates each queried region. Thus, it can reduce the scope of the search
   */
  // Parameters
  const as_parameters_t* const as_parameters = search->as_parameters;
  const search_parameters_t* const parameters = as_parameters->search_parameters;
  const uint64_t filtering_threshold = parameters->filtering_threshold;
  const double proper_length = fm_index_get_proper_length(search->archive->fm_index);
  const uint64_t sensibility_error_length = parameters->filtering_region_factor*proper_length;
  // Approximate Search Data
  fm_index_t* const fm_index = search->archive->fm_index;
  region_profile_t* const region_profile = &search->region_profile;
  filtering_candidates_t* const filtering_candidates = search->filtering_candidates;
  interval_set_t* const intervals_result = search->interval_set;
  mm_stack_t* const mm_stack = search->mm_stack;
  // Pattern
  pattern_t* const pattern = &search->pattern;
  const uint64_t num_wildcards = pattern->num_wildcards;
  uint8_t* const key = search->pattern.key;
  // Region profile
  const uint64_t num_regions = region_profile->num_filtering_regions;
  const uint64_t num_standard_regions = region_profile->num_standard_regions;
  const uint64_t num_unique_regions = num_regions - region_profile->num_standard_regions;
  uint64_t num_standard_regions_left = num_standard_regions;
  uint64_t num_unique_regions_left = num_unique_regions;
  uint64_t errors_allowed = 0; // Number of errors allowed/generated/applied so far
  // Dynamic Scheduling (Pre-Sort regions)
  if (dynamic_scheduling) {
    region_profile_sort_by_estimated_mappability(&search->region_profile);
    gem_cond_debug_block(DEBUG_REGION_SCHEDULE_PRINT) {
      region_profile_schedule_filtering_adaptive(region_profile,search->max_complete_error,sensibility_error_length);
      region_profile_schedule_print(region_profile,search->max_complete_error,sensibility_error_length);
    }
  }
  // Generate candidates for each region
  PROF_ADD_COUNTER(GP_AS_GENERATE_CANDIDATES_NUM_ELEGIBLE_REGIONS,MIN(search->max_complete_error+1,num_regions));
  REGION_LOCATOR_ITERATE(region_profile,region,position) {
    PROF_INC_COUNTER(GP_AS_GENERATE_CANDIDATES_PROCESSED);
    // Dynamic Schedule
    if (dynamic_scheduling) {
      if (region->type == region_standard) {
        --num_standard_regions_left;
      } else {
        --num_unique_regions_left;
      }
      region_schedule_filtering_adaptive(
          region,num_standard_regions_left,num_unique_regions_left,
          search->max_complete_error,sensibility_error_length,errors_allowed);
    }
    // Generate candidates for the region
    errors_allowed += approximate_search_generate_region_candidates(fm_index,
        key,region,filtering_threshold,filtering_candidates,intervals_result,mm_stack);
    // Check candidates ahead (Dynamic scheduling+filtering)
    if (verify_ahead && asearch_control_filter_ahead_candidates(search,matches)) {
      PROFILE_PAUSE(GP_AS_GENERATE_CANDIDATES,PROFILE_LEVEL);
      PROFILE_START(GP_AS_GENERATE_CANDIDATES_DYNAMIC_FILTERING,PROFILE_LEVEL);
      filtering_candidates_process_candidates(filtering_candidates,search->archive,pattern,as_parameters,true,mm_stack);
      filtering_candidates_verify_candidates(filtering_candidates,search->archive,
          search->text_collection,pattern,as_parameters,matches,mm_stack);
      filtering_candidates_align_candidates(
          filtering_candidates,search->archive->text,search->archive->locator,
          search->text_collection,pattern,search->emulated_rc_search,as_parameters,false,matches,mm_stack);
      asearch_control_adjust_max_differences_using_strata(search,matches);
      PROFILE_STOP(GP_AS_GENERATE_CANDIDATES_DYNAMIC_FILTERING,PROFILE_LEVEL);
      PROFILE_CONTINUE(GP_AS_GENERATE_CANDIDATES,PROFILE_LEVEL);
    }
    // Check cut-off condition
    approximate_search_update_mcs(search,errors_allowed + num_wildcards);
    if (asearch_control_fulfilled(search,matches)) {
      PROF_ADD_COUNTER(GP_AS_GENERATE_CANDIDATES_SKIPPED,num_regions-(position+1));
      break;
    }
  }
  // Set the minimum number of mismatches required
  region_profile->errors_allowed = errors_allowed;
  PROFILE_STOP(GP_AS_GENERATE_CANDIDATES,PROFILE_LEVEL);
}
/*
 * Buffered Copy/Retrieve
 */
void approximate_search_generate_exact_candidates_buffered_print_benchmark(approximate_search_t* const search);
void approximate_search_generate_exact_candidates_buffered_copy(
    approximate_search_t* const search,gpu_buffer_fmi_decode_t* const gpu_buffer_fmi_decode) {
  // Parameters
  region_profile_t* const region_profile = &search->region_profile;
  const uint64_t num_filtering_regions = region_profile->num_filtering_regions;
  // Store Buffer Position
  search->gpu_buffer_fmi_decode_offset = gpu_buffer_fmi_decode_get_num_queries(gpu_buffer_fmi_decode);
  search->gpu_buffer_fmi_decode_total = region_profile->total_candidates;
  // Copy all candidates positions
  uint64_t i;
  for (i=0;i<num_filtering_regions;++i) {
    region_search_t* const filtering_region = region_profile->filtering_region + i;
    if (filtering_region->degree==REGION_FILTER_DEGREE_ZERO) {
      uint64_t bwt_position;
      for (bwt_position=filtering_region->lo;bwt_position<filtering_region->hi;++bwt_position) {
        gpu_buffer_fmi_decode_add_query(gpu_buffer_fmi_decode,bwt_position);
      }
    }
  }
  // BENCHMARK
  #ifdef CUDA_BENCHMARK_GENERATE_DECODE_CANDIDATES
  approximate_search_generate_exact_candidates_buffered_print_benchmark(search);
  #endif
}
void approximate_search_generate_exact_candidates_buffered_retrieve(
    approximate_search_t* const search,gpu_buffer_fmi_decode_t* const gpu_buffer_fmi_decode) {
  // Parameters
  region_profile_t* const region_profile = &search->region_profile;
  const uint64_t num_filtering_regions = region_profile->num_filtering_regions;
  const uint64_t key_length = search->pattern.key_length;
  const uint64_t boundary_error = search->pattern.max_effective_bandwidth;
  // Add all candidates positions
  uint64_t buffer_offset_begin = search->gpu_buffer_fmi_decode_offset;
  uint64_t i;
  for (i=0;i<num_filtering_regions;++i) {
    region_search_t* const filtering_region = region_profile->filtering_region + i;
    if (filtering_region->degree==REGION_FILTER_DEGREE_ZERO) {
      // Retrieve/Decode all pending candidates
      const uint64_t pending_candidates = filtering_region->hi - filtering_region->lo;
      if (pending_candidates < DECODE_NUM_POSITIONS_PREFETCHED) {
        filtering_candidates_decode_filtering_positions_buffered(
            search->filtering_candidates,search->archive->locator,search->archive->text,
            search->archive->fm_index,filtering_region->begin,filtering_region->end,
            filtering_region->lo,filtering_region->hi,key_length,boundary_error,
            gpu_buffer_fmi_decode,buffer_offset_begin);
      } else {
        filtering_candidates_decode_filtering_positions_buffered_prefetched(
            search->filtering_candidates,search->archive->locator,search->archive->text,
            search->archive->fm_index,filtering_region->begin,filtering_region->end,
            filtering_region->lo,filtering_region->hi,key_length,boundary_error,
            gpu_buffer_fmi_decode,buffer_offset_begin);
      }
      buffer_offset_begin += pending_candidates;
    }
  }
  // Process all candidates
  filtering_candidates_process_candidates_buffered(search->filtering_candidates,
      search->archive,&search->pattern,search->as_parameters,false,search->mm_stack);
  // Set state to verify-candidates
  search->processing_state = asearch_processing_state_candidates_processed;
}
/*
 * Display/Benchmark
 */
void approximate_search_generate_exact_candidates_buffered_print_benchmark(approximate_search_t* const search) {
#ifdef CUDA_BENCHMARK_GENERATE_DECODE_CANDIDATES
  // Parameters
  region_profile_t* const region_profile = &search->region_profile;
  const uint64_t num_filtering_regions = region_profile->num_filtering_regions;
  // Prepare benchmark file
  if (benchmark_decode_candidates==NULL) {
    benchmark_decode_candidates = fopen("gem3.decode.candidates.benchmark","w+");
  }
  // Add all candidates positions
  uint64_t i;
  for (i=0;i<num_filtering_regions;++i) {
    region_search_t* const filtering_region = region_profile->filtering_region + i;
    if (filtering_region->degree==REGION_FILTER_DEGREE_ZERO) {
      uint64_t bwt_position;
      for (bwt_position=filtering_region->begin;bwt_position<filtering_region->end;++bwt_position) {
        fm_index_decode_print_benchmark(benchmark_decode_candidates,search->archive->fm_index,bwt_position);
      }
    }
  }
#endif
}

