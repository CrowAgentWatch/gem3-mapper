/*
 * PROJECT: GEMMapper
 * FILE: sa_builder.h
 * DATE: 07/06/2013
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 *   Implements a data storage structure as to classify SA positions with respect
 *   to it's stating k-mer. SA positions are stored in blocks of a given size
 */

#ifndef SA_BUILDER_H_
#define SA_BUILDER_H_

#include "essentials.h"
#include "dna_text.h"
#include "sampled_sa.h"

/*
 * Checkers
 */
#define SA_BUILDER_CHECK(sa_builder) GEM_CHECK_NULL(sa_builder)

/*
 * Constants
 */
#define DS_SHALLOW_LIMIT            100
#define DS_MK_QUICKSORT_THRESHOLD    20

#define SA_BUILDER_NUM_WRITTERS     128

#define SA_BUILDER_ALPHABET_SIZE    8
#define SA_BUILDER_TOTAL_MONOMERS   (SA_BUILDER_ALPHABET_SIZE)
#define SA_BUILDER_TOTAL_DIMERS     (8*SA_BUILDER_TOTAL_MONOMERS)
#define SA_BUILDER_TOTAL_TRIMERS    (8*SA_BUILDER_TOTAL_DIMERS)
#define SA_BUILDER_TOTAL_TETRAMERS  (8*SA_BUILDER_TOTAL_TRIMERS)

#define SA_BUILDER_KMER_LENGTH                      8
#define SA_BUILDER_KMER_MASK_INDEX(kmer_idx)        ((kmer_idx) & 0x0000000000FFFFFFull)

#define SA_BUILDER_ENC_TRIMER(one,two,three)        (((one)<<6) | ((two)<<3) | (three))
#define SA_BUILDER_TRIMER_MASK_INDEX(kmer_idx)      (((kmer_idx)>>15) & 0x00000000000001FFull)

#define SA_BUILDER_ENC_TETRAMER(one,two,three,four) (((one)<<9) | ((two)<<6) | (three<<3) | (four))
#define SA_BUILDER_TETRAMER_MASK_INDEX(kmer_idx)    (((kmer_idx)>>12) & 0x0000000000000FFFull)

/*
 * Compact Text
 *
 *             2-BWT       Cached Text
 *  ++--+     +--+--+  +--+--+--+--+--+--+
 *  ||  | ... |  |  |  |  |  |  |  |  |  |
 *  ++--+     +--+--+  +--+--+--+--+--+--+
 *    21       07 06    05 04 03 02 01 00  <= Packets of 3 bits
 *
 */
#define SA_BWT_CACHED_LENGTH          2
#define SA_SUFFIX_CACHED_LENGTH       6

#define SA_BWT_CYCLIC_LENGTH (SA_BUILDER_KMER_LENGTH-1)
#define SA_BWT_PADDED_LENGTH (MAX(SA_BWT_CYCLIC_LENGTH,DS_SHALLOW_LIMIT+21))

/*
 * SA position (Stored)
 *
 *     2BWT       Cached Text      SA position => (40 bits => 1TB indexable)
 *   +--+--+  +--+--+--+--+--+--+  +--+   +--+
 *   |  |  |  |  |  |  |  |  |  |  ||||...||||
 *   +--+--+  +--+--+--+--+--+--+  +--+   +--+
 *   63...58  57      ...      40   39 ... 00  <= Bits positions
 *
 */
#define SA_COMPACTED_TEXT_MASK_PIGGYBACKING(kmer_idx) (((kmer_idx) & 0x000000003FFFFFC0ull)<<34)

#define SA_POS_MASK_POSITION(sa_pos)        ((sa_pos) & 0x000000FFFFFFFFFFull)
#define SA_POS_MASK_SUFFIX_CACHE(sa_pos)    ((sa_pos) & 0x03FFFF0000000000ull)
#define SA_POS_MASK_FIRST_CHARACTER(sa_pos) (((sa_pos)>>55) & 0x0000000000000007ull)
#define SA_POS_MASK_GET_BWT1(sa_pos)        (((sa_pos)>>58) & 0x0000000000000007ull)
#define SA_POS_MASK_GET_BWT2(sa_pos)        (((sa_pos)>>61) & 0x0000000000000007ull)

#define SA_POS_GENERATE(sa_pos,c0,c1)   ( (((((uint64_t)c0)<<3) | ((uint64_t)c1))<<58) | ((uint64_t)sa_pos) )

#define SA_SBUCKET_TYPE_A           0x1000000000000000ull
#define SA_SBUCKET_TYPE_B           0x2000000000000000ull
#define SA_SBUCKET_TYPE_C           0x4000000000000000ull
#define SA_SBUCKET_TYPE_MASK        0xF000000000000000ull
#define SA_SBUCKET_STATE_PENDING    0x0100000000000000ull
#define SA_SBUCKET_STATE_ASSIGNED   0x0200000000000000ull
#define SA_SBUCKET_STATE_COMPLETED  0x0300000000000000ull
#define SA_SBUCKET_STATE_PROCESSED  0x0400000000000000ull
#define SA_SBUCKET_STATE_MASK       0x0F00000000000000ull
#define SA_SBUCKET_THREAD_MASK      0x00FFFFFFFFFFFFFFull

#define SA_SBUCKET_TEST_TYPE(sa_bucket_flags,TYPE)   ((sa_bucket_flags  & SA_SBUCKET_TYPE_MASK)  == SA_SBUCKET_TYPE_##TYPE)
#define SA_SBUCKET_TEST_STATE(sa_bucket_flags,STATE) ((sa_bucket_flags  & SA_SBUCKET_STATE_MASK) == SA_SBUCKET_STATE_##STATE)
#define SA_SBUCKET_GET_THREAD(sa_bucket_flags)       ((sa_bucket_flags) & SA_SBUCKET_THREAD_MASK)

#define SA_SBUCKET_SET_TYPE(sa_bucket_flags,TYPE) \
  sa_bucket_flags = (sa_bucket_flags & (~SA_SBUCKET_TYPE_MASK))  | SA_SBUCKET_TYPE_##TYPE
#define SA_SBUCKET_SET_STATE(sa_bucket_flags,STATE) \
  sa_bucket_flags = (sa_bucket_flags & (~SA_SBUCKET_STATE_MASK)) | SA_SBUCKET_STATE_##STATE

/*
 * SA-Builder
 */
typedef struct {
  uint64_t thread_responsible;
  uint64_t sa_offset;               // Offset to the first SA-pos for this group
  uint64_t num_sa_positions;        // Total SA-pos in this group
  fm_t* sa_positions_file;          // Points to the next free SA-pos for this group
} sa_group_t;
typedef struct {
  /* Text */
  char* name_prefix;                 // Name Prefix
  dna_text_t* enc_text;              // Encoded Text
  /* K-mer counting/splitting */
  uint64_t* kmer_count;              // K-mer Count Table (Boundaries of the SA)
  uint64_t num_kmers;                // Total number of possible k-mers
  /* SA-Positions Generation */
  char* sa_positions_file_name;      // SA-positions File Name
  fm_t* sa_positions_file;           // SA-positions File
  fm_t** sa_file_reader;             // SA-positions File Readers
  sa_group_t* sa_groups;             // Groups of SA-positions (to subsidiary sort)
  uint64_t num_sa_groups;            // Number of groups
  /* SA-Positions Subsidiary Sorting */
  uint64_t block_size;               // Memory Block Size
  uint64_t max_bucket_size;
  /* System */
  uint64_t num_threads;              // Number of threads to parallelize the computation
  uint64_t max_thread_memory;
  pthread_t* pthreads;
  /* Stats */
  stats_vector_t* kmer_count_stats;  // k-mer count stats
  uint64_t kmer_count_max;
  stats_vector_t* group_size_stats;  // Group size stats
  stats_vector_t* suffix_cmp_length;
  /* Misc */
  ticker_t ticker;
} sa_builder_t;

/*
 * Setup
 */
GEM_INLINE sa_builder_t* sa_builder_new(
    char* const name_prefix,dna_text_t* const enc_text,
    const uint64_t num_threads,const uint64_t max_memory);
GEM_INLINE void sa_builder_delete(sa_builder_t* const sa_builder);

/*
 * Sorting Suffixes
 *   1.- Count all suffixes
 *   2.- Store all suffixes
 *   3.- Sort all suffixes
 */
GEM_INLINE void sa_builder_count_suffixes(sa_builder_t* const sa_builder,uint64_t* const character_occurrences,const bool verbose);
GEM_INLINE void sa_builder_store_suffixes(sa_builder_t* const sa_builder,const bool verbose);
GEM_INLINE void sa_builder_sort_suffixes(
    sa_builder_t* const sa_builder,dna_text_t* const enc_bwt,sampled_sa_builder_t* const sampled_sa,const bool verbose);

/*
 * Stats
 */
GEM_INLINE void sa_builder_display_stats(FILE* const stream,sa_builder_t* const sa_builder,const bool display_groups);

/*
 * Debug
 */
GEM_INLINE void sa_builder_debug_print_sa(
    FILE* stream,sa_builder_t* const sa_builder,
    const uint64_t sa_position,const uint64_t sa_suffix_length);

/*
 * Errors
 */
#define GEM_ERROR_SA_BUILDER_LIMIT_MAX_BLOCK_MEM "SA Builder. Maximum SA-bucket (%lu GB) larger than max-memory-block allowed (%lu GB)"
#define GEM_ERROR_SA_BUILDER_LIMIT_MAX_BLOCK_ADDRESSED "SA Builder. Maximum SA-block (%lu GB) cannot be addressed (%lu GB)"
#define GEM_ERROR_SA_BUILDER_STORE_BLOCK_NOT_FILLED "SA Builder. K-mer block not filled according to previous counting"
#define GEM_ERROR_SA_BUILDER_SEQUENCE_MIN_LENGTH "SA Builder. Index total length (%lu) is below minimum threshold (%lu)"

#endif /* SA_BUILDER_H_ */
