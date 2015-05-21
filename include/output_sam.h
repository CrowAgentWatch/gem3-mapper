/*
 * PROJECT: GEMMapper
 * FILE: output_sam.h
 * DATE: 06/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef OUTPUT_SAM_H_
#define OUTPUT_SAM_H_

#include "essentials.h"
#include "buffered_output_file.h"
#include "archive.h"
#include "archive_search.h"
#include "sequence.h"
#include "matches.h"
#include "paired_matches.h"

/*
 * SAM Parameters
 */
typedef struct {
  /* Header & RG  */
  char *read_group_header;
  string_t *read_group_id;
  /* Read & Qualities */
  bool omit_secondary_read__qualities;
  /* CIGAR */
  bool print_mismatches;
  /* XA */
  bool compact_xa;
  /* GEM compatibility */
  bool print_gem_fields;
} output_sam_parameters_t;

GEM_INLINE void output_sam_parameters_set_defaults(output_sam_parameters_t* const sam_parameters);
GEM_INLINE void output_sam_parse_read_group_header(char* const read_group_buffer,output_sam_parameters_t* const sam_parameters);

/*
 * SAM Headers
 */
GEM_INLINE void output_sam_print_header(
    output_file_t* const output_file,archive_t* const archive,
    output_sam_parameters_t* const sam_parameters,int argc,char** argv);
			

/*
 * SAM output SE
 */
GEM_INLINE void output_sam_single_end_matches(
    buffered_output_file_t* const buffered_output_file,
    archive_search_t* const archive_search,matches_t* const matches,
    const output_sam_parameters_t* const output_sam_parameters);

/*
 * SAM output PE
 */
GEM_INLINE void output_sam_paired_end_matches(
    buffered_output_file_t* const buffered_output_file,
    archive_search_t* const archive_search_end1,archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches,const output_sam_parameters_t* const output_sam_parameters);

#endif /* OUTPUT_SAM_H_ */
