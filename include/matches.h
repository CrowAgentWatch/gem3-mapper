/*
 * PROJECT: GEMMapper
 * FILE: matches.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Data structure to store alignment matches {sequence,position,strand,...}
 */

#ifndef MATCHES_H_
#define MATCHES_H_

#include "essentials.h"
#include "locator.h"
#include "interval_set.h"
#include "text_collection.h"
#include "match_alignment.h"
#include "matches_counters.h"
#include "matches_metrics.h"
#include "align_swg.h"

/*
 * Interval Match
 */
typedef struct {
  /* Meta-info */
  bool emulated_rc_search; // Match resulting from a RC-emulated search (using the forward-strand)
  /* Index */
  uint64_t lo;             // SA Lo-Position
  uint64_t hi;             // SA Hi-Position
  /* Sequence */
  uint8_t* text;           // Pointer to the matching-text
  uint64_t text_length;    // Length of the matching-text
  strand_t strand;         // Mapping Strand
  /* Score */
  uint64_t distance;       // Distance of the alignment
  int32_t swg_score;       // SWG Distance (score) of the alignment
} match_interval_t;

/*
 * Position Match (Trace-Match)
 */
typedef struct {
  /* Match Text (Reference) */
  uint64_t trace_offset;   // Trace-offset in the text-collection
  uint8_t* text;           // Pointer to the matching-text
  uint64_t text_length;    // Length of the matching-text
  /* Match */
  char* sequence_name;     // Sequence name (After decoding.Eg Chr1)
  strand_t strand;         // Mapping Strand
  bs_strand_t bs_strand;   // Bisulfite Strand
  uint64_t text_position;  // Position of the match in the text. Local text (Eg wrt Chr1)
  bool emulated_rc_search; // Match resulting from a RC-emulated search (using the forward-strand)
  /* Score */
  uint64_t distance;       // Distance
  uint64_t edit_distance;  // Edit-Distance
  int32_t swg_score;       // SWG Distance/Score
  uint8_t mapq_score;      // MAPQ Score
  /* Alignment */
  match_alignment_t match_alignment; // Match Alignment (CIGAR + ...)
#ifdef GEM_DEBUG
  void* match_scaffold;    // Supporting Scaffolding
#endif
} match_trace_t;

/*
 * Matches
 */
typedef struct {
  /* Search-matches state */
  uint64_t max_complete_stratum;
  /* Text Collection Buffer */
  text_collection_t* text_collection;  // Stores text-traces (candidates/matches/regions/...)
  /* Matches Counters */
  matches_counters_t* counters;        // Global counters
  /* Interval Matches */
  vector_t* interval_matches;          // Interval Matches (match_interval_t)
  /* Position Matches */
  vector_t* position_matches;          // Position Matches (match_trace_t)
  ihash_t* begin_pos_matches;          // Begin position (of the aligned match) in the text-space
  ihash_t* end_pos_matches;            // End position (of the aligned match) in the text-space
  /* CIGAR */
  vector_t* cigar_vector;              // CIGAR operations storage (cigar_element_t)
  /* Metrics */
  matches_metrics_t metrics;           // Metrics
} matches_t;

/*
 * Setup
 */
matches_t* matches_new();
void matches_configure(matches_t* const matches,text_collection_t* const text_collection);
void matches_clear(matches_t* const matches);
void matches_delete(matches_t* const matches);

/*
 * Accessors
 */
bool matches_is_mapped(const matches_t* const matches);
void matches_recompute_metrics(matches_t* const matches);
uint64_t matches_get_first_stratum_matches(matches_t* const matches);
uint64_t matches_get_subdominant_stratum_matches(matches_t* const matches);

/*
 * Index
 */
void matches_index_rebuild(matches_t* const matches,mm_stack_t* const mm_stack);
void matches_index_clear(matches_t* const matches);

/*
 * Matches
 */
match_trace_t* matches_get_match_trace_buffer(const matches_t* const matches);
match_trace_t* matches_get_match_trace(const matches_t* const matches,const uint64_t offset);
uint64_t matches_get_num_match_traces(const matches_t* const matches);
void matches_get_clear_match_traces(const matches_t* const matches);

cigar_element_t* match_trace_get_cigar_buffer(const matches_t* const matches,const match_trace_t* const match_trace);
uint64_t match_trace_get_cigar_length(const match_trace_t* const match_trace);
uint64_t match_trace_get_event_distance(const match_trace_t* const match_trace);
int64_t match_trace_get_effective_length(
    matches_t* const matches,const uint64_t read_length,
    const uint64_t cigar_buffer_offset,const uint64_t cigar_length);

/*
 * Adding Matches
 */
bool matches_add_match_trace(
    matches_t* const matches,match_trace_t* const match_trace,
    const bool update_counters,const locator_t* const locator,
    mm_stack_t* const mm_stack);
void matches_add_interval_match(
    matches_t* const matches,const uint64_t lo,const uint64_t hi,
    const uint64_t length,const uint64_t distance,const bool emulated_rc_search);
void matches_add_interval_set(
    matches_t* const matches,interval_set_t* const interval_set,
    const uint64_t length,const bool emulated_rc_search);

void matches_hint_allocate_match_trace(matches_t* const matches,const uint64_t num_matches_trace_to_add);
void matches_hint_allocate_match_interval(matches_t* const matches,const uint64_t num_matches_interval_to_add);

/*
 * Sorting Matches
 */
void matches_sort_by_distance(matches_t* const matches);
void matches_sort_by_swg_score(matches_t* const matches);
void matches_sort_by_mapq_score(matches_t* const matches);
void matches_sort_by_sequence_name__position(matches_t* const matches);

/*
 * Filters
 */
void matches_filter_by_mapq(matches_t* const matches,const uint8_t mapq_threshold,mm_stack_t* const mm_stack);

/*
 * Display
 */
void matches_print(FILE* const stream,matches_t* const matches);

#endif /* MATCHES_H_ */
