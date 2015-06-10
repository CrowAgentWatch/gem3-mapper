/*
 * PROJECT: GEMMapper
 * FILE: mapper_profile.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 *   Basically, these counters/timers focus on the measurement of:
 *     - Time spent in certain functions/blocks-code (and number of calls/executions)
 *     - Functional profile of execution flow branches (counting)
 */

#ifndef MAPPER_PROFILE_H_
#define MAPPER_PROFILE_H_

#include "essentials.h"

/*
 * Mapper
 */
#define GP_MAPPER_ALL                       0
#define GP_MAPPER_LOAD_INDEX                1
#define GP_OUTPUT_MAP_SE                    2
#define GP_OUTPUT_MAP_PE                    3
#define GP_OUTPUT_SAM_SE                    4
#define GP_OUTPUT_SAM_PE                    5

#define GP_MAPPER_CUDA_THREAD               6
#define GP_MAPPER_CUDA_THREAD_GENERATING    7
#define GP_MAPPER_CUDA_THREAD_SELECTING     8
#define GP_MAPPER_CUDA_THREAD_RESTART_UNFIT 260

#define GP_MAPPER_NUM_READS                 9

/*
 * Archive Search
 */
#define GP_ARCHIVE_SEARCH_SE                               10
#define GP_ARCHIVE_SEARCH_SE_INIT                          11
#define GP_ARCHIVE_SEARCH_PREPARE_SEQUENCE                 12
#define GP_ARCHIVE_SEARCH_SE_GENERATE_CANDIDATES           13
#define GP_ARCHIVE_SEARCH_SE_VERIFY_CANDIDATES             14
#define GP_ARCHIVE_SEARCH_SE_FINISH_SEARCH                 15
#define GP_ARCHIVE_SELECT_SE_MATCHES                       16
#define GP_ARCHIVE_SCORE_SE_MATCHES                        17

// Copy/Retrieve candidates
#define GP_ARCHIVE_SEARCH_COPY_CANDIDATES                  18
#define GP_ARCHIVE_SEARCH_RETRIEVE_CANDIDATES              19

// Search Group
#define GP_ARCHIVE_SEARCH_GROUP_BUFFERS_USED               20
#define GP_ARCHIVE_SEARCH_GROUP_RETRIEVE_CANDIDATES_DELAY  21

// Paired Mode
#define GP_ARCHIVE_SEARCH_PE                                          22
#define GP_ARCHIVE_SEARCH_PE_EXTENSION                                23
#define GP_ARCHIVE_SEARCH_PE_EXTEND_END1                              24
#define GP_ARCHIVE_SEARCH_PE_EXTEND_END1_SUCCESS                      25
#define GP_ARCHIVE_SEARCH_PE_EXTEND_END2                              26
#define GP_ARCHIVE_SEARCH_PE_EXTEND_END2_SUCCESS                      27
#define GP_ARCHIVE_SEARCH_PE_EXTEND_CANDIDATES                        28
#define GP_ARCHIVE_SEARCH_PE_RECOVER_BY_EXTENSION                     30
#define GP_ARCHIVE_SEARCH_PE_RECOVER_BY_EXTENSION_END1                31
#define GP_ARCHIVE_SEARCH_PE_RECOVER_BY_EXTENSION_END2                32
#define GP_ARCHIVE_SEARCH_PE_RECOVER_BY_EXTENSION_HIT                 33
#define GP_ARCHIVE_SEARCH_PE_PAIRED_FILTERED                          34
#define GP_ARCHIVE_SEARCH_PE_PAIRED_FILTERING_SUCCESS                 35
#define GP_ARCHIVE_SEARCH_PE_DISCARD_FILTERING_REGIONS                36
#define GP_ARCHIVE_SEARCH_PE_DISCARD_FILTERING_REGIONS_TOTAL          37
#define GP_ARCHIVE_SEARCH_PE_DISCARD_FILTERING_REGIONS_NOT_CONCORDANT 38

#define GP_ARCHIVE_SEARCH_PE_GENERATE_CANDIDATES                      39
#define GP_ARCHIVE_SEARCH_PE_FINISH_SEARCH                            40

#define GP_ARCHIVE_SELECT_PE_MATCHES                                  42
#define GP_ARCHIVE_SCORE_PE_MATCHES                                   43
#define GP_PAIRED_MATCHES_FIND_PAIRS                                  44

/*
 * I/O
 */
#define GP_INPUT_FASTA_PARSE_SEQUENCE                   45
#define GP_INPUT_FILL_BUFFER                            46
#define GP_BUFFERED_INPUT_RELOAD                        47
#define GP_BUFFERED_INPUT_RELOAD__DUMP_ATTACHED         48
#define GP_BUFFERED_INPUT_BUFFER_SIZE                   49
#define GP_BUFFERED_OUTPUT_DUMP                         50
#define GP_OUTPUT_WRITE_BUFFER                          51
#define GP_OUTPUT_BYTES_WRITTEN                         52
#define GP_OUTPUT_BUFFER_EXTENSIONS                     53
#define GP_OUTPUT_BUFFER_REQUESTS                       54
#define GP_OUTPUT_BUFFER_REQUESTS_STALLS                55
#define GP_OUTPUT_BUFFER_REQUESTS_STALLS_BUSY           56
#define GP_OUTPUT_BUFFER_REQUESTS_STALLS_NOT_PRIORITY   57

/*
 * Approximate search
 */
#define GP_AS_MAIN                 60
#define GP_AS_READ_RECOVERY        62
#define GP_AS_EXACT_SEARCH         63
#define GP_AS_NEIGHBORHOOD_SEARCH  64

#define GP_AS_ADAPTIVE_MCS         70

#define GP_AS_FILTERING_EXACT                               71
#define GP_AS_FILTERING_EXACT_BOOST                         72
#define GP_AS_FILTERING_INEXACT                             73
#define GP_AS_FILTERING_LOCAL_ALIGNMENTS                    74

#define GP_AS_FILTERING_EXACT_MAPPED                        75
#define GP_AS_FILTERING_EXACT_BOOST_MAPPED                  76
#define GP_AS_FILTERING_INEXACT_MAPPED                      77
#define GP_AS_FILTERING_LOCAL_ALIGNMENTS_MAPPED             78

#define GP_AS_FILTERING_EXACT_MCS                           79
#define GP_AS_FILTERING_EXACT_BOOST_MCS                     80
#define GP_AS_FILTERING_INEXACT_MCS                         81
#define GP_AS_FILTERING_LOCAL_ALIGNMENTS_MCS                82

#define GP_AS_GENERATE_CANDIDATES                           85
#define GP_AS_GENERATE_CANDIDATES_NUM_ELEGIBLE_REGIONS      86
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D2                 87
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D2_HIT             88
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D2_HIT_CANDIDATES  89
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D1                 90
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D1_HIT             91
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D1_HIT_CANDIDATES  92
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D0_HIT             93
#define GP_AS_GENERATE_CANDIDATES_SEARCH_D0_HIT_CANDIDATES  94
#define GP_AS_GENERATE_CANDIDATES_PROCESSED                 95
#define GP_AS_GENERATE_CANDIDATES_SKIPPED                   96
#define GP_AS_GENERATE_CANDIDATES_DYNAMIC_FILTERING         97

/*
 * Region Profile
 */
#define GP_REGION_PROFILE_ADAPTIVE                     100
#define GP_REGION_PROFILE_QUIT_PROFILE                 101

#define GP_REGION_PROFILE_MINIMAL                      105
#define GP_REGION_PROFILE_MINIMAL_NUM_REGIONS          106
#define GP_REGION_PROFILE_MINIMAL_NUM_REGIONS_UNIQUE   107
#define GP_REGION_PROFILE_MINIMAL_NUM_REGIONS_STANDARD 108
#define GP_REGION_PROFILE_MINIMAL_REGION_LENGTH        109
#define GP_REGION_PROFILE_MINIMAL_REGION_CANDIDATES    110
#define GP_REGION_PROFILE_MINIMAL_TOTAL_CANDIDATES     111

#define GP_REGION_PROFILE_DELIMIT                      112
#define GP_REGION_PROFILE_DELIMIT_NUM_REGIONS          113
#define GP_REGION_PROFILE_DELIMIT_NUM_REGIONS_UNIQUE   114
#define GP_REGION_PROFILE_DELIMIT_NUM_REGIONS_STANDARD 115
#define GP_REGION_PROFILE_DELIMIT_REGION_LENGTH        116
#define GP_REGION_PROFILE_DELIMIT_REGION_CANDIDATES    117
#define GP_REGION_PROFILE_DELIMIT_TOTAL_CANDIDATES     118

#define GP_REGION_PROFILE_BOOST                        112
#define GP_REGION_PROFILE_BOOST_NUM_REGIONS            113
#define GP_REGION_PROFILE_BOOST_NUM_REGIONS_UNIQUE     114
#define GP_REGION_PROFILE_BOOST_NUM_REGIONS_STANDARD   115
#define GP_REGION_PROFILE_BOOST_REGION_LENGTH          116
#define GP_REGION_PROFILE_BOOST_REGION_CANDIDATES      117
#define GP_REGION_PROFILE_BOOST_TOTAL_CANDIDATES       118

#define GP_REGION_PROFILE_MIN_REGION_LENGTH            119

/*
 * Filtering Candidates (Verifying)
 */
#define GP_FC_VERIFICATION                          220
#define GP_FC_PROCESS_CANDIDATES                    221
#define GP_FC_DECODE_POSITIONS                      222
#define GP_FC_VERIFY_CANDIDATE_REGIONS              223
#define GP_FC_RETRIEVE_BPM_BUFFER_CANDIDATE_REGIONS 224
#define GP_FC_REALIGN_CANDIDATE_REGIONS             225
#define GP_FC_LOCAL_ALIGNMENT                       226
#define GP_FC_REALIGN_BPM_BUFFER_CANDIDATE_REGIONS  227
#define GP_FC_RETRIEVE_CANDIDATE_REGIONS            228
#define GP_FC_COMPOSE_REGIONS                       229

#define GP_FC_EXTEND_MATCH                          250
#define GP_FC_EXTEND_RETRIEVE_CANDIDATE_REGIONS     251
#define GP_FC_EXTEND_VERIFY_CANDIDATE_REGIONS       252
#define GP_FC_EXTEND_VERIFY_CANDIDATE_LENGTH        253
#define GP_FC_EXTEND_REALIGN_CANDIDATE_REGIONS      254

#define GP_CANDIDATE_POSITIONS                      300
#define GP_CANDIDATE_POSITIONS_DUPLICATED           301
#define GP_CANDIDATE_REGIONS                        302
#define GP_CANDIDATE_REGIONS_DUPLICATED             303
#define GP_CANDIDATE_REGION_LENGTH                  304
#define GP_KMER_COUNTER_FILTER                      305
#define GP_KMER_COUNTER_FILTER_DISCARDED            306
#define GP_LEVENSHTEIN_ACCEPTED                     307

#define GP_ACCEPTED_REGIONS                         325
#define GP_ACCEPTED_EXACT                           326
#define GP_ACCEPTED_INEXACT                         327
#define GP_ACCEPTED_REGIONS_LENGTH                  328

#define GP_MATCHING_REGIONS_CHAIN                   350
#define GP_MATCHING_REGIONS_CHAIN_SUCCESS           351
#define GP_MATCHING_REGIONS_CHAIN_COVERAGE          352
#define GP_MATCHING_REGIONS_EXTEND                  353
#define GP_MATCHING_REGIONS_EXTEND_COVERAGE         354
#define GP_MATCHING_REGIONS_SCAFFOLD                355
#define GP_MATCHING_REGIONS_SCAFFOLDED              356
#define GP_MATCHING_REGIONS_SCAFFOLD_COVERAGE       357

#define GP_BPM_TILED                                370
#define GP_BMP_TILED_NUM_TILES                      371
#define GP_BMP_TILED_NUM_TILES_VERIFIED             372
#define GP_BPM_QUICK_ABANDON                        373
#define GP_BPM_ALL                                  374
#define GP_BPM_ALL_QUICK_ABANDON                    375
#define GP_BPM_ALL_MATCHES_FOUND                    376

/*
 * BPM-GPU
 */
#define GP_BPM_GPU_INIT                             401
#define GP_BPM_GPU_BUFFER_INIT                      402
#define GP_BPM_GPU_BUFFER_SEND                      403
#define GP_BPM_GPU_BUFFER_CHECK_TIME                404
#define GP_BPM_GPU_BUFFER_NUM_CANDIDATES            405
#define GP_BPM_GPU_BUFFER_CANDIDATES_LENGTH         406
#define GP_BPM_GPU_BUFFER_USAGE_CANDIDATES          407
#define GP_BPM_GPU_BUFFER_USAGE_QUERIES             408
#define GP_BPM_GPU_BUFFER_USAGE_PEQ_ENTRIES         409

/*
 * FM-Index
 */
#define GP_FMIDX_LOOKUP_DIST                        450

/*
 * Matches Align
 */
#define GP_MATCHES_ALIGN_EXACT                      480
#define GP_MATCHES_ALIGN_HAMMING                    481
#define GP_MATCHES_ALIGN_LEVENSHTEIN                482
#define GP_MATCHES_ALIGN_SWG                        483

/*
 * SWG
 */
#define GP_SWG_ALIGN_FULL                           500
#define GP_SWG_ALIGN_FULL_LENGTH                    501
#define GP_SWG_ALIGN_BANDED                         502
#define GP_SWG_ALIGN_BANDED_LENGTH                  503

/*
 * Neighborhood Search
 */
#define GP_NS_BEST_MATCH                            520

#define GP_NS_NODES_EXPLORED                        521
#define GP_NS_NODES_EXPLORED_MTABLE                 522
#define GP_NS_NODE_SUCCESS                          523
#define GP_NS_FAILED_OPT                            524
#define GP_NS_NODE_CLOSED                           525
#define GP_NS_NODE_CLOSED_DEPTH                     526

/*
 * Checks
 */
#define GP_CHECK_NUM_READS                          550
#define GP_CHECK_NUM_MAPS                           551
#define GP_CHECK_INCORRECT                          552
#define GP_CHECK_SUBOPTIMAL                         553
#define GP_CHECK_SUBOPTIMAL_SUBDOMINANT             554
#define GP_CHECK_SUBOPTIMAL_DIFF                    555
#define GP_CHECK_SUBOPTIMAL_SCORE                   556
#define GP_CHECK_SUBOPTIMAL_DISTANCE                557

/*
 * Dummy
 */
#define GP_DUMMY1                                   600
#define GP_DUMMY2                                   601

/*
 * Mapper SE
 */
GEM_INLINE void mapper_profile_print_mapper_single_end(
    FILE* const stream,const bool map_output,const uint64_t num_threads);
GEM_INLINE void mapper_profile_print_mapper_single_end_cuda(
    FILE* const stream,const bool map_output,const uint64_t num_threads);

/*
 * Mapper PE
 */
GEM_INLINE void mapper_profile_print_mapper_paired_end(
    FILE* const stream,const bool map_output,const uint64_t num_threads);
GEM_INLINE void mapper_profile_print_mapper_paired_end_cuda(
    FILE* const stream,const bool map_output,const uint64_t num_threads);

/*
 * Error Messages
 */
//#define GEM_ERROR_MAPPER_PROFILE ""

#endif /* MAPPER_PROFILE_H_ */
