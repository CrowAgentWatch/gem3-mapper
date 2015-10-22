/*
 * PROJECT: GEMMapper
 * FILE: matches.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Data structure to store alignment matches {sequence,position,strand,CIGAR}
 */

#include "matches.h"
#include "matches_cigar.h"
#include "matches_classify.h"

/*
 * Constants
 */
#define MATCHES_INIT_INTERVAL_MATCHES  100
#define MATCHES_INIT_GLOBAL_MATCHES   1000
#define MATCHES_INIT_CIGAR_OPS        5000

/*
 * Setup
 */
GEM_INLINE matches_t* matches_new() {
  // Allocate handler
  matches_t* const matches = mm_alloc(matches_t);
  // Search-matches state
  matches->max_complete_stratum = ALL;
  // Matches Counters
  matches->counters = matches_counters_new();
  // Interval Matches
  matches->interval_matches = vector_new(MATCHES_INIT_INTERVAL_MATCHES,match_interval_t);
  // Position Matches
  matches->position_matches = vector_new(MATCHES_INIT_GLOBAL_MATCHES,match_trace_t);
  matches->begin_pos_matches = ihash_new();
  matches->end_pos_matches = ihash_new();
  // CIGAR buffer
  matches->cigar_vector = vector_new(MATCHES_INIT_CIGAR_OPS,cigar_element_t);
  // Init metrics
  matches_metrics_init(&matches->metrics);
  // Return
  return matches;
}
GEM_INLINE void matches_configure(matches_t* const matches,text_collection_t* const text_collection) {
  // Text Collection Buffer
  matches->text_collection = text_collection;
}
GEM_INLINE void matches_clear(matches_t* const matches) {
  matches->max_complete_stratum = ALL;
  matches_counters_clear(matches->counters);
  matches_metrics_init(&matches->metrics);
  vector_clear(matches->interval_matches);
  vector_clear(matches->position_matches);
  matches_index_clear(matches);
  vector_clear(matches->cigar_vector);
}
GEM_INLINE void matches_delete(matches_t* const matches) {
  // Delete all
  matches_counters_delete(matches->counters);
  vector_delete(matches->interval_matches);
  vector_delete(matches->position_matches);
  ihash_delete(matches->begin_pos_matches);
  ihash_delete(matches->end_pos_matches);
  vector_delete(matches->cigar_vector);
  // Delete handler
  mm_free(matches);
}
/*
 * Accessors
 */
GEM_INLINE bool matches_is_mapped(const matches_t* const matches) {
  return vector_get_used(matches->position_matches) > 0 || vector_get_used(matches->interval_matches) > 0;
}
GEM_INLINE void matches_recompute_metrics(matches_t* const matches) {
  // Parameters
  matches_metrics_t* const metrics = &matches->metrics;
  // ReCompute distance metrics
  const match_trace_t* match = matches_get_match_trace_buffer(matches);
  const uint64_t num_matches = matches_get_num_match_traces(matches);
  matches_metrics_init(metrics);
  uint64_t i;
  for (i=0;i<num_matches;++i,++match) {
    matches_metrics_update(metrics,match->distance,match->edit_distance,match->swg_score);
  }
}
GEM_INLINE uint64_t matches_get_first_stratum_matches(matches_t* const matches) {
  const uint64_t min_distance = matches_metrics_get_min_distance(&matches->metrics);
  return (min_distance==UINT32_MAX) ? 0 : matches_counters_get_count(matches->counters,min_distance);
}
GEM_INLINE uint64_t matches_get_subdominant_stratum_matches(matches_t* const matches) {
  const uint64_t first_stratum_matches = matches_get_first_stratum_matches(matches);
  return matches_counters_get_total_count(matches->counters) - first_stratum_matches;
}
/*
 * Counters
 */
//GEM_INLINE void matches_counters_add(
/*
 * Index
 */
GEM_INLINE void matches_index_match(
    matches_t* const matches, const uint64_t match_trace_offset,
    const uint64_t begin_position,const uint64_t effective_length,
    mm_stack_t* const mm_stack) {
  // Store begin/end positions of the match (as to fast index matches)
  uint64_t* const match_trace_offset_1 = mm_stack_malloc_uint64(mm_stack);
  *match_trace_offset_1 = match_trace_offset;
  ihash_insert(matches->begin_pos_matches,begin_position,match_trace_offset_1);
  uint64_t* const match_trace_offset_2 = mm_stack_malloc_uint64(mm_stack);
  *match_trace_offset_2 = match_trace_offset;
  ihash_insert(matches->end_pos_matches,begin_position+effective_length,match_trace_offset_2);
}
GEM_INLINE uint64_t* matches_lookup_match(
    matches_t* const matches,const uint64_t begin_position,const uint64_t effective_length) {
  uint64_t* match_trace_offset = ihash_get(matches->begin_pos_matches,begin_position,uint64_t);
  return (match_trace_offset!=NULL) ? match_trace_offset :
      ihash_get(matches->end_pos_matches,begin_position+effective_length,uint64_t);
}
GEM_INLINE void matches_index_clear(matches_t* const matches) {
  ihash_clear(matches->begin_pos_matches);
  ihash_clear(matches->end_pos_matches);
}
GEM_INLINE void matches_index_rebuild(matches_t* const matches,mm_stack_t* const mm_stack) {
  // Clear
  matches_index_clear(matches);
  // Re-index positions
  match_trace_t* const match = matches_get_match_trace_buffer(matches);
  const uint64_t num_matches = matches_get_num_match_traces(matches);
  uint64_t i;
  for (i=0;i<num_matches;++i) {
    matches_index_match(matches,i,match[i].match_alignment.match_position,
        match[i].match_alignment.effective_length,mm_stack);
  }
}
/*
 * Matches
 */
GEM_INLINE match_trace_t* matches_get_match_trace_buffer(const matches_t* const matches) {
  return vector_get_mem(matches->position_matches,match_trace_t);
}
GEM_INLINE match_trace_t* matches_get_match_trace(const matches_t* const matches,const uint64_t offset) {
  return vector_get_elm(matches->position_matches,offset,match_trace_t);
}
GEM_INLINE uint64_t matches_get_num_match_traces(const matches_t* const matches) {
  return vector_get_used(matches->position_matches);
}
GEM_INLINE void matches_get_clear_match_traces(const matches_t* const matches) {
  vector_clear(matches->position_matches);
}
GEM_INLINE cigar_element_t* match_trace_get_cigar_buffer(const matches_t* const matches,const match_trace_t* const match_trace) {
  return vector_get_elm(matches->cigar_vector,match_trace->match_alignment.cigar_offset,cigar_element_t);
}
GEM_INLINE uint64_t match_trace_get_cigar_length(const match_trace_t* const match_trace) {
  return match_trace->match_alignment.cigar_length;
}
GEM_INLINE uint64_t match_trace_get_event_distance(const match_trace_t* const match_trace) {
  return match_trace->distance;
}
GEM_INLINE int64_t match_trace_get_effective_length(
    matches_t* const matches,const uint64_t read_length,
    const uint64_t cigar_buffer_offset,const uint64_t cigar_length) {
  // Exact Match
  if (cigar_length==0) return read_length; // Even all-matching matches have CIGAR=1
  // Traverse CIGAR
  const cigar_element_t* cigar_element = vector_get_elm(matches->cigar_vector,cigar_buffer_offset,cigar_element_t);
  int64_t i, effective_length = read_length;
  for (i=0;i<cigar_length;++i,++cigar_element) {
    switch (cigar_element->type) {
      case cigar_ins:
        effective_length += cigar_element->length;
        break;
      case cigar_del:
        effective_length -= cigar_element->length;
        break;
      default:
        break;
    }
  }
  GEM_INTERNAL_CHECK(effective_length >= 0,"Match effective length must be positive");
  return effective_length;
}
/*
 * Adding Matches
 */
GEM_INLINE void match_trace_locate(match_trace_t* const match_trace,const locator_t* const locator) {
  GEM_INTERNAL_CHECK(match_trace->match_alignment.effective_length >= 0,"Match effective length must be positive");
  location_t location;
  locator_map(locator,match_trace->match_alignment.match_position,&location);
  match_trace->text_position = location.position;
  match_trace->sequence_name = location.tag;
  if (location.strand == Reverse) { // Adjust position by the effective length
    match_trace->text_position -= (uint64_t) match_trace->match_alignment.effective_length;
    match_trace->strand = Reverse;
    GEM_INTERNAL_CHECK(!match_trace->emulated_rc_search,
        "Archive-Select. Locating match-trace. "
        "Impossible combination (search_strand==Reverse while emulated searching FR-index)");
  } else {
    match_trace->strand = match_trace->emulated_rc_search ? Reverse : Forward;
  }
  match_trace->bs_strand = location.bs_strand;
}
GEM_INLINE bool matches_add_match_trace(
    matches_t* const matches,match_trace_t* const match_trace,
    const bool update_counters,const locator_t* const locator,
    mm_stack_t* const mm_stack) {
  // Init
  match_trace->mapq_score = 0;
  // Check duplicates
  uint64_t* dup_match_trace_offset = matches_lookup_match(matches,
      match_trace->match_alignment.match_position,match_trace->match_alignment.effective_length);
  if (dup_match_trace_offset==NULL) {
    // Correct CIGAR (Reverse it if the search was performed in the reverse strand, emulated)
    if (match_trace->emulated_rc_search) {
      matches_cigar_reverse(matches->cigar_vector,match_trace->match_alignment.cigar_offset,
          match_trace->match_alignment.cigar_length);
      match_trace->emulated_rc_search = false;
    }
    // Locate-map the match
    match_trace_locate(match_trace,locator);
    // Add the match-trace
    const uint64_t new_match_trace_offset = vector_get_used(matches->position_matches);
    match_trace_t* new_match_trace;
    vector_alloc_new(matches->position_matches,match_trace_t,new_match_trace);
    *new_match_trace = *match_trace;
    // Index
    matches_index_match(matches,new_match_trace_offset,new_match_trace->match_alignment.match_position,
        new_match_trace->match_alignment.effective_length,mm_stack);
    // Update counters
    if (update_counters) {
      matches_counters_add(matches->counters,match_trace->distance,1);
    }
    matches_metrics_update(&matches->metrics,
        match_trace->distance,match_trace->edit_distance,match_trace->swg_score); // Update Metrics
    return true;
  } else {
    match_trace_t* const dup_match_trace = vector_get_elm(matches->position_matches,*dup_match_trace_offset,match_trace_t);
    // Pick up the least distant match
    if (dup_match_trace->distance > match_trace->distance) {
      // Correct CIGAR (Reverse it if the search was performed in the reverse strand, emulated)
      if (match_trace->emulated_rc_search) {
        matches_cigar_reverse(matches->cigar_vector,match_trace->match_alignment.cigar_offset,
            match_trace->match_alignment.cigar_length);
        match_trace->emulated_rc_search = false;
      }
      // Locate-map the match
      match_trace_locate(match_trace,locator);
      // Update counters
      if (update_counters) {
        matches_counters_sub(matches->counters,dup_match_trace->distance,1);
        matches_counters_add(matches->counters,match_trace->distance,1);
      }
      // Add the match-trace
      *dup_match_trace = *match_trace;
      // Recompute metrics
      matches_recompute_metrics(matches);
    }
    return false;
  }
}
GEM_INLINE void matches_add_interval_match(
    matches_t* const matches,const uint64_t lo,const uint64_t hi,
    const uint64_t length,const uint64_t distance,const bool emulated_rc_search) {
  // Check non-empty interval
  const uint64_t num_matches = hi-lo;
  if (gem_expect_false(num_matches==0)) return;
  // Setup the interval-match
  match_interval_t match_interval;
  match_interval.text = NULL;
  match_interval.text_length = length;
  match_interval.lo = lo;
  match_interval.hi = hi;
  match_interval.distance = distance;
  match_interval.emulated_rc_search = emulated_rc_search;
  // Update counters
  matches_counters_add(matches->counters,distance,num_matches); // FIXME Revise
  // Add the interval
  vector_insert(matches->interval_matches,match_interval,match_interval_t);
}
GEM_INLINE void matches_add_interval_set(
    matches_t* const matches,interval_set_t* const interval_set,
    const uint64_t length,const bool emulated_rc_search) {
  INTERVAL_SET_ITERATE(interval_set,interval) {
    matches_add_interval_match(matches,interval->lo,
        interval->hi,length,interval->distance,emulated_rc_search);
  }
}
GEM_INLINE void matches_hint_allocate_match_trace(matches_t* const matches,const uint64_t num_matches_trace_to_add) {
  vector_reserve_additional(matches->position_matches,num_matches_trace_to_add);
}
GEM_INLINE void matches_hint_allocate_match_interval(matches_t* const matches,const uint64_t num_matches_interval_to_add) {
  vector_reserve_additional(matches->interval_matches,num_matches_interval_to_add);
}
/*
 * Sorting Matches
 */
int match_trace_cmp_distance(const match_trace_t* const a,const match_trace_t* const b) {
  const int distance_diff = (int)a->distance - (int)b->distance;
  if (distance_diff) return distance_diff;
  const int distance_swg = (int)b->swg_score - (int)a->swg_score;
  if (distance_swg) return distance_swg;
  return (int)a->edit_distance - (int)b->edit_distance;
}
GEM_INLINE void matches_sort_by_distance(matches_t* const matches) {
  qsort(vector_get_mem(matches->position_matches,match_trace_t),
      vector_get_used(matches->position_matches),sizeof(match_trace_t),
      (int (*)(const void *,const void *))match_trace_cmp_distance);
}
int match_trace_cmp_swg_score(const match_trace_t* const a,const match_trace_t* const b) {
  return (int)b->swg_score - (int)a->swg_score;
}
GEM_INLINE void matches_sort_by_swg_score(matches_t* const matches) {
  qsort(vector_get_mem(matches->position_matches,match_trace_t),
      vector_get_used(matches->position_matches),sizeof(match_trace_t),
      (int (*)(const void *,const void *))match_trace_cmp_swg_score);
}
int match_trace_cmp_mapq_score(const match_trace_t* const a,const match_trace_t* const b) {
  return (int)b->mapq_score - (int)a->mapq_score;
}
GEM_INLINE void matches_sort_by_mapq_score(matches_t* const matches) {
  qsort(vector_get_mem(matches->position_matches,match_trace_t),
      vector_get_used(matches->position_matches),sizeof(match_trace_t),
      (int (*)(const void *,const void *))match_trace_cmp_mapq_score);
}
int match_trace_cmp_sequence_name__position(const match_trace_t* const a,const match_trace_t* const b) {
  const int cmp = gem_strcmp(a->sequence_name,b->sequence_name);
  return (cmp!=0) ? cmp : ((int)a->text_position - (int)b->text_position);
}
GEM_INLINE void matches_sort_by_sequence_name__position(matches_t* const matches) {
  // Sort global matches (match_trace_t) wrt distance
  qsort(vector_get_mem(matches->position_matches,match_trace_t),
      vector_get_used(matches->position_matches),sizeof(match_trace_t),
      (int (*)(const void *,const void *))match_trace_cmp_sequence_name__position);
}
/*
 * Filters
 */
GEM_INLINE void matches_filter_by_mapq(matches_t* const matches,const uint8_t mapq_threshold,mm_stack_t* const mm_stack) {
  const uint64_t num_matches = matches_get_num_match_traces(matches);
  match_trace_t* match_in = matches_get_match_trace_buffer(matches);
  match_trace_t* match_out = match_in;
  bool reallocated = false;
  uint64_t i;
  for (i=0;i<num_matches;++i,++match_in) {
    if (match_in->mapq_score >= mapq_threshold) {
      if (match_out != match_in) {
        reallocated = true;
        *match_out = *match_in;
      }
      ++match_out;
    }
  }
  vector_update_used(matches->position_matches,match_out);
  // Because the matches had been reallocated, the indexed-positions are no longer valid
  if (reallocated) {
    matches_index_rebuild(matches,mm_stack);
    matches_recompute_metrics(matches);
  }
}
/*
 * Display
 */
GEM_INLINE void matches_print(FILE* const stream,matches_t* const matches) {
  tab_fprintf(stream,"[GEM]>Matches\n");
  tab_global_inc();
  const matches_class_t matches_class = matches_classify(matches);
  tab_fprintf(stream,"=> Class %s\n",matches_class_label[matches_class]);
  tab_fprintf(stream,"=> Counters\t");
  matches_counters_print(stream,matches->counters,matches->max_complete_stratum);
  fprintf(stream,"\n");
  tab_fprintf(stream,"=> Interval.Matches %lu\n",vector_get_used(matches->interval_matches));
  tab_fprintf(stream,"=> Position.Matches %lu\n",vector_get_used(matches->position_matches));
  tab_fprintf(stream,"  => Positions.Hashed.Begin %lu\n",ihash_get_num_elements(matches->begin_pos_matches));
  tab_fprintf(stream,"  => Positions.Hashed.End %lu\n",ihash_get_num_elements(matches->end_pos_matches));
  tab_fprintf(stream,"=> Metrics.SE\n");
  tab_global_inc();
  matches_metrics_print(stream,&matches->metrics,false);
  tab_global_dec();
  tab_global_dec();
}


