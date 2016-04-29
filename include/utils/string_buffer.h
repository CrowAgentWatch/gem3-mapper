/*
 * PROJECT: GEMMapper
 * FILE: string_buffer.h
 * DATE: 20/08/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Simple string-buffer implementation.
 *   Static sting string_new(0), which share memory across instances (stores memory pointer)
 *   Dynamic string string_new(n>0), which handle their own memory and hold copy of the string
 */

#ifndef STRING_BUFFER_H_
#define STRING_BUFFER_H_

#include "system/commons.h"
#include "system/errors.h"
#include "system/mm_stack.h"

typedef struct {
  char* buffer;         // String buffer
  uint64_t allocated;   // Number of bytes allocated
  uint64_t length;      // Length of the string (not including EOS)
  /* MM */
  mm_stack_t* mm_stack; // MM-Stack
} string_t;

// Direction (traversal)
typedef enum { traversal_forward, traversal_backward } traversal_direction_t;

/*
 * Printers
 */
#define PRIs ".*s"
#define PRIs_content(string) (int)string_get_length((string_t*)string),string_get_buffer((string_t*)string)

/*
 * Setup
 */
void string_init(string_t* const restrict string,const uint64_t length);
void string_init_static(string_t* const restrict string,char* const restrict buffer);
void string_init_mm(string_t* const restrict string,const uint64_t length,mm_stack_t* const restrict mm_stack);
void string_resize(string_t* const restrict string,const uint64_t length);
void string_clear(string_t* const restrict string);
void string_destroy(string_t* const restrict string);

/*
 * Accessors
 */
void string_set_buffer(string_t* const restrict string,char* const restrict buffer_src,const uint64_t length);
char* string_get_buffer(string_t* const restrict string);

void string_set_length(string_t* const restrict string,const uint64_t length);
uint64_t string_get_length(string_t* const restrict string);

char* string_char_at(string_t* const restrict string,const uint64_t pos);

/*
 * Basic editing
 */
void string_append_char(string_t* const restrict string,const char character);
void string_append_eos(string_t* const restrict string);

/*
 * Append & trimming
 */
#define string_append_buffer string_right_append_buffer
#define string_append_string string_right_append_string
void string_left_append_buffer(string_t* const restrict string,const char* const restrict buffer,const uint64_t length);
void string_left_append_string(string_t* const restrict string_dst,string_t* const restrict string_src);
void string_right_append_buffer(string_t* const restrict string,const char* const restrict buffer,const uint64_t length);
void string_right_append_string(string_t* const restrict string_dst,string_t* const restrict string_src);
void string_trim_left(string_t* const restrict string,const uint64_t length);
void string_trim_right(string_t* const restrict string,const uint64_t length);

void string_copy_reverse(string_t* const restrict string_dst,string_t* const restrict string_src);

/*
 * Compare functions
 */
bool string_is_null(const string_t* const restrict string);
int64_t string_cmp(string_t* const restrict string_a,string_t* const restrict string_b);
int64_t string_ncmp(string_t* const restrict string_a,string_t* const restrict string_b,const uint64_t length);
bool string_equals(string_t* const restrict string_a,string_t* const restrict string_b);
bool string_nequals(string_t* const restrict string_a,string_t* const restrict string_b,const uint64_t length);

/*
 * Handlers
 */
string_t* string_dup(string_t* const restrict string);
void string_copy(string_t* const restrict string_dst,string_t* const restrict string_src);

/*
 * String Printers
 */
int sbprintf_v(string_t* const restrict string,const char *template,va_list v_args);
int sbprintf(string_t* const restrict string,const char *template,...);
int sbprintf_append_v(string_t* const restrict string,const char *template,va_list v_args);
int sbprintf_append(string_t* const restrict string,const char *template,...);

/*
 * Iterator
 */
#define STRING_ITERATE(string,mem,pos) \
  uint64_t pos; \
  const uint64_t __length_##mem = string_get_length(string); \
  char* mem = string_get_buffer(string); \
  for (pos=0;pos<__length_##mem;++pos) /* mem[pos] */

/*
 * Basic String Functions Wrappers
 */
void gem_strncpy(char* const restrict buffer_dst,const char* const restrict buffer_src,const uint64_t length);
char* gem_strndup(const char* const restrict buffer,const uint64_t length);
char* gem_strdup(const char* const restrict buffer);
int gem_strcmp(const char* const restrict buffer_a,const char* const restrict buffer_b);
int gem_strcasecmp(const char* const restrict buffer_a,const char* const restrict buffer_b);
bool gem_streq(const char* const restrict buffer_a,const char* const restrict buffer_b);
bool gem_strcaseeq(const char* const restrict buffer_a,const char* const restrict buffer_b);
int gem_strncmp(const char* const restrict buffer_a,const char* const restrict buffer_b,const uint64_t length);
int gem_strncasecmp(const char* const restrict buffer_a,const char* const restrict buffer_b,const uint64_t length);
bool gem_strneq(const char* const restrict buffer_a,const char* const restrict buffer_b,const uint64_t length);
char* gem_strcat(const char* const restrict buffer_a,const char* const restrict buffer_b);
uint64_t gem_strlen(const char* const restrict buffer);

char* gem_strrmext(char* const restrict buffer);
char* gem_strbasename(char* const restrict buffer);


/*
 * Error Messages
 */
#define GEM_ERROR_STRING_STATIC "Could not perform operation on static string"

#endif /* STRING_BUFFER_H_ */
