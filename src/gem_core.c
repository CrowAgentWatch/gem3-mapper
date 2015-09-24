/*
 * PROJECT: GEMMapper
 * FILE: gem_core.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#include "gem_core.h"

/*
 * GEM Runtime
 */
GEM_INLINE void gem_runtime_init(const uint64_t num_threads,const uint64_t max_memory,char* const tmp_folder) {
  // GEM error handler
  gem_handle_error_signals();
  // Setup Profiling (Add master thread)
  PROF_NEW(num_threads);
  // Register Master-Thread
  gem_thread_register_id(0);
  // Configure Memory Limits // TODO Configure Memory Limits
  if (max_memory==0) {
    // f(mm_get_available_mem();
  } else {
    // f(max_memory);
  }
  // Setup temporal folder
  if (tmp_folder!=NULL) mm_set_tmp_folder(tmp_folder);
}
GEM_INLINE void gem_runtime_destroy() {
  // Clean-up Profiler
  PROF_DELETE();
  // Clean-up Thread Info
  gem_thread_cleanup();
  // Delete Memory-Pool
  mm_pool_delete();
}

