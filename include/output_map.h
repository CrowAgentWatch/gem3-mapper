/*
 * PROJECT: GEMMapper
 * FILE: output_map.h
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef OUTPUT_MAP_H_
#define OUTPUT_MAP_H_

#include "essentials.h"
#include "buffered_output_file.h"
#include "sequence.h"
#include "archive.h"
#include "archive_search.h"
#include "matches.h"
#include "paired_matches.h"

/*
 * MAP Parameters
 */
typedef enum {
  map_format_v1 = 1, // GEMv1
  map_format_v2 = 2, // GEMv2
  map_format_v3 = 3, // GEMv3
} output_map_format_t;
typedef struct {
  /* MAP format  */
  output_map_format_t format_version;
} output_map_parameters_t;

/*
 * Setup
 */
void output_map_parameters_set_defaults(output_map_parameters_t* const output_map_parameters);

/*
 * Utils
 */
void output_map_cigar(
    FILE* const stream,match_trace_t* const match_trace,matches_t* const matches);
void output_map_alignment_pretty(
    FILE* const stream,match_trace_t* const match_trace,matches_t* const matches,
    uint8_t* const key,const uint64_t key_length,uint8_t* const text,
    const uint64_t text_length,mm_stack_t* const mm_stack);
/*
 * MAP fields
 */
void output_map_print_counters(
    buffered_output_file_t* const buffered_output_file,
    matches_counters_t* const matches_counter,
    const uint64_t mcs,const bool compact);
void output_map_print_match(
    buffered_output_file_t* const buffered_output_file,
    const matches_t* const matches,const match_trace_t* const match_trace,
    const bool print_mapq,const output_map_format_t output_map_format);
void output_map_print_paired_match(
    buffered_output_file_t* const buffered_output_file,
    const matches_t* const matches_end1,const matches_t* const matches_end2,
    const paired_map_t* const paired_map,const output_map_format_t output_map_format);

/*
 * MAP SingleEnd
 */
void output_map_single_end_matches(
    buffered_output_file_t* const buffered_output_file,archive_search_t* const archive_search,
    matches_t* const matches,output_map_parameters_t* const output_map_parameters);

/*
 * MAP PairedEnd
 */
void output_map_paired_end_matches(
    buffered_output_file_t* const buffered_output_file,
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches,output_map_parameters_t* const output_map_parameters);

#endif /* OUTPUT_MAP_H_ */
