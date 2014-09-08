/*
 * PROJECT: GEMMapper
 * FILE: fm.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: File management module.
 *   Provides functionality for an easy file management providing stream-based access for reading & writing
 */

#include "fm.h"
#include "mm.h"
#include "errors.h"
#include "vector.h"

#include <unistd.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#ifdef HAVE_BZLIB
#include <bzlib.h>
#endif

/*
 * Config
 */
#ifndef O_NOATIME
  #define O_NOATIME 0 /* FIXME: O_NOATIME is not allowed if only read rights are guaranteed */
#endif

/*
 * File-Manager
 */
struct _fm_t {
  /* File */
  int fd;                 /* File descriptor */
  FILE* file;             /* FILE */
#ifdef HAVE_BZLIB
  gzFile gz_file;         /* GZip FILE */
#endif
#ifdef HAVE_BZLIB
  BZFILE* bz_file;        /* BZip FILE */
#endif
  /* Attributes */
  fm_type file_type;      /* File type */
  fm_mode mode;           /* File mode {R,W,R/W} */
  char *file_name;        /* File name */
  /* Locator */
  uint64_t byte_position; /* Current byte position */
  uint64_t file_size;     /* File size */
  bool eof;               /* End of file flag */
  /* Auxiliary Skip Buffers */
  uint8_t* skip_read_buffer;
  uint8_t* skip_write_buffer;
};

/*
 * I/O Constants/Values
 */
// Buffer Size
#define FM_BULK_COPY_BLOCK_SIZE BUFFER_SIZE_2G
#define FM_SKIP_BUFFER_SIZE     BUFFER_SIZE_2M
// Open Flags
const int fm_open_flags[3] = { O_RDONLY, O_WRONLY|O_CREAT|O_NOATIME|O_TRUNC, O_RDWR|O_CREAT|O_NOATIME };
const char* const fm_file_open_flags[3] = { "rb", "wb", "wb+" };
const char* const fm_gzfile_open_flags[3] = { "rb", "wb9", "wb+9"};

/*
 * Handy Macros
 */
#define FM_IS_READING(fm_mode) (fm_mode!=FM_WRITE)
#define FM_IS_WRITING(fm_mode) (fm_mode!=FM_READ)

/*
 * Setup
 */
GEM_INLINE void fm_initialize(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  // Locator
  file_manager->eof = FM_IS_READING(file_manager->mode) ? feof(file_manager->file) : false;
  file_manager->byte_position = 0;
  // Init Special Files
  switch (file_manager->file_type) {
    case FM_STREAM:
    case FM_REGULAR_FILE:
      break;
#ifdef HAVE_ZLIB
    case FM_GZIPPED_FILE: {
      // TODO: patch FM_READ_WRITE
      file_manager->fd = fileno(file_manager->file);
      gem_cond_fatal_error__perror(file_manager->fd==-1,FM_FILENO);
      file_manager->gz_file = gzdopen(file_manager->fd,fm_gzfile_open_flags[file_manager->mode]);
      gem_cond_fatal_error(!file_manager->gz_file,FM_GZOPEN);
      break;
    }
#endif
#ifdef HAVE_BZLIB
    case FM_BZIPPED_FILE: {
      // TODO: patch FM_READ_WRITE
      int bzerror;
      file_manager->bz_file = FM_IS_READING(file_manager->mode) ?
          BZ2_bzReadOpen(&bzerror,file_manager->file,0,0,NULL,0) :
          BZ2_bzWriteOpen(&bzerror,file_manager->file,9,0,0);
      gem_cond_fatal_error(bzerror!=BZ_OK,FM_BZOPEN);
      break;
    }
#endif
    default:
      GEM_INVALID_CASE();
      break;
  }
  // Auxiliary Skip Buffers
  file_manager->skip_read_buffer=NULL;
  file_manager->skip_write_buffer=NULL;
}
GEM_INLINE void fm_check_file_type(fm_t* const file_manager,char* const file_name) {
  GEM_CHECK_NULL(file_manager);
  // Check FILE stats
  struct stat stat_info;
  gem_cond_fatal_error__perror(stat(file_name,&stat_info)==-1,FM_STAT,file_name);
  // Open the FILE and check type
  FILE* file;
  gem_cond_fatal_error__perror(!(file=fopen(file_name,"r")),FM_OPEN,file_name);
  if (S_ISREG(stat_info.st_mode)) { // Regular file
    file_manager->file_type = FM_REGULAR_FILE;
    file_manager->file_size = stat_info.st_size;
    // Check if GZIP/BZIP compressed
    unsigned char tbuf[4];
    if (fread(tbuf,1,4,file) == 4) {
      if(tbuf[0]==0x1f && tbuf[1]==0x8b && tbuf[2]==0x08) {
        file_manager->file_type = FM_GZIPPED_FILE;
      } else if(tbuf[0]=='B' && tbuf[1]=='Z' && tbuf[2]=='h' && tbuf[3]>='0' && tbuf[3]<='9') {
        file_manager->file_type = FM_BZIPPED_FILE;
      }
    }
  } else {
    file_manager->file_type = FM_STREAM;
    file_manager->file_size = UINT64_MAX;
  }
  gem_cond_fatal_error__perror(fclose(file),FM_CLOSE,file_name);
}
GEM_INLINE fm_t* fm_open_file(char* const file_name,const fm_mode mode) {
  GEM_CHECK_NULL(file_name);
  // Allocate handler
  fm_t* file_manager = mm_alloc(fm_t);
  // File
  file_manager->fd = open(file_name,fm_open_flags[mode],S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  gem_cond_fatal_error__perror(file_manager->fd==-1,FM_OPEN,file_name);
  file_manager->file = fdopen(file_manager->fd,fm_file_open_flags[mode]);
  gem_cond_fatal_error__perror(file_manager->file==NULL,FM_FDOPEN,file_name);
  file_manager->gz_file = NULL;
  file_manager->bz_file = NULL;
  // Attributes
  file_manager->mode = mode;
  file_manager->file_name = strdup(file_name);
  gem_cond_fatal_error(file_manager->file_name==NULL,STRDUP);
  file_manager->file_size = UINT64_MAX;
  // Check BZIP/GZIP compress formats (low layer)
  if (FM_IS_READING(mode)) {
    fm_check_file_type(file_manager,file_name);
  } else {
    file_manager->file_type = FM_REGULAR_FILE;
  }
  // Initialize file manager
  fm_initialize(file_manager);
  // Return fm
  return file_manager;
}
GEM_INLINE fm_t* fm_open_temp_file() {
  // Allocate handler
  fm_t* const file_manager = mm_alloc(fm_t);
  // Open a file
  file_manager->file_name = mm_calloc(strlen(mm_get_tmp_folder())+22,char,true);
  sprintf(file_manager->file_name,"%sfm_temp_XXXXXX",mm_get_tmp_folder());
  // Make it temporary
  file_manager->fd = mkstemp(file_manager->file_name);
  gem_cond_fatal_error__perror(file_manager->fd==-1,SYS_MKSTEMP,file_manager->file_name);
  gem_cond_fatal_error__perror(unlink(file_manager->file_name),SYS_HANDLE_TMP); // Make it temporal
  // File
  file_manager->file = fdopen(file_manager->fd,fm_file_open_flags[FM_READ_WRITE]);
  gem_cond_fatal_error__perror(file_manager->file==NULL,FM_FDOPEN,file_manager->file_name);
  file_manager->gz_file = NULL;
  file_manager->bz_file = NULL;
  // Attributes
  file_manager->mode = FM_READ_WRITE;
  file_manager->file_size = UINT64_MAX;
  file_manager->file_type = FM_REGULAR_FILE;
  // Initialize file manager
  fm_initialize(file_manager);
  // Return fm
  return file_manager;
}
GEM_INLINE fm_t* fm_open_FILE(FILE* const stream,const fm_mode mode) {
  GEM_CHECK_NULL(stream);
  // Allocate handler
  fm_t* file_manager = mm_alloc(fm_t);
  // File
  file_manager->fd = 0;
  file_manager->file = stream;
  file_manager->gz_file = NULL;
  file_manager->bz_file = NULL;
  // Attributes
  file_manager->mode = mode;
  file_manager->file_name = STREAM_FILE_NAME;
  file_manager->file_type = FM_STREAM;
  file_manager->file_size = UINT64_MAX;
  // Initialize file manager
  fm_initialize(file_manager);
  // Return fm
  return file_manager;
}
GEM_INLINE fm_t* fm_open_gzFILE(FILE* const stream,const fm_mode mode) {
#ifndef HAVE_BZLIB
  gem_fatal_error(FM_NO_ZLIB_SUPPORT);
  return NULL;
#else
  GEM_CHECK_NULL(stream);
  // Allocate handler
  fm_t* file_manager = mm_alloc(fm_t);
  // File
  file_manager->fd = 0;
  file_manager->file = stream;
  file_manager->gz_file = NULL;
  file_manager->bz_file = NULL;
  // Attributes
  file_manager->mode = mode;
  file_manager->file_name = STREAM_FILE_NAME;
  file_manager->file_type = FM_GZIPPED_FILE;
  file_manager->file_size = UINT64_MAX;
  // Initialize file manager
  fm_initialize(file_manager);
  // Return fm
  return file_manager;
#endif
}
GEM_INLINE fm_t* fm_open_bzFILE(FILE* const stream,const fm_mode mode) {
#ifndef HAVE_BZLIB
  gem_fatal_error(FM_NO_BZLIB_SUPPORT);
  return NULL;
#else
  GEM_CHECK_NULL(stream);
  // Allocate handler
  fm_t* file_manager = mm_alloc(fm_t);
  // File
  file_manager->fd = 0;
  file_manager->file = stream;
  file_manager->gz_file = NULL;
  file_manager->bz_file = NULL;
  // Attributes
  file_manager->mode = mode;
  file_manager->file_name = STREAM_FILE_NAME;
  file_manager->file_type = FM_BZIPPED_FILE;
  file_manager->file_size = UINT64_MAX;
  // Initialize file manager
  fm_initialize(file_manager);
  // Return fm
  return file_manager;
#endif
}
GEM_INLINE void fm_close(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  // Close fm
  switch (file_manager->file_type) {
    case FM_STREAM:
      gem_cond_fatal_error__perror(fclose(file_manager->file),FM_CLOSE,file_manager->file_name);
      break;
    case FM_REGULAR_FILE:
      gem_cond_fatal_error__perror(fclose(file_manager->file),FM_CLOSE,file_manager->file_name);
      break;
#ifdef HAVE_ZLIB
    case FM_GZIPPED_FILE:
      gem_cond_fatal_error(gzclose(file_manager->gz_file),FM_GZCLOSE,file_manager->file_name);
      break;
#endif
#ifdef HAVE_BZLIB
    case FM_BZIPPED_FILE: {
      int bzerr;
      if (FM_IS_READING(file_manager->mode)) {
        BZ2_bzReadClose(&bzerr,file_manager->bz_file);
      } else {
        BZ2_bzWriteClose(&bzerr,file_manager->bz_file,0,NULL,NULL);
      }
      gem_cond_fatal_error(bzerr!=BZ_OK,FM_BZCLOSE,file_manager->file_name);
      break;
    }
#endif
    default:
      GEM_INVALID_CASE();
      break;
  }
  // Free
  if (strcmp(file_manager->file_name,STREAM_FILE_NAME)) mm_free(file_manager->file_name);
  if (file_manager->skip_read_buffer!=NULL)  mm_free(file_manager->skip_read_buffer);
  if (file_manager->skip_write_buffer!=NULL) mm_free(file_manager->skip_write_buffer);
  mm_free(file_manager);
}
/*
 * Accesors
 */
GEM_INLINE uint64_t fm_get_current_position(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  return file_manager->byte_position;
}
GEM_INLINE bool fm_eof(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  // Shortcut eof
  if (file_manager->eof) return true;
  // Refresh eof
  switch (file_manager->file_type) {
    case FM_STREAM:
    case FM_REGULAR_FILE:
      file_manager->eof = feof(file_manager->file);
      return file_manager->eof;
      break;
#ifdef HAVE_ZLIB
    case FM_GZIPPED_FILE:
      file_manager->eof = gzeof(file_manager->gz_file);
      return file_manager->eof;
      break;
#endif
#ifdef HAVE_BZLIB
    case FM_BZIPPED_FILE:
      return file_manager->eof;
      break;
#endif
    default:
      GEM_INVALID_CASE();
      break;
  }
  return true;
}
GEM_INLINE char* fm_get_file_name(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  return file_manager->file_name;
}
GEM_INLINE uint64_t fm_get_file_size(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  return file_manager->file_size;
}
/*
 * Seek
 */
GEM_INLINE void fm_seek(fm_t* const file_manager,const uint64_t position) {
  FM_CHECK(file_manager);
  if (file_manager->file_type==FM_REGULAR_FILE) {
    if (file_manager->byte_position!=position) {
      // True Skip (if possible)
      gem_cond_fatal_error__perror(fseek(file_manager->file,position,SEEK_SET),FM_SEEK,
          file_manager->file_name,file_manager->byte_position+position);
      // Update locator
      file_manager->byte_position = position;
      file_manager->eof = position >= file_manager->file_size;
    }
  } else {
    GEM_NOT_SUPPORTED();
  }
}
GEM_INLINE void fm_skip_forward(fm_t* const file_manager,const uint64_t num_bytes) {
  FM_CHECK(file_manager);
  GEM_CHECK_ZERO(num_bytes);
  if (FM_IS_READING(file_manager->mode)) {
    /*
     * GT_FM_READ
     */
    if (file_manager->file_type==FM_STREAM || file_manager->file_type==FM_REGULAR_FILE) {
      // True Skip (if possible)
      gem_cond_fatal_error__perror(fseek(file_manager->file,num_bytes,SEEK_CUR),FM_SEEK,
          file_manager->file_name,file_manager->byte_position+num_bytes);
      // Update locator
      file_manager->byte_position += num_bytes;
    } else {
      // Skip By Reading
      if (file_manager->skip_read_buffer==NULL) {
        file_manager->skip_read_buffer = mm_malloc_(1,FM_SKIP_BUFFER_SIZE,false,0);
      }
      uint64_t total_bytes_read = 0, bytes_read;
      while (total_bytes_read < num_bytes) {
        const uint64_t bytes_to_read = (total_bytes_read+FM_SKIP_BUFFER_SIZE < num_bytes) ?
            FM_SKIP_BUFFER_SIZE : num_bytes-total_bytes_read;
        bytes_read = fm_read_mem(file_manager,file_manager->skip_read_buffer,bytes_to_read);
        total_bytes_read += bytes_to_read;
        if (bytes_read != bytes_to_read) break;
      }
    }
  } else {
    /*
     * GT_FM_WRITE
     */
    // Skip Writing Zeros
    if (file_manager->skip_write_buffer==NULL) {
      file_manager->skip_write_buffer = mm_malloc_(1,FM_SKIP_BUFFER_SIZE,true,0);
    }
    uint64_t total_bytes_written = 0;
    while (total_bytes_written < num_bytes) {
      const uint64_t bytes_to_write = (total_bytes_written+FM_SKIP_BUFFER_SIZE < num_bytes) ?
          FM_SKIP_BUFFER_SIZE : num_bytes-total_bytes_written;
      fm_write_mem(file_manager,file_manager->skip_write_buffer,bytes_to_write);
      total_bytes_written += bytes_to_write;
    }
  }
}
GEM_INLINE void fm_skip_uint64(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  if (FM_IS_READING(file_manager->mode)) {
    fm_read_uint64(file_manager);
  } else {
    fm_write_uint64(file_manager,0);
  }
}
GEM_INLINE void fm_skip_uint32(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  if (FM_IS_READING(file_manager->mode)) {
    fm_read_uint32(file_manager);
  } else {
    fm_write_uint32(file_manager,0);
  }
}
GEM_INLINE void fm_skip_uint16(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  if (FM_IS_READING(file_manager->mode)) {
    fm_read_uint16(file_manager);
  } else {
    fm_write_uint16(file_manager,0);
  }
}
GEM_INLINE void fm_skip_uint8(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  if (FM_IS_READING(file_manager->mode)) {
    fm_read_uint8(file_manager);
  } else {
    fm_write_uint8(file_manager,0);
  }
}
GEM_INLINE void fm_skip_align(fm_t* const file_manager,const uint64_t num_bytes) {
  FM_CHECK(file_manager);
  GEM_CHECK_ZERO(num_bytes);
  const uint64_t bytes_mod = file_manager->byte_position % num_bytes;
  if (bytes_mod > 0) {
    const uint64_t bytes_to_skip = num_bytes - bytes_mod;
    fm_skip_forward(file_manager,bytes_to_skip);
  }
}
GEM_INLINE void fm_skip_align_16(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  fm_skip_align(file_manager,2);
}
GEM_INLINE void fm_skip_align_32(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  fm_skip_align(file_manager,4);
}
GEM_INLINE void fm_skip_align_64(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  fm_skip_align(file_manager,8);
}
GEM_INLINE void fm_skip_align_128(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  fm_skip_align(file_manager,16);
}
GEM_INLINE void fm_skip_align_512(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  fm_skip_align(file_manager,64);
}
GEM_INLINE void fm_skip_align_1024(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  fm_skip_align(file_manager,128);
}
GEM_INLINE void fm_skip_align_4KB(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  fm_skip_align(file_manager,512);
}
GEM_INLINE void fm_skip_align_mempage(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  int64_t sz = sysconf(_SC_PAGESIZE);
  gem_cond_fatal_error__perror(sz==-1,SYS_SYSCONF);
  fm_skip_align(file_manager,sz);
}
/*
 * Read
 */
GEM_INLINE uint64_t fm_read_uint64(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  uint64_t var;
  fm_read_mem(file_manager,&var,8);
  return var;
}
GEM_INLINE uint32_t fm_read_uint32(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  uint32_t var;
  fm_read_mem(file_manager,&var,4);
  return var;
}
GEM_INLINE uint16_t fm_read_uint16(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  uint16_t var;
  fm_read_mem(file_manager,&var,2);
  return var;
}
GEM_INLINE uint8_t fm_read_uint8(fm_t* const file_manager) {
  FM_CHECK(file_manager);
  uint8_t var;
  fm_read_mem(file_manager,&var,1);
  return var;
}
GEM_INLINE uint64_t fm_read_mem(fm_t* const file_manager,void* const dst,const uint64_t num_bytes) {
  FM_CHECK(file_manager);
  GEM_CHECK_NULL(dst);
  GEM_CHECK_ZERO(num_bytes);
  gem_fatal_check(!FM_IS_READING(file_manager->mode),FM_INVALID_MODE_READ,file_manager->file_name);
  // Read
  int64_t num_bytes_read = 0;
  switch (file_manager->file_type) {
    case FM_STREAM:
    case FM_REGULAR_FILE:
      num_bytes_read = fread(dst,1,num_bytes,file_manager->file);
      // gem_cond_fatal_error(num_bytes_read==0,FM_READ_ZERO,num_bytes,file_manager->file_name);
      file_manager->eof = (num_bytes_read<num_bytes);
      break;
#ifdef HAVE_ZLIB
    case FM_GZIPPED_FILE:
      num_bytes_read = gzread(file_manager->gz_file,dst,num_bytes);
      gem_cond_fatal_error(num_bytes_read==-1,FM_GZREAD,num_bytes,file_manager->file_name);
      file_manager->eof = (num_bytes_read<num_bytes);
      break;
#endif
#ifdef HAVE_BZLIB
    case FM_BZIPPED_FILE:
      if (!file_manager->eof) {
        int bzerror;
        num_bytes_read = BZ2_bzRead(&bzerror,file_manager->bz_file,dst,num_bytes);
        if (gem_expect_false(bzerror==BZ_STREAM_END)) {
          file_manager->eof = true;
        } else {
          gem_cond_fatal_error(bzerror!=BZ_OK,FM_BZREAD,num_bytes,file_manager->file_name);
          file_manager->eof = num_bytes_read<num_bytes;
        }
      }
      break;
#endif
    default:
      GEM_INVALID_CASE();
      break;
  }
  // Update locator
  file_manager->byte_position += num_bytes_read;
  return num_bytes_read;
}
GEM_INLINE uint64_t fm_read_mem_parallel(
    fm_t* const file_manager,void* const dst,const uint64_t num_bytes,const uint64_t num_threads) {
  GEM_NOT_IMPLEMENTED(); // TODO
  return 0;
}
GEM_INLINE mm_t* fm_load_mem(fm_t* const file_manager,const uint64_t num_bytes) {
  FM_CHECK(file_manager);
  GEM_CHECK_ZERO(num_bytes);
  // Allocate Memory
  mm_t* const mm = mm_bulk_mmalloc(num_bytes,false);
  void* const mm_mem = mm_get_base_mem(mm);
  // Read
  const uint64_t bytes_read = fm_read_mem(file_manager,mm_mem,num_bytes);
  gem_cond_fatal_error(bytes_read!=num_bytes,FM_LOAD,num_bytes,bytes_read);
  // Return
  return mm;
}
/*
 * Write
 */
GEM_INLINE void fm_write_uint64(fm_t* const file_manager,const uint64_t data) {
  FM_CHECK(file_manager);
  fm_write_mem(file_manager,&data,8);
}
GEM_INLINE void fm_write_uint32(fm_t* const file_manager,const uint32_t data) {
  FM_CHECK(file_manager);
  fm_write_mem(file_manager,&data,4);
}
GEM_INLINE void fm_write_uint16(fm_t* const file_manager,const uint16_t data) {
  FM_CHECK(file_manager);
  fm_write_mem(file_manager,&data,2);
}
GEM_INLINE void fm_write_uint8(fm_t* const file_manager,const uint8_t data) {
  FM_CHECK(file_manager);
  fm_write_mem(file_manager,&data,1);
}
GEM_INLINE void fm_write_mem(fm_t* const file_manager,const void* const src,const uint64_t num_bytes) {
  FM_CHECK(file_manager);
  gem_fatal_check(!FM_IS_WRITING(file_manager->mode),FM_INVALID_MODE_WRITE,file_manager->file_name);
  switch (file_manager->file_type) {
    case FM_STREAM:
    case FM_REGULAR_FILE: {
      gem_cond_fatal_error__perror(fwrite(src,1,num_bytes,file_manager->file)!=num_bytes,FM_WRITE,file_manager->file_name);
      break;
    }
#ifdef HAVE_ZLIB
    case FM_GZIPPED_FILE: {
      gem_cond_fatal_error(gzwrite(file_manager->gz_file,src,num_bytes)!=num_bytes,FM_GZWRITE,file_manager->file_name);
      break;
    }
#endif
#ifdef HAVE_BZLIB
    case FM_BZIPPED_FILE: {
      int bzerror;
      BZ2_bzWrite(&bzerror,file_manager->bz_file,(void*)src,num_bytes);
      gem_cond_fatal_error(bzerror!=BZ_OK,FM_BZWRITE,file_manager->file_name);
      break;
    }
#endif
    default:
      GEM_INVALID_CASE();
      break;
  }
  // Update locator
  file_manager->byte_position += num_bytes;
}
/*
 * Bulk read of file
 */
GEM_INLINE void fm_bulk_read_fd(const int fd,void* const dst,const uint64_t size) {
  GEM_CHECK_NULL(dst);
  uint64_t bytes_written = 0;
  while (bytes_written < size) {
    const uint64_t bytes_pending = size-bytes_written;
    const uint64_t chunk_size = (bytes_pending<FM_BULK_COPY_BLOCK_SIZE) ? bytes_pending : FM_BULK_COPY_BLOCK_SIZE;
    // Copy chunk
    gem_cond_fatal_error__perror(read(fd,dst+bytes_written,chunk_size)!=chunk_size,FM_FDREAD,chunk_size,fd);
    bytes_written += chunk_size;
  }
}
GEM_INLINE void fm_bulk_read_file(char* const file_name,void* const dst,const uint64_t offset,const uint64_t size) {
  GEM_CHECK_NULL(file_name);
  GEM_CHECK_NULL(dst);
  // Retrieve input file info
  struct stat stat_info;
  gem_cond_fatal_error__perror(stat(file_name,&stat_info)==-1,FM_STAT,file_name);
  // Open file descriptor
  const int fd = open(file_name,O_RDONLY,S_IRUSR);
  gem_cond_fatal_error__perror(fd==-1,FM_FDOPEN,file_name);
  if (offset > 0) {
    gem_cond_fatal_error__perror(lseek(fd,offset,SEEK_SET)==-1,FM_SEEK,file_name,offset); // Seek
  }
  // Read file
  fm_bulk_read_fd(fd,dst,(size==0) ? stat_info.st_size-offset : size);
}
GEM_INLINE void fm_bulk_read_file_parallel(
    char* const file_name,void* const dst,const uint64_t offset,const uint64_t size,const uint64_t num_threads) {
  GEM_NOT_IMPLEMENTED(); // TODO
}
/*
 * FileManager Wrappers
 */
GEM_INLINE void gem_stat(char* const file_name,struct stat *stat_info) {
  GEM_CHECK_NULL(file_name);
  gem_cond_fatal_error__perror(stat(file_name,stat_info)==-1,FM_STAT,file_name);
}
GEM_INLINE int gem_open_fd(char* const file_name,const int flags,const mode_t mode) {
  int fd = open(file_name,flags,mode);
  gem_cond_fatal_error__perror(fd==-1,FM_OPEN,file_name);
  return fd;
}
GEM_INLINE FILE* gem_open_FILE(char* const file_name,const char* opentype) {
  FILE* const file = fopen(file_name,opentype);
  gem_cond_fatal_error__perror(file==NULL,FM_FDOPEN,file_name);
  return file;
}
GEM_INLINE void gem_unlink(char* const file_name) {
  gem_cond_fatal_error__perror(unlink(file_name),FM_UNLINK,file_name);
}
/*
 * FileManager Printers
 */
GEM_INLINE int vfmprintf(fm_t* const file_manager,const char *template,va_list v_args) {
  FM_CHECK(file_manager);
  GEM_CHECK_NULL(template);
  GEM_CHECK_NULL(v_args);
  // Depending on the type we might have to compose the data into a buffer
  int num_bytes = 0;
  switch (file_manager->file_type) {
    case FM_STREAM:
    case FM_REGULAR_FILE:
      num_bytes = vfprintf(file_manager->file,template,v_args);
      break;
    case FM_GZIPPED_FILE:
    case FM_BZIPPED_FILE:
      if (file_manager->skip_write_buffer==NULL) {
        file_manager->skip_write_buffer = mm_malloc_(1,FM_SKIP_BUFFER_SIZE,true,0);
      }
      num_bytes = vsprintf((char*)file_manager->skip_write_buffer,template,v_args);
      fm_write_mem(file_manager,file_manager->skip_write_buffer,num_bytes);
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
  return num_bytes;
}
GEM_INLINE int fmprintf(fm_t* const file_manager,const char *template,...) {
  FM_CHECK(file_manager);
  GEM_CHECK_NULL(template);
  va_list v_args;
  va_start(v_args,template);
  const int num_bytes = vfmprintf(file_manager,template,v_args);
  va_end(v_args);
  return num_bytes;
}
