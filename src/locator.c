/*
 * PROJECT: GEMMapper
 * FILE: locator.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Provides functionality as to recover sequence location from an archive of sequences
 */

#include "locator.h"

/*
 * Constants
 */
#define LOCATOR_NUM_INITIAL_TAGS 128

/*
 * Setup
 */
#define LOCATOR_CALCULATE_SIZES(locator) \
  const uint64_t intervals_locator_size = locator->num_intervals*sizeof(locator_interval_t); \
  const uint64_t tag_locator_size = locator->num_tags*sizeof(locator_tag_t); \
  const uint64_t tag_buffer_size = locator->tags_buffer_size;
GEM_INLINE locator_t* locator_read_mem(mm_t* const memory_manager) {
  // Allocate locator
  locator_t* const locator = mm_alloc(locator_t);
  // Read locator
  const uint64_t locator_model_no = mm_read_uint64(memory_manager);
  gem_cond_fatal_error(locator_model_no!=LOCATOR_MODEL_NO,
      LOCATOR_WRONG_MODEL_NO,locator_model_no,(uint64_t)LOCATOR_MODEL_NO);
  locator->num_intervals = mm_read_uint64(memory_manager);
  locator->num_tags = mm_read_uint64(memory_manager);
  locator->tags_buffer_size = mm_read_uint64(memory_manager);
  // Calculate sizes
  LOCATOR_CALCULATE_SIZES(locator);
  // Null Locator memory
  locator->mm = NULL;
  // Read tags
  locator->tags_buffer = mm_read_mem(memory_manager,tag_buffer_size);
  // Read tag-locator
  locator->tag_locator = mm_read_mem(memory_manager,tag_locator_size);
  // Read intervals
  locator->intervals = mm_read_mem(memory_manager,intervals_locator_size);
  // Return
  return locator;
}
GEM_INLINE void locator_delete(locator_t* const locator) {
  // Free memory managers
  if (locator->mm) mm_bulk_free(locator->mm);
  // Free handler
  mm_free(locator);
}
/*
 * Locator Accessors
 */
GEM_INLINE uint64_t locator_get_size(locator_t* const locator) {
  const uint64_t intervals_locator_size = locator->num_intervals*sizeof(locator_interval_t);
  const uint64_t tag_locator_size = locator->num_tags*sizeof(locator_tag_t);
  const uint64_t tag_buffer_size = locator->tags_buffer_size;
  return intervals_locator_size + tag_locator_size + tag_buffer_size;
}
GEM_INLINE locator_interval_t* locator_get_interval(const locator_t* const locator,const uint64_t interval_index) {
  gem_fatal_check(interval_index >= locator->num_intervals,
      LOCATOR_INTERVAL_INDEX_OOB,interval_index,(uint64_t)0,locator->num_intervals);
  return locator->intervals + interval_index;
}
/*
 * Interval Accessors
 */
GEM_INLINE char* locator_interval_get_tag(const locator_t* const locator,const locator_interval_t* const interval) {
  return locator->tags_buffer+locator->tag_locator[interval->tag_id].offset;
}
GEM_INLINE uint64_t locator_interval_get_index_length(const locator_interval_t* const interval) {
  return (interval->end_position - interval->begin_position);
}
GEM_INLINE uint64_t locator_interval_get_text_length(const locator_interval_t* const interval) {
  return interval->sequence_length;
}
/*
 * Locating functions
 */
GEM_INLINE uint64_t locator_lookup_interval_index(const locator_t* const locator,const uint64_t index_position) {
  // Binary search of the interval
  const locator_interval_t* const intervals = locator->intervals;
  uint64_t lo = 0;
  uint64_t hi = locator->num_intervals-1;
  do {
    const uint64_t half = (hi+lo+1)/2;
    if (intervals[half].begin_position > index_position) {
      hi = half-1;
    } else {
      lo = half;
    }
  } while (hi > lo);
  // Return Interval
  GEM_INTERNAL_CHECK(intervals[lo].begin_position <= index_position &&
      index_position < intervals[lo].end_position,"Locator-Interval Binary Search. Wrong Boundaries");
  return lo;
}
GEM_INLINE locator_interval_t* locator_lookup_interval(const locator_t* const locator,const uint64_t index_position) {
  return locator->intervals + locator_lookup_interval_index(locator,index_position);
}
/*
 * Map functions (High level mapping)
 */
// Direct Locator (Position-to-location mapping)
GEM_INLINE void locator_map(const locator_t* const locator,const uint64_t index_position,location_t* const location) {
  // Find interval
  const locator_interval_t* const interval = locator_lookup_interval(locator,index_position);
  // Fill @location
  if (interval->strand == Forward) {
    location->position = interval->sequence_offset + (index_position-interval->begin_position);
    location->strand = Forward;
    location->tag = locator_interval_get_tag(locator,interval);
  } else { // Reverse
    location->position = interval->sequence_offset +
        (interval->end_position-interval->begin_position) - (index_position-interval->begin_position);
    location->strand = Reverse;
    location->tag = locator_interval_get_tag(locator,interval);
  }
  location->bs_strand = interval->bs_strand;
}
// Inverse Locator (Location-to-position mapping)
GEM_INLINE uint64_t locator_inverse_map(
    locator_t* const locator,const uint8_t* const tag,
    const strand_t strand,const int64_t text_position) {
  locator_interval_t* const intervals = locator->intervals;
  const uint64_t num_intervals = locator->num_intervals;
  uint64_t i;
//  if (strand==Forward) {
//    for (i=0;i<num_intervals;++i) {
//      if (intervals[i].tag_id > 0 && intervals[i].sequence_offset <= text_position &&
//          text_position < intervals[i].sequence_offset+intervals[i].sequence_length) {
//        if (gem_streq((char*)tag,locator_interval_get_tag(locator,intervals+i))) {
//          return intervals[i].begin_position + (text_position-intervals[i].sequence_offset);
//        }
//      }
//    }
//  } else {
//    for (i=0;i<num_intervals;++i) {
//      if (intervals[i].tag_id < 0 && intervals[i].sequence_offset <= text_position &&
//          text_position < intervals[i].sequence_offset+intervals[i].sequence_length) {
//        if (gem_streq((char*)tag,locator_interval_get_tag(locator,intervals+i))) {
//          return intervals[i].end_position - (text_position-intervals[i].sequence_offset);
//        }
//      }
//    }
//  }
  // TODO Improve by doing binary search
  // TODO Improve by doing stranded search
  for (i=0;i<num_intervals;++i) {
    if (intervals[i].sequence_offset <= text_position &&
        text_position < intervals[i].sequence_offset+intervals[i].sequence_length) {
      if (gem_streq((char*)tag,locator_interval_get_tag(locator,intervals+i))) {
        return intervals[i].begin_position + (text_position-intervals[i].sequence_offset);
      }
    }
  }
  gem_fatal_error(LOCATOR_INVERSE_NOT_FOUND);
  return 0;
}
/*
 * Display
 */
GEM_INLINE void locator_interval_print(FILE* const stream,locator_interval_t* const interval,const char* const interval_tag) {
  // Print interval type
  switch (interval->type) {
    case locator_interval_chromosomal_assembly: fprintf(stream,"chromosome\t"); break;
    case locator_interval_unlocalized_contigs: fprintf(stream,"unlocalized\t"); break;
    case locator_interval_unplaced_contig: fprintf(stream,"unplaced\t"); break;
    case locator_interval_alt_contig: fprintf(stream,"alt-contig\t"); break;
    case locator_interval_uncalled: fprintf(stream,"uncalled\t"); break;
    case locator_interval_variant: fprintf(stream,"variant\t"); break;
    default: GEM_INVALID_CASE(); break;
  }
  // Print interval information
  fprintf(stream,"Idx=[%"PRIu64",%"PRIu64")\tText=%s:%c:[%"PRIu64",%"PRIu64")",
      interval->begin_position,interval->end_position,interval_tag,
      interval->strand==Forward ? '+' : '-',interval->sequence_offset,
      interval->sequence_offset+interval->sequence_length);
  // Print BS-Strand info (if any)
  switch (interval->bs_strand) {
    case bs_strand_none: fprintf(stream,"\n"); break;
    case bs_strand_C2T:  fprintf(stream,"\tBisulfite(C2T)\n"); break;
    case bs_strand_G2A:  fprintf(stream,"\tBisulfite(G2A)\n"); break;
    case bs_strand_mixed: // fprintf(stream,"\tBisulfite(Mixed)\n"); break;
    default: GEM_INVALID_CASE(); break;
  }
}
GEM_INLINE void locator_print_summary(
    FILE* const stream,const uint64_t num_intervals,
    const uint64_t num_tags,const uint64_t tags_buffer_size) {
  // Print locator info
  tab_fprintf(stream,"  => Intervals.Size %"PRIu64" MB (%"PRIu64" intervals x %"PRIu64"B per interval)\n",
      CONVERT_B_TO_MB(sizeof(locator_interval_t)*num_intervals),num_intervals,sizeof(locator_interval_t));
  tab_fprintf(stream,"  => Tags.Locator.Size %"PRIu64" B (%"PRIu64" locators x %"PRIu64"B per locator)\n",
      CONVERT_B_TO_MB(sizeof(locator_tag_t)*num_tags),num_tags,sizeof(locator_tag_t));
  tab_fprintf(stream,"  => Tags.Buffer.Size %"PRIu64" B\n",tags_buffer_size);
}
GEM_INLINE void locator_print(FILE* const stream,const locator_t* const locator,const bool display_intervals) {
  GEM_CHECK_NULL(stream);
  // Print locator info
  tab_fprintf(stream,"[GEM]>Locator\n");
  locator_print_summary(stream,locator->num_intervals,locator->num_tags,locator->tags_buffer_size);
  // Print intervals
  if (display_intervals) {
    tab_fprintf(stream,"  => Locator.intervals\n");
    uint64_t i;
    for (i=0;i<locator->num_intervals;++i) {
      // Print interval information
      locator_interval_t* const interval = locator->intervals + i;
      tab_fprintf(stream,"       [%"PRIu64"] ",i);
      const char* const interval_tag = locator_interval_get_tag(locator,interval);
      locator_interval_print(stream,interval,interval_tag);
    }
  }
  // Flush
  fflush(stream);
}
