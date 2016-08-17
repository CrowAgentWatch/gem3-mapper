/*
 * PROJECT: GEMMapper
 * FILE: gpu_buffer.h
 * DATE: 06/06/2012
 * AUTHOR(S): Alejandro Chacon <alejandro.chacon@uab.es>
 *            Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#ifndef GPU_BUFFER_COLLECTION_H_
#define GPU_BUFFER_COLLECTION_H_

#include "utils/essentials.h"
#include "system/profiler_timer.h"
#include "gpu/gpu_config.h"
#include "archive/archive.h"

typedef struct {
  /* Buffers */
  void* gpu_buffers_dto;              // GPU-Buffer Initializer DTO
  void** internal_buffers;            // Internal Buffers
  uint64_t num_buffers;               // Total number of buffers allocated
  /* Active Modules */
  bool gpu_region_profile_available;
  bool gpu_decode_candidates_sa_available;
  bool gpu_decode_candidates_text_available;
  bool gpu_verify_candidates_available;
} gpu_buffer_collection_t;

/*
 * Setup
 */
gpu_buffer_collection_t* gpu_buffer_collection_new(
    char* const gpu_index_name,
    const uint64_t num_buffers,
    const uint64_t buffer_size,
    const bool verbose);
void gpu_buffer_collection_delete(gpu_buffer_collection_t* const gpu_buffer_collection);

/*
 * Accessors
 */
void* gpu_buffer_collection_get_buffer(
    const gpu_buffer_collection_t* const gpu_buffer_collection,
    const uint64_t buffer_no);

#endif /* GPU_BUFFER_COLLECTION_H_ */
