/*
 * PROJECT: GEMMapper
 * FILE: gem-mapper.c
 * DATE:5/12/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION: Genomic Read Mapper
 */
#define REPORT_STATS

#include "gem_core.h"

/*
 * GEM-mapper Error Handling
 */
#define gem_mapper_error_msg(error_msg,args...) \
  fprintf(stderr,"GEM-Mapper error:\n> "error_msg"\n",##args); \
  exit(1)
#define gem_mapper_cond_error_msg(condition,error_msg,args...) \
  do { \
    if (__builtin_expect((condition),0)){ \
      gem_mapper_error_msg(error_msg,##args); \
    } \
  } while (0)
/*
 * GEM-mapper Parsing
 */
int parse_arguments_system_integer(char* const argument,uint64_t* const value) {
  char* argument_ptr = argument;
  // Textual
  if (gem_strcaseeq(argument_ptr,"c")) {
    *value = system_get_num_processors();
    return 0;
  }
  // Number
  double number;
  const int error = input_text_parse_double((const char** const)&argument_ptr,&number);
  if (error) return error;
  // Textual
  if (gem_strcaseeq(argument_ptr,"c")) {
    *value = number*(double)system_get_num_processors();
  } else {
    *value = number;
  }
  return 0;
}
/*
 * GEM-mapper I/O related functions
 */
input_file_t* gem_mapper_open_input_file(
    char* const input_file_name,const fm_type input_compression,
    const uint64_t input_block_size,const bool verbose_user) {
  // Open input file
  input_file_t* input_file;
  if (input_file_name==NULL) {
    gem_cond_log(verbose_user,"[Reading input file from stdin]");
    switch (input_compression) {
      case FM_GZIPPED_FILE:
        input_file = input_gzip_stream_open(stdin,input_block_size);
        break;
      case FM_BZIPPED_FILE:
        input_file = input_bzip_stream_open(stdin,input_block_size);
        break;
      default:
        input_file = input_stream_open(stdin,input_block_size);
        break;
    }
  } else {
    gem_cond_log(verbose_user,"[Opening input file '%s']",input_file_name);
    input_file = input_file_open(input_file_name,input_block_size,false);
  }
  return input_file;
}
void gem_mapper_open_input(mapper_parameters_t* const parameters) {
  if (parameters->io.separated_input_files) {
    parameters->input_file_end1 = gem_mapper_open_input_file(
        parameters->io.input_file_name_end1,parameters->io.input_compression,
        parameters->io.input_block_size,parameters->misc.verbose_user);
    parameters->input_file_end2 = gem_mapper_open_input_file(
        parameters->io.input_file_name_end2,parameters->io.input_compression,
        parameters->io.input_block_size,parameters->misc.verbose_user);
  } else {
    parameters->input_file = gem_mapper_open_input_file(
        parameters->io.input_file_name,parameters->io.input_compression,
        parameters->io.input_block_size,parameters->misc.verbose_user);
  }
}
void gem_mapper_close_input(mapper_parameters_t* const parameters) {
  if (parameters->io.separated_input_files) {
    input_file_close(parameters->input_file_end1);
    input_file_close(parameters->input_file_end2);
  } else {
    input_file_close(parameters->input_file);
  }
}
void gem_mapper_open_output(mapper_parameters_t* const parameters) {
  // Open output stream
  if (parameters->io.output_file_name==NULL) {
    gem_cond_log(parameters->misc.verbose_user,"[Outputting to stdout]");
    parameters->output_stream = stdout;
  } else {
    gem_cond_log(parameters->misc.verbose_user,"[Outputting to '%s']",parameters->io.output_file_name);
    parameters->output_stream = fopen(parameters->io.output_file_name,"w");
  }
  // Open output file
  const mapper_parameters_cuda_t* const cuda = &parameters->cuda;
  const uint64_t max_output_buffers = (cuda->cuda_enabled) ? cuda->output_num_buffers : parameters->io.output_num_buffers;
  const uint64_t output_buffer_size = (cuda->cuda_enabled) ? cuda->output_buffer_size : parameters->io.output_buffer_size;
  switch (parameters->io.output_compression) {
    case FM_GZIPPED_FILE:
      parameters->output_file = output_gzip_stream_new(parameters->output_stream,max_output_buffers,output_buffer_size);
      break;
    case FM_BZIPPED_FILE:
      parameters->output_file = output_bzip_stream_new(parameters->output_stream,max_output_buffers,output_buffer_size);
      break;
    default:
      parameters->output_file = output_stream_new(parameters->output_stream,max_output_buffers,output_buffer_size);
      break;
  }
}
void gem_mapper_close_output(mapper_parameters_t* const parameters) {
  output_file_close(parameters->output_file);
}
void gem_mapper_print_profile(mapper_parameters_t* const parameters) {
  // Reduce Stats
  switch (parameters->misc.profile_reduce_type) {
    case reduce_sum: PROF_REDUCE_SUM(); break;
    case reduce_max: PROF_REDUCE_MAX(); break;
    case reduce_min: PROF_REDUCE_MIN(); break;
    case reduce_mean: PROF_REDUCE_MEAN(); break;
    case reduce_sample: PROF_REDUCE_SAMPLE(); break;
    default: GEM_INVALID_CASE(); break;
  }
  // System
  system_print_info(gem_info_get_stream());
  // Parameters
  // mapper_parameters_print(gem_info_get_stream(),parameters);
  // Mapper
  if (!parameters->cuda.cuda_enabled) {
    // CPU Mapper
    switch (parameters->mapper_type) {
      case mapper_se:
        mapper_profile_print_mapper_single_end(gem_info_get_stream(),
            parameters->io.output_format==MAP,parameters->system.num_threads);
        break;
      case mapper_pe:
        mapper_profile_print_mapper_paired_end(gem_info_get_stream(),
            parameters->io.output_format==MAP,parameters->system.num_threads);
        break;
      case mapper_graph:
        break;
      default:
        break;
    }
  } else {
    // CUDA Mapper
    switch (parameters->mapper_type) {
      case mapper_se:
        mapper_profile_print_mapper_single_end_cuda(gem_info_get_stream(),
            parameters->io.output_format==MAP,parameters->system.num_threads);
        break;
      case mapper_pe:
        mapper_profile_print_mapper_paired_end_cuda(gem_info_get_stream(),
            parameters->io.output_format==MAP,parameters->system.num_threads);
        break;
      case mapper_graph:
        break;
      default:
        break;
    }
  }
}
/*
 * GEM-Mapper options Menu
 */
option_t gem_mapper_options[] = {
  /* I/O */
  { 'I', "index", REQUIRED, TYPE_STRING, 2, VISIBILITY_USER, "<index_file.gem>", "" },
  { 'i', "input", REQUIRED, TYPE_STRING, 2, VISIBILITY_USER, "<file>", "(FASTA/FASTQ, default=stdin)" },
  { '1', "i1", REQUIRED, TYPE_STRING, 2, VISIBILITY_USER, "<file>", "(paired-end, end-1)" },
  { '2', "i2", REQUIRED, TYPE_STRING, 2, VISIBILITY_USER, "<file>", "(paired-end, end-2)" },
  { 'z', "gzip-input", NO_ARGUMENT, TYPE_NONE, 2, VISIBILITY_USER, "", "(gzip input)" },
  { 'j', "bzip-input", NO_ARGUMENT, TYPE_NONE, 2, VISIBILITY_USER, "", "(bzip input)" },
  { 201, "input-model", REQUIRED, TYPE_STRING, 2, VISIBILITY_DEVELOPER, "<input_block_size,num_buffers,num_records>", "(default=64M,2c,5K)" },
  { 'o', "output", REQUIRED, TYPE_STRING, 2, VISIBILITY_USER, "<output_prefix>" , "(default=stdout)" },
  { 202, "gzip-output", NO_ARGUMENT, TYPE_NONE, 2, VISIBILITY_USER, "", "(gzip output)" },
  { 203, "bzip-output", NO_ARGUMENT, TYPE_NONE, 2, VISIBILITY_USER, "", "(bzip output)" },
  { 204, "output-model", REQUIRED, TYPE_STRING, 2, VISIBILITY_DEVELOPER, "<buffer_size,num_buffers>", "(default=4M,5c)" },
  { 205, "report-file", REQUIRED, TYPE_STRING, 2, VISIBILITY_USER, "<file_name>", "(default=NULL)" },
  /* Qualities */
  { 'q', "quality-format", REQUIRED, TYPE_STRING, 3, VISIBILITY_ADVANCED, "'ignore'|'offset-33'|'offset-64'", "(default=offset-33)" },
  { 'Q', "quality-model", REQUIRED, TYPE_STRING, 3, VISIBILITY_ADVANCED, "'gem'|'flat'", "(default=gem)" },
  { 300, "gem-quality-threshold", REQUIRED, TYPE_INT, 3, VISIBILITY_ADVANCED, "<number>", "(default=26, that is e<=2e-3)" },
  { 301, "mismatch-alphabet", REQUIRED, TYPE_STRING, 4, VISIBILITY_ADVANCED, "<symbols>" , "(default='ACGT')" },
  /* Single-end Alignment */
  { 400, "mapping-mode", REQUIRED, TYPE_STRING, 4, VISIBILITY_USER, "'fast'|'thorough'|'complete'" , "(default=thorough)" },
  { 401, "search-max-matches", REQUIRED, TYPE_INT, 4, VISIBILITY_ADVANCED, "<number>" , "(default=1K)" },
  { 'E', "complete-search-error", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_ADVANCED, "<number|percentage>" , "(default=0.04, 4%)" },
  { 's', "complete-strata-after-best", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_ADVANCED, "<number|percentage>" , "(default=1)" },
  { 'e', "alignment-max-error", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_USER, "<number|percentage>" , "(default=0.08, 8%)" },
  { 402, "alignment-override-max-error", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_USER, "'never'|'if-unmapped'" , "(default=if-unmapped)" },
  { 403, "alignment-max-bandwidth", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_ADVANCED, "<number|percentage>" , "(default=0.20, 20%)" },
  { 404, "alignment-min-identity", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_USER, "<number|percentage>" , "(default=40%)" },
  { 405, "alignment-scaffolding", OPTIONAL, TYPE_STRING, 4, VISIBILITY_ADVANCED, "" , "(default=true)" },
  { 406, "alignment-scaffolding-min-coverage", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_DEVELOPER, "<number|percentage>" , "(default=80%)" },
  { 407, "alignment-scaffolding-min-homopolymer-context", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_DEVELOPER, "<number|percentage>" , "(default=2)" },
  { 408, "alignment-scaffolding-min-matching_length", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_DEVELOPER, "<number|percentage>" , "(default=10)" },
  { 409, "alignment-curation", OPTIONAL, TYPE_STRING, 4, VISIBILITY_ADVANCED, "" , "(default=true)" },
  { 410, "alignment-curation-min-end-context", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_DEVELOPER, "<number|percentage>" , "(default=2)" },
  { 411, "region-model-minimal", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_DEVELOPER, "<region_th>,<max_steps>,<dec_factor>,<region_type_th>" , "" },
  { 412, "region-model-boost", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_DEVELOPER, "<region_th>,<max_steps>,<dec_factor>,<region_type_th>" , "" },
  { 413, "region-model-delimit", REQUIRED, TYPE_FLOAT, 4, VISIBILITY_DEVELOPER, "<region_th>,<max_steps>,<dec_factor>,<region_type_th>" , "" },
  /* Paired-end Alignment */
  { 'p', "paired-end-alignment", NO_ARGUMENT, TYPE_NONE, 5, VISIBILITY_USER, "" , "" },
  { 'l', "min-template-length", REQUIRED, TYPE_INT, 5, VISIBILITY_USER, "<number>" , "(default=disabled)" },
  { 'L', "max-template-length", REQUIRED, TYPE_INT, 5, VISIBILITY_USER, "<number>" , "(default=10000)" },
  { 500, "discordant-pair-search", REQUIRED, TYPE_STRING, 5, VISIBILITY_USER, "'always'|'if-no-concordant'|'never'" , "(default=if-no-concordant)" },
  { 501, "pair-orientation", REQUIRED, TYPE_STRING, 5, VISIBILITY_ADVANCED, "'FR'|'RF'|'FF'|'RR'" , "(default=FR)" },
  { 502, "discordant-pair-orientation", REQUIRED, TYPE_STRING, 5, VISIBILITY_ADVANCED, "'FR'|'RF'|'FF'|'RR'" , "(default=RF)" },
  { 503, "pair-layout", REQUIRED, TYPE_STRING, 5, VISIBILITY_ADVANCED, "'separate'|'overlap'|'contain'" , "(default=separated,overlap)" },
  { 504, "discordant-pair-layout", REQUIRED, TYPE_STRING, 5, VISIBILITY_ADVANCED, "'separate'|'overlap'|'contain'" , "(default=contain)" },
  /* Bisulfite Alignment */
  { 601, "bisulfite-read", REQUIRED, TYPE_STRING, 6, VISIBILITY_ADVANCED, "'inferred','1','2','interleaved'",  "(default=inferred)" },
  // { 602, "bisulfite-suffix", REQUIRED, TYPE_STRING, 6, VISIBILITY_ADVANCED, "<C2T suffix, G2A suffix>" , "(default=#C2T,#G2A)" },
  /* Alignment Score */
  { 700, "alignment-model", REQUIRED, TYPE_STRING, 7, VISIBILITY_ADVANCED, "'none'|'hamming'|'edit'|'gap-affine'" , "(default=gap-affine)" },
  { 701, "gap-affine-penalties", REQUIRED, TYPE_STRING, 7, VISIBILITY_USER, "A,B,O,X" , "(default=1,4,6,1)" },
  { 'A', "matching-score", REQUIRED, TYPE_INT, 7, VISIBILITY_ADVANCED, "" , "(default=1)" },
  { 'B', "mismatch-penalty", REQUIRED, TYPE_INT, 7, VISIBILITY_ADVANCED, "" , "(default=4)" },
  { 'O', "gap-open-penalty", REQUIRED, TYPE_INT, 7, VISIBILITY_ADVANCED, "" , "(default=6)" },
  { 'X', "gap-extension-penalty", REQUIRED, TYPE_INT, 7, VISIBILITY_ADVANCED, "" , "(default=1)" },
  { 702, "gap-affine-threshold", OPTIONAL, TYPE_INT, 7, VISIBILITY_ADVANCED, "" , "(default=0.20)" },
  /* MAQ Score */
  { 800, "mapq-model", REQUIRED, TYPE_STRING, 8, VISIBILITY_ADVANCED, "'none'|'gem'" , "(default=gem)" },
  // TODO { 801, "mapq-threshold", REQUIRED, TYPE_INT, 8, VISIBILITY_DEVELOPER, "<number>" , "(default=0)" },
  /* Reporting */
  { 'D', "min-reported-strata", REQUIRED, TYPE_FLOAT, 9, VISIBILITY_USER, "<number|percentage>|'all'" , "(stratum-wise, default=0)" },
  { 'm', "min-reported-matches", REQUIRED, TYPE_INT,  9, VISIBILITY_USER, "<number>|'all'" , "(default=10)" },
  { 'M', "max-reported-matches", REQUIRED, TYPE_INT,  9, VISIBILITY_USER, "<number>|'all'" , "(default=100)" },
  /* Output Format */
  { 'F',  "output-format", REQUIRED, TYPE_STRING, 10, VISIBILITY_USER, "'MAP'|'SAM'" , "(default=SAM)" },
  { 1000, "sam-compact", OPTIONAL, TYPE_STRING, 10, VISIBILITY_USER, "'true'|'false'" , "(default=true)" },
  { 'r',  "sam-read-group-header", REQUIRED, TYPE_STRING, 10, VISIBILITY_USER, "<read_group_header> (i.e. '@RG\\tID:xx\\tSM:yy')" , "(default=NULL)" },
  { 1001, "sam-gem-compatible", OPTIONAL, TYPE_STRING, 10, VISIBILITY_DEVELOPER, "'true'|'false'" , "(default=false)" },
  { 1002, "map-format", REQUIRED, TYPE_INT, 10, VISIBILITY_DEVELOPER, "'1'|'2'|'3'" , "(default=2)" },
  /* System */
  { 't',  "threads", REQUIRED, TYPE_STRING, 11, VISIBILITY_USER, "<number>" , "(default=#cores)" },
  { 1100, "max-memory", REQUIRED, TYPE_STRING, 11, VISIBILITY_DEVELOPER, "<maximum-memory>" , "(Eg 2GB)" },
  { 1101, "tmp-folder", REQUIRED, TYPE_STRING, 11, VISIBILITY_DEVELOPER, "<temporal_dir_path>" , "(default=/tmp/)" },
  /* CUDA Settings */
#ifdef HAVE_CUDA
  { 1200, "cuda", OPTIONAL, TYPE_STRING, 12, VISIBILITY_USER, "", "(default=disabled)"},
  { 1201, "cuda-buffers-per-thread", REQUIRED, TYPE_STRING, 12, VISIBILITY_DEVELOPER, "<num_buffers,buffer_size>" , "(default=2,3,3,4M)" },
#endif /* HAVE_CUDA */
  /* Presets/Hints */
  { 1300, "reads-model", REQUIRED, TYPE_STRING, 13, VISIBILITY_DEVELOPER, "<average_length>[,<std_length>]" , "(default=150,50)" },
  /* Debug */
  { 'c', "check-alignments", REQUIRED, TYPE_STRING, 14, VISIBILITY_DEVELOPER, "'correct'|'best'|'complete'" , "" },
  /* Miscellaneous */
  { 1500, "profile", OPTIONAL, TYPE_STRING, 15, VISIBILITY_DEVELOPER, "'sum'|'min'|'max'|'mean'|'sample'" , "(disabled)" },
  { 'v',  "verbose", OPTIONAL, TYPE_STRING, 15, VISIBILITY_USER, "'quiet'|'user'|'dev'" , "(default=user)" },
  { 'h',  "help", OPTIONAL, TYPE_NONE, 15, VISIBILITY_USER, "" , "(print usage)" },
  {  0, "", 0, 0, 0, false, "", ""}
};
char* gem_mapper_groups[] = {
  /*  0 */ "Null",
  /*  1 */ "Unclassified",
  /*  2 */ "I/O",
  /*  3 */ "Qualities",
  /*  4 */ "Single-end Alignment",
  /*  5 */ "Paired-end Alignment",
  /*  6 */ "Bisulfite Alignment",
  /*  7 */ "Alignment Score",
  /*  8 */ "MAPQ Score",
  /*  9 */ "Reporting",
  /* 10 */ "Output-format",
  /* 11 */ "System",
  /* 12 */ "CUDA Settings",
  /* 13 */ "Presets/Hints",
  /* 14 */ "Debug",
  /* 15 */ "Miscellaneous",
};
void gem_mapper_print_usage(const option_visibility_t visibility_level) {
  fprintf(stderr, "USAGE: ./gem-mapper [ARGS]...\n");
  options_fprint_menu(stderr,gem_mapper_options,gem_mapper_groups,true,visibility_level);
}
void parse_arguments(int argc,char** argv,mapper_parameters_t* const parameters) {
  // Check number of parameters (quick usage & exit)
  if (argc <= 1) {
    gem_mapper_print_usage(VISIBILITY_USER);
    exit(0);
  }
  // Set CMD line
  parameters->argc = argc;
  parameters->argv = argv;
  // Parameters
  mapper_parameters_io_t* const io = &parameters->io;
  search_parameters_t* const search = &parameters->search_parameters;
  paired_search_parameters_t* const paired_search = &search->paired_search_parameters;
  mapper_parameters_cuda_t* const cuda = &parameters->cuda;
  // Parse parameters
  char *bs_suffix1=0, *bs_suffix2=0;
  struct option* getopt_options = options_adaptor_getopt(gem_mapper_options);
  string_t* const getopt_short_string = options_adaptor_getopt_short(gem_mapper_options);
  char* const getopt_short = string_get_buffer(getopt_short_string);
  int option, option_index;
  while (true) {
    // Get option &  Select case
    if ((option=getopt_long(argc,argv,getopt_short,getopt_options,&option_index))==-1) break;
    switch (option) {
    /* I/O */
    case 'I': // --index
      io->index_file_name = optarg;
      break;
    case 'i': // --input
      io->separated_input_files = false;
      io->input_file_name = optarg; // TODO Multiple input files
      break;
    case '1': // --i1
      paired_search->paired_end_search = true;
      io->separated_input_files = true;
      io->input_file_name_end1 = optarg;
      break;
    case '2': // --i2
      paired_search->paired_end_search = true;
      io->separated_input_files = true;
      io->input_file_name_end2 = optarg;
      break;
    case 'z': // --gzip-input
      io->input_compression = FM_GZIPPED_FILE;
      break;
    case 'j': // --bzip-input
      io->input_compression = FM_BZIPPED_FILE;
      break;
    case 201: { // --input-model=64M,2c,5K
      char *input_block_size=NULL, *num_buffers=NULL, *num_records=NULL;
      const int num_arguments = input_text_parse_csv_arguments(optarg,3,&input_block_size,&num_buffers,&num_records);
      gem_mapper_cond_error_msg(num_arguments!=3,"Option '--input-model' wrong number of arguments");
      // Parse input-buffer size
      int error = input_text_parse_size(input_block_size,&io->input_block_size);
      gem_mapper_cond_error_msg(error,"Option '--input-model'. Error parsing 'input_block_size'");
      // Parse number of buffers
      error = parse_arguments_system_integer(num_buffers,&io->input_num_buffers);
      gem_mapper_cond_error_msg(error,"Option '--input-model'. Error parsing 'num_buffers'");
      // Parse number of records per buffer
      error = input_text_parse_size(num_records,&io->input_buffer_lines);
      gem_mapper_cond_error_msg(error,"Option '--input-model'. Error parsing 'num_records'");
      // Adjust
      io->input_buffer_lines *= (2*4); // 2l-Paired x 4l-FASTQRecord
      // Propagate settings to CUDA
      cuda->input_block_size = io->input_block_size;
      cuda->input_num_buffers = io->input_num_buffers;
      cuda->input_buffer_lines = io->input_buffer_lines;
      break;
    }
    case 'o': // --output
      io->output_file_name = optarg;
      break;
    case 202: // --gzip-output
      io->output_compression = FM_GZIPPED_FILE;
      break;
    case 203: // --bzip-output
      io->output_compression = FM_BZIPPED_FILE;
      break;
    case 204: { // --output-model=4M,4c
      char *buffer_size=NULL, *num_buffers=NULL;
      const int num_arguments = input_text_parse_csv_arguments(optarg,2,&buffer_size,&num_buffers);
      gem_mapper_cond_error_msg(num_arguments!=2,"Option '--output-model' wrong number of arguments");
      // Parse output-buffer size
      int error = input_text_parse_size(buffer_size,&io->output_buffer_size);
      gem_mapper_cond_error_msg(error,"Option '--output-model'. Error parsing 'buffer_size'");
      // Parse number of output-buffers
      error = parse_arguments_system_integer(num_buffers,&io->output_num_buffers);
      gem_mapper_cond_error_msg(error,"Option '--output-model'. Error parsing 'num_buffers'");
      // Propagate settings to CUDA
      cuda->output_buffer_size = io->output_buffer_size;
      cuda->output_num_buffers = io->output_num_buffers;
      break;
    }
		 case 205: // --report-file
			 io->report_file_name = optarg;
			 break;
    /* Qualities */
    case 'q': // --quality-format
      if (gem_strcaseeq(optarg,"ignore")) {
        search->quality_format=qualities_ignore;
      } else if (gem_strcaseeq(optarg,"offset-33")) {
        parameters->search_parameters.quality_format=qualities_offset_33;
      } else if (gem_strcaseeq(optarg,"offset-64")) {
        search->quality_format=qualities_offset_64;
      } else {
        gem_mapper_error_msg("Option '-q|--quality-format' must be 'ignore', 'offset-33' or 'offset-64'");
      }
      break;
    case 'Q': // --quality-model
      if (gem_strcaseeq(optarg,"gem")) {
        search->quality_model=quality_model_type_gem;
        // TODO parameters->quality_score=gem_quality_score;
      } else if (gem_strcaseeq(optarg,"flat")) {
        search->quality_model=quality_model_type_flat;
        // TODO parameters->quality_score=flat_quality_score;
      } else {
        gem_mapper_error_msg("Option '-Q|--quality-model' must be 'gem' or 'flat'");
      }
      break;
    case 300: // --gem-quality-threshold (default=26, that is e<=2e-3)
      input_text_parse_extended_uint64(optarg,&search->quality_threshold);
      break;
    case 301: { // --mismatch-alphabet (default='ACGT')
      const char* const mismatch_alphabet = optarg;
      search_configure_replacements(&parameters->search_parameters,mismatch_alphabet,gem_strlen(mismatch_alphabet));
      break;
    }
    /* Single-end Alignment */
    case 400: // --mapping-mode in {'fast'|'thorough'|'complete'} (default=thorough)
      if (gem_strcaseeq(optarg,"fast")) {
        search->mapping_mode = mapping_adaptive_filtering_fast;
      } else if (gem_strcaseeq(optarg,"thorough")) {
        search->mapping_mode = mapping_adaptive_filtering_thorough;
      } else if (gem_strcaseeq(optarg,"complete")) {
        search->mapping_mode = mapping_adaptive_filtering_complete;
      } else if (gem_strcaseeq(optarg,"brute-force")) {
        search->mapping_mode = mapping_neighborhood_search;
      } else if (gem_strcaseeq(optarg,"filtering-complete")) {
        search->mapping_mode = mapping_fixed_filtering_complete;
      } else if (gem_strcaseeq(optarg,"test")) {
        search->mapping_mode = mapping_test;
      } else {
        gem_mapper_error_msg("Option '--mapping-mode' must be 'fast'|'thorough'|'complete'");
      }
      break;
    case 401: // --search-max-matches (default=1K)
      input_text_parse_extended_uint64(optarg,&search->search_max_matches);
      paired_search->max_paired_matches = search->search_max_matches;
      break;
    case 'E': // --complete-search-error (default=0.04, 4%)
      search->complete_search_error = atof(optarg);
      break;
    case 's': // --complete-strata-after-best (default=1)
      input_text_parse_extended_double(optarg,(double*)&search->complete_strata_after_best);
      break;
    case 'e': // --alignment-max-error (default=0.08, 8%)
      search->alignment_max_error = atof(optarg);
      break;
    case 402: // --alignment-override-max-error in {'never'|'if-unmapped'} (default=if-no-match)
      if (gem_strcaseeq(optarg,"FR")) {
        search->unbounded_alignment = unbounded_alignment_never;
      } else if (gem_strcaseeq(optarg,"RF")) {
        search->unbounded_alignment = unbounded_alignment_if_unmapped;
      } else {
        gem_mapper_error_msg("Option '--alignment-override-max-error' must be 'never'|'if-unmapped'");
      }
      break;
    case 403: // --alignment-max-bandwidth (default=0.20, 20%)
      input_text_parse_extended_double(optarg,(double*)&search->max_bandwidth);
      break;
    case 404: // --alignment-min-identity (default=40)
      input_text_parse_extended_double(optarg,(double*)&search->alignment_min_identity);
      break;
    case 405: // --alignment-scaffolding (default=true)
      search->alignment_scaffolding = input_text_parse_extended_bool(optarg);
      break;
    case 406: // --alignment-scaffolding-min-coverage (default=80%)
      input_text_parse_extended_double(optarg,(double*)&search->alignment_scaffolding_min_coverage);
      break;
    case 407: // --alignment-scaffolding-min-homopolymer-context (default=2)
      input_text_parse_extended_double(optarg,(double*)&search->alignment_scaffolding_homopolymer_min_context);
      break;
    case 408: // --alignment-scaffolding-min-matching_length (default=10)
      input_text_parse_extended_double(optarg,(double*)&search->alignment_scaffolding_min_matching_length);
      break;
    case 409: // --alignment-curation (default=true)
      search->cigar_curation = input_text_parse_extended_bool(optarg);
      break;
    case 410: // --alignment-curation-min-end-context (default=2)
      input_text_parse_extended_double(optarg,(double*)&search->cigar_curation_min_end_context);
      break;
    case 411: { // --region-model-minimal <region_th>,<max_steps>,<dec_factor>,<region_type_th>
      char *region_th=NULL, *max_steps=NULL, *dec_factor=NULL, *region_type_th=NULL;
      const int num_arguments = input_text_parse_csv_arguments(optarg,4,&region_th,&max_steps,&dec_factor,&region_type_th);
      gem_mapper_cond_error_msg(num_arguments!=4,"Option '--region-model' wrong number of arguments");
      input_text_parse_extended_uint64(region_th,&search->rp_minimal.region_th); // Parse region_th
      input_text_parse_extended_uint64(max_steps,&search->rp_minimal.max_steps); // Parse max_steps
      input_text_parse_extended_uint64(dec_factor,&search->rp_minimal.dec_factor); // Parse dec_factor
      input_text_parse_extended_uint64(region_type_th,&search->rp_minimal.region_type_th); // Parse region_type_th
      break;
    }
    case 412: { // --region-model-boost   <region_th>,<max_steps>,<dec_factor>,<region_type_th>
      char *region_th=NULL, *max_steps=NULL, *dec_factor=NULL, *region_type_th=NULL;
      const int num_arguments = input_text_parse_csv_arguments(optarg,4,&region_th,&max_steps,&dec_factor,&region_type_th);
      gem_mapper_cond_error_msg(num_arguments!=4,"Option '--region-model' wrong number of arguments");
      input_text_parse_extended_uint64(region_th,&search->rp_boost.region_th); // Parse region_th
      input_text_parse_extended_uint64(max_steps,&search->rp_boost.max_steps); // Parse max_steps
      input_text_parse_extended_uint64(dec_factor,&search->rp_boost.dec_factor); // Parse dec_factor
      input_text_parse_extended_uint64(region_type_th,&search->rp_boost.region_type_th); // Parse region_type_th
      break;
    }
    case 413: { // --region-model-delimit <region_th>,<max_steps>,<dec_factor>,<region_type_th>
      char *region_th=NULL, *max_steps=NULL, *dec_factor=NULL, *region_type_th=NULL;
      const int num_arguments = input_text_parse_csv_arguments(optarg,4,&region_th,&max_steps,&dec_factor,&region_type_th);
      gem_mapper_cond_error_msg(num_arguments!=4,"Option '--region-model' wrong number of arguments");
      input_text_parse_extended_uint64(region_th,&search->rp_delimit.region_th); // Parse region_th
      input_text_parse_extended_uint64(max_steps,&search->rp_delimit.max_steps); // Parse max_steps
      input_text_parse_extended_uint64(dec_factor,&search->rp_delimit.dec_factor); // Parse dec_factor
      input_text_parse_extended_uint64(region_type_th,&search->rp_delimit.region_type_th); // Parse region_type_th
      break;
    }
    /* Paired-end Alignment */
    case 'p': // --paired-end-alignment
      paired_search->paired_end_search = true;
      break;
    case 'l': // --min-template-length (default=disabled)
      input_text_parse_extended_uint64(optarg,&paired_search->min_template_length);
      break;
    case 'L': // --max-template-length (default=10000)
      input_text_parse_extended_uint64(optarg,&paired_search->max_template_length);
      break;
    case 500: { // --discordant-pair-search in 'always'|'if-no-concordant'|'never' (default=if-no-concordant)
      if (gem_strcaseeq(optarg,"always")) {
        paired_search->pair_discordant_search = pair_discordant_search_always;
      } else if (gem_strcaseeq(optarg,"if-no-concordant")) {
        paired_search->pair_discordant_search = pair_discordant_search_only_if_no_concordant;
      } else if (gem_strcaseeq(optarg,"never")) {
        paired_search->pair_discordant_search = pair_discordant_search_never;
      } else {
        gem_mapper_error_msg("Option '--search-discordant' must be 'always'|'if-no-concordant'|'never'");
      }
      break;
    }
    case 501: { // --pair-orientation in {'FR'|'RF'|'FF'|'RR'} (default=FR)
      // Init null
      paired_search->pair_orientation[pair_orientation_FR] = pair_relation_invalid;
      paired_search->pair_orientation[pair_orientation_RF] = pair_relation_invalid;
      paired_search->pair_orientation[pair_orientation_FF] = pair_relation_invalid;
      paired_search->pair_orientation[pair_orientation_RR] = pair_relation_invalid;
      // Start parsing
      char *pair_orientation = strtok(optarg,",");
      while (pair_orientation!=NULL) {
        if (gem_strcaseeq(pair_orientation,"FR")) {
          paired_search->pair_orientation[pair_orientation_FR] = pair_relation_concordant; continue;
        }
        if (gem_strcaseeq(pair_orientation,"RF")) {
          paired_search->pair_orientation[pair_orientation_RF] = pair_relation_concordant; continue;
        }
        if (gem_strcaseeq(pair_orientation,"FF")) {
          paired_search->pair_orientation[pair_orientation_FF] = pair_relation_concordant; continue;
        }
        if (gem_strcaseeq(pair_orientation,"RR")) {
          paired_search->pair_orientation[pair_orientation_RR] = pair_relation_concordant; continue;
        }
        gem_mapper_error_msg("Option '--pair-orientation' must be 'FR'|'RF'|'FF'|'RR'");
        pair_orientation = strtok(NULL,",");
      }
      break;
    }
    case 502: { // --discordant-pair-orientation in {'FR'|'RF'|'FF'|'RR'} (default=RF)
      // Start parsing
      char *discordant_pair_orientation = strtok(optarg,",");
      while (discordant_pair_orientation!=NULL) {
        if (gem_strcaseeq(discordant_pair_orientation,"FR")) {
          paired_search->pair_orientation[pair_orientation_FR] = pair_relation_discordant; continue;
        }
        if (gem_strcaseeq(discordant_pair_orientation,"RF")) {
          paired_search->pair_orientation[pair_orientation_RF] = pair_relation_discordant; continue;
        }
        if (gem_strcaseeq(discordant_pair_orientation,"FF")) {
          paired_search->pair_orientation[pair_orientation_FF] = pair_relation_discordant; continue;
        }
        if (gem_strcaseeq(discordant_pair_orientation,"RR")) {
          paired_search->pair_orientation[pair_orientation_RR] = pair_relation_discordant; continue;
        }
        gem_mapper_error_msg("Option '--discordant-pair-orientation' must be 'FR'|'RF'|'FF'|'RR'");
        discordant_pair_orientation = strtok(NULL,",");
      }
      break;
    }
    case 503: { // --pair-layout in {'separate'|'overlap'|'contain'} (default=separated,overlap,contain)
      paired_search->pair_layout[pair_layout_separate] = pair_relation_invalid;
      paired_search->pair_layout[pair_layout_overlap] = pair_relation_invalid;
      paired_search->pair_layout[pair_layout_contain] = pair_relation_invalid;
      char *pair_layout = strtok(optarg,","); // Start parsing
      while (pair_layout!=NULL) {
        if (gem_strcaseeq(pair_layout,"separate")) {
          paired_search->pair_layout[pair_layout_separate] = pair_relation_concordant; continue;
        }
        if (gem_strcaseeq(pair_layout,"overlap"))  {
          paired_search->pair_layout[pair_layout_overlap] = pair_relation_concordant; continue;
        }
        if (gem_strcaseeq(pair_layout,"contain"))  {
          paired_search->pair_layout[pair_layout_contain] = pair_relation_concordant; continue;
        }
        gem_mapper_error_msg("Option '--pair-layout' must be 'separate'|'overlap'|'contain'");
        pair_layout = strtok(NULL,",");
      }
      break;
    }
    case 504: { // --discordant-pair-layout in {'separate'|'overlap'|'contain'} (default=none)
      char *pair_layout = strtok(optarg,","); // Start parsing
      while (pair_layout!=NULL) {
        if (gem_strcaseeq(pair_layout,"separate")) {
          paired_search->pair_layout[pair_layout_separate] = pair_relation_discordant; continue;
        }
        if (gem_strcaseeq(pair_layout,"overlap"))  {
          paired_search->pair_layout[pair_layout_overlap] = pair_relation_discordant; continue;
        }
        if (gem_strcaseeq(pair_layout,"contain"))  {
          paired_search->pair_layout[pair_layout_contain] = pair_relation_discordant; continue;
        }
        gem_mapper_error_msg("Option '--pair-layout' must be 'separate'|'overlap'|'contain'");
        pair_layout = strtok(NULL,",");
      }
      break;
    }
    /* Bisulfite Alignment */
    case 601: // --bisulfite_read
      if (gem_strcaseeq(optarg,"inferred")) {
        parameters->search_parameters.bisulfite_read = bisulfite_read_inferred;
        break;
      }
      if (gem_strcaseeq(optarg,"1")) {
        parameters->search_parameters.bisulfite_read = bisulfite_read_1;
        break;
      }
      if (gem_strcaseeq(optarg,"2")) {
        parameters->search_parameters.bisulfite_read = bisulfite_read_2;
        break;
      }
      if (gem_strcaseeq(optarg,"interleaved")) {
        parameters->search_parameters.bisulfite_read = bisulfite_read_interleaved;
        break;
      }
      gem_fatal_error_msg("Option '--bisulfite_read' must be '1'|'2'|'interleaved'|'inferred'");
      break;
    case 602: { // --bisulfite_suffix
      const int num_arguments = input_text_parse_csv_arguments(optarg,2,bs_suffix1,bs_suffix2);
      gem_cond_fatal_error_msg(num_arguments!=2,"Option '--bisulfite_suffix' wrong number of arguments");
      break;
    }
    /* Alignment Score */
    case 700: // --alignment-model
      if (gem_strcaseeq(optarg,"none")) {
        search->alignment_model = alignment_model_none;
      } else if (gem_strcaseeq(optarg,"hamming")) {
        search->alignment_model = alignment_model_hamming;
      } else if (gem_strcaseeq(optarg,"edit") || gem_strcaseeq(optarg,"levenshtein") ) {
        search->alignment_model = alignment_model_levenshtein;
      } else if (gem_strcaseeq(optarg,"gap-affine")) {
        search->alignment_model = alignment_model_gap_affine;
      } else {
        gem_mapper_error_msg("Option '--alignment-model' must be 'none'|'hamming'|'edit'|'gap-affine'");
      }
      break;
    case 701: { // --gap-affine-penalties (A,B,O,X) (default=1,4,6,1)
      char *matching=NULL, *mismatch=NULL, *gap_open=NULL, *gap_extension=NULL;
      const int num_arguments = input_text_parse_csv_arguments(optarg,4,&matching,&mismatch,&gap_open,&gap_extension);
      gem_mapper_cond_error_msg(num_arguments!=4,"Option '--gap-affine-penalties' wrong number of arguments");
      uint64_t matching_score, mismatch_penalty, gap_open_penalty, gap_extension_penalty;
      // Parse matching-score
      input_text_parse_extended_uint64(matching,&matching_score);
      // Parse mismatch-penalty
      input_text_parse_extended_uint64(mismatch,&mismatch_penalty);
      // Parse gap-open-penalty
      input_text_parse_extended_uint64(gap_open,&gap_open_penalty);
      // Parse gap-extension-penalty
      input_text_parse_extended_uint64(gap_extension,&gap_extension_penalty);
      // Configure scores
      search_configure_alignment_match_scores(&parameters->search_parameters,matching_score);
      search_configure_alignment_mismatch_scores(&parameters->search_parameters,mismatch_penalty);
      search->swg_penalties.gap_open_score = -((int32_t)gap_open_penalty);
      search->swg_penalties.gap_extension_score = -((int32_t)gap_extension_penalty);
      break;
    }
    case 'A': { // --matching-score (default=1)
      uint64_t matching_score;
      input_text_parse_extended_uint64(optarg,&matching_score);
      search_configure_alignment_match_scores(&parameters->search_parameters,matching_score);
      break;
    }
    case 'B': { // --mismatch-penalty (default=4)
      uint64_t mismatch_penalty;
      input_text_parse_extended_uint64(optarg,&mismatch_penalty);
      search_configure_alignment_mismatch_scores(&parameters->search_parameters,mismatch_penalty);
      break;
    }
    case 'O': { // --gap-open-penalty (default=6)
      uint64_t gap_open_penalty;
      input_text_parse_extended_uint64(optarg,&gap_open_penalty);
      search->swg_penalties.gap_open_score = -((int32_t)gap_open_penalty);
      break;
    }
    case 'X': { // --gap-extension-penalty (default=1)
      uint64_t gap_extension_penalty;
      input_text_parse_extended_uint64(optarg,&gap_extension_penalty);
      search->swg_penalties.gap_extension_score = -((int32_t)gap_extension_penalty);
      break;
    }
    case 702: // --gap-affine-threshold
      input_text_parse_extended_double(optarg,&search->swg_threshold);
      break;
    /* MAQ Score */
    case 800: // --mapq-model in {'none'|'gem'|'classify'} (default=gem)
      if (gem_strcaseeq(optarg,"none")) {
        parameters->select_parameters.mapq_model = mapq_model_none;
      } else if (gem_strcaseeq(optarg,"gem")) {
        parameters->select_parameters.mapq_model = mapq_model_gem;
      } else if (gem_strcaseeq(optarg,"classify")) {
        parameters->select_parameters.mapq_model = mapq_model_classify;
      } else {
        gem_mapper_error_msg("Option '--mapq-model' must be in {'none'|'gem'|'classify'}");
      }
      break;
    case 801: { // --mapq-threshold <number>
      uint64_t mapq_threshold;
      input_text_parse_extended_uint64(optarg,&mapq_threshold);
      parameters->select_parameters.mapq_threshold = MIN(255,mapq_threshold);
      break;
    }
    /* Reporting */
    case 'D': // --min-reported-strata
      input_text_parse_extended_double(optarg,&parameters->select_parameters.min_reported_strata);
      break;
    case 'm': // --min-reported-matches
      input_text_parse_extended_uint64(optarg,&parameters->select_parameters.min_reported_matches);
      break;
    case 'M': // --max-reported-matches
      input_text_parse_extended_uint64(optarg,&parameters->select_parameters.max_reported_matches);
      break;
    /*  Output-format */
    case 'F': // --output-format
      if (gem_strcaseeq(optarg,"MAP")) {
        parameters->io.output_format = MAP;
      } else if (gem_strcaseeq(optarg,"SAM")) {
        parameters->io.output_format = SAM;
      } else {
        gem_mapper_error_msg("Option '-F|--output-format' must be 'MAP' or 'SAM'");
      }
      break;
    case 1000: // --sam-compact in {'true'|'false'} (default=true)
      parameters->io.sam_parameters.compact_xa = (optarg==NULL) ? true : input_text_parse_extended_bool(optarg);
      break;
		case 'r': // --sam-read-group-header
			output_sam_parse_read_group_header(optarg,&parameters->io.sam_parameters);
		  break;
    case 1001: // --sam-gem-compatible in {'true'|'false'} (default=true)
      parameters->io.sam_parameters.print_gem_fields = (optarg==NULL) ? true : input_text_parse_extended_bool(optarg);
      break;
    case 1002: { // --map-format in {'1'|'2'|'3'} (default=1)
      const uint64_t format_version = atol(optarg);
      if (format_version==1) {
        parameters->io.map_parameters.format_version = map_format_v1;
      } else if (format_version==2) {
        parameters->io.map_parameters.format_version = map_format_v2;
      } else if (format_version==3) {
        parameters->io.map_parameters.format_version = map_format_v3;
      } else {
        gem_mapper_error_msg("Option '--map-format' must be in {'1'|'2'|'3'}");
      }
      break;
    }
    /* System */
    case 't': // --threads
      gem_mapper_cond_error_msg(parse_arguments_system_integer(optarg,&parameters->system.num_threads),
          "Option '--threads-cuda'. Error parsing 'num_selecting_threads'");
      break;
    case 1100: // --max-memory
      gem_cond_fatal_error(input_text_parse_size(optarg,&(parameters->system.max_memory)),PARSING_SIZE,"--max-memory",optarg);
      break;
    case 1101: // --tmp-folder
      parameters->system.tmp_folder = optarg;
      break;
    /* CUDA Settings */
    case 1200: // --cuda
      if (!gpu_supported()) GEM_CUDA_NOT_SUPPORTED();
      parameters->cuda.cuda_enabled = true;
      if (optarg) {
        if (gem_strcaseeq(optarg,"emulated")) {
          parameters->cuda.cpu_emulated = true;
        } else {
          gem_mapper_error_msg("Option '--cuda' invalid argument '%s'",optarg);
        }
      }
      break;
    case 1201: { // --cuda-buffers-per-thread=2,3,3,4M
      if (!gpu_supported()) GEM_CUDA_NOT_SUPPORTED();
      char *num_buffers=NULL, *buffer_size=NULL;
      const int num_arguments = input_text_parse_csv_arguments(optarg,2,&num_buffers,&buffer_size);
      gem_mapper_cond_error_msg(num_arguments!=2,"Option '--cuda-buffers-per-thread' wrong number of arguments");
      // Number of region-profile buffers per thread
      gem_mapper_cond_error_msg(input_text_parse_integer(
          (const char** const)&num_buffers,(int64_t*)&parameters->cuda.num_fmi_bsearch_buffers),
          "Option '--cuda-buffers-per-thread'. Error parsing 'region-profile buffers'");
      // Number of decode-candidates buffers per thread
      gem_mapper_cond_error_msg(input_text_parse_integer(
          (const char** const)&num_buffers,(int64_t*)&parameters->cuda.num_fmi_bsearch_buffers),
          "Option '--cuda-buffers-per-thread'. Error parsing 'decode-candidates buffers'");
      // Number of verify-candidates buffers per thread
      gem_mapper_cond_error_msg(input_text_parse_integer(
          (const char** const)&num_buffers,(int64_t*)&parameters->cuda.num_fmi_bsearch_buffers),
          "Option '--cuda-buffers-per-thread'. Error parsing 'verify-candidates buffers'");
      // Buffer size
      gem_mapper_cond_error_msg(input_text_parse_size(buffer_size,&parameters->cuda.gpu_buffer_size),
          "Option '--cuda-buffers-per-thread'. Error parsing 'buffer_size'");
      break;
    }
    /* Presets/Hints */
    case 1300: { // --reads-model <average_length>[,<std_length>]
      char *average_length, *std_length;
      const int num_arguments = input_text_parse_csv_arguments(optarg,2,&average_length,&std_length);
      gem_mapper_cond_error_msg(num_arguments==0,"Option '--reads-model' wrong number of arguments");
      // Average read length
      gem_mapper_cond_error_msg(input_text_parse_size(average_length,&parameters->hints.avg_read_length),
          "Option '--reads-model'. Error parsing 'average_length'");
      if (num_arguments > 1) {
        // Standard deviation read length
        gem_mapper_cond_error_msg(input_text_parse_size(std_length,&parameters->hints.std_read_length),
            "Option '--reads-model'. Error parsing 'std_length'");
      }
      break;
    }
    /* Debug */
    case 'c': { // --check-alignments in {'correct'|'best'|'complete'}
      select_parameters_t* const select_parameters = &parameters->select_parameters;
      if (!optarg) {
        select_parameters->check_type = archive_check_correct;
      } else if (gem_strcaseeq(optarg,"none")) {
        select_parameters->check_type = archive_check_nothing;
      } else if (gem_strcaseeq(optarg,"correct")) {
        select_parameters->check_type = archive_check_correct;
      } else if (gem_strcaseeq(optarg,"first-best") || gem_strcaseeq(optarg,"best")) {
        select_parameters->check_type = archive_check_correct__first_optimum;
      } else if (gem_strcaseeq(optarg,"all-best")) {
        select_parameters->check_type = archive_check_correct__all_optimum;
      } else if (gem_strcaseeq(optarg,"complete")) {
        select_parameters->check_type = archive_check_correct__complete;
      } else {
        gem_mapper_error_msg("Option '--check-alignments' must be 'correct'|'first-best'|'all-best'|'complete'");
      }
      break;
    }
    /* Misc */
    case 1500: // --profile in {'sum'|'min'|'max'|'mean'|'sample'}
      parameters->misc.profile = true;
      if (optarg) {
        if (gem_strcaseeq(optarg,"SUM")) {
          parameters->misc.profile_reduce_type = reduce_sum;
        } else if (gem_strcaseeq(optarg,"MIN")) {
          parameters->misc.profile_reduce_type = reduce_min;
        } else if (gem_strcaseeq(optarg,"MAX")) {
          parameters->misc.profile_reduce_type = reduce_max;
        } else if (gem_strcaseeq(optarg,"MEAN")) {
          parameters->misc.profile_reduce_type = reduce_mean;
        } else if (gem_strcaseeq(optarg,"SAMPLE")) {
          parameters->misc.profile_reduce_type = reduce_sample;
        } else {
          gem_mapper_error_msg("Option '--profile' must be 'sum'|'min'|'max'|'mean'|'sample'");
        }
      }
      break;
    case 'v': // -v|--verbose in {'quiet'|'user'|'dev'}
      if (optarg) {
        if (gem_strcaseeq(optarg,"quiet")) {
          parameters->misc.verbose_user = false;
          parameters->misc.verbose_dev = false;
        } else if (gem_strcaseeq(optarg,"user")) {
          parameters->misc.verbose_user = true;
          parameters->misc.verbose_dev = false;
        } else if (gem_strcaseeq(optarg,"dev") || gem_strcaseeq(optarg,"debug")) {
          parameters->misc.verbose_user = true;
          parameters->misc.verbose_dev = true;
        } else {
          gem_mapper_error_msg("Option '-v|--verbose' must be 'quiet'|'user'|'dev'");
        }
      } else {
        parameters->misc.verbose_user = true;
        parameters->misc.verbose_dev = false;
      }
      break;
    case 'h':
      if (optarg==NULL || gem_strcaseeq(optarg,"user")) {
        gem_mapper_print_usage(VISIBILITY_USER);
      } else if (gem_strcaseeq(optarg,"advanced")) {
        gem_mapper_print_usage(VISIBILITY_ADVANCED);
      } else if (gem_strcaseeq(optarg,"developer")) {
        gem_mapper_print_usage(VISIBILITY_DEVELOPER);
      } else {
        gem_mapper_error_msg("Help argument not valid {'user','advanced'}");
      }
      exit(0);
    case '?':
    default:
      gem_mapper_error_msg("Option not recognized");
    }
  }
  /*
   * Parameters Check
   */
  // I/O Parameters
  const char* const pindex = parameters->io.index_file_name;
  const char* const poutput = parameters->io.output_file_name;
  gem_mapper_cond_error_msg(pindex==NULL,"Index file required");
  if (!parameters->io.separated_input_files) {
    const char* const pinput = parameters->io.input_file_name;
    if (pinput!=NULL) {
      gem_mapper_cond_error_msg(gem_streq(pindex,pinput), "Index and Input-file must be different");
      if (poutput!=NULL) {
        gem_mapper_cond_error_msg(gem_streq(pinput,poutput),"Input-file and Output-file must be different");
        gem_mapper_cond_error_msg(gem_streq(pindex,poutput),"Index and Output-file must be different");
      }
    }
  } else {
    const char* const pinput_1 = parameters->io.input_file_name_end1;
    const char* const pinput_2 = parameters->io.input_file_name_end2;
    gem_mapper_cond_error_msg(pinput_1==NULL, "Missing Input-End1 (--i1)");
    gem_mapper_cond_error_msg(pinput_2==NULL, "Missing Input-End2 (--i2)");
    gem_mapper_cond_error_msg(gem_streq(pinput_1,pinput_2), "Input-End1 and Input-End2 must be different");
    gem_mapper_cond_error_msg(gem_streq(pindex,pinput_1), "Index and Input-End1 must be different");
    gem_mapper_cond_error_msg(gem_streq(pindex,pinput_2), "Index and Input-End2 must be different");
    if (poutput!=NULL) {
      gem_mapper_cond_error_msg(gem_streq(pinput_1,poutput),"Input-End1 and Output-file must be different");
      gem_mapper_cond_error_msg(gem_streq(pinput_2,poutput),"Input-End2 and Output-file must be different");
      gem_mapper_cond_error_msg(gem_streq(pindex,poutput),"Index and Output-file must be different");
    }
  }
  /*
   * Search Parameters
   */
  /* Mapping strategy (Mapping mode + properties) */
  if (paired_search->paired_end_search) {
    parameters->mapper_type = mapper_pe;
    gem_cond_warn_msg(parameters->search_parameters.bisulfite_read != bisulfite_read_inferred,
        "Option '--bisulfite_read' ignored with paired end mode");
  } else {
    parameters->mapper_type = mapper_se;
  }
  /* Qualities */
  gem_mapper_cond_error_msg(search->quality_threshold > 94,
      "Quality threshold is too high (please consider lowering it or run ignoring qualities)");
  gem_mapper_cond_error_msg(search->quality_threshold == 0,
      "Quality threshold is zero (all base-calls will be considered wrong)");
  /* Reporting */
  gem_mapper_cond_error_msg(parameters->select_parameters.min_reported_matches >
      parameters->select_parameters.max_reported_matches,
      "Option '--max-reported-matches' must be greater or equal than 'min-reported-matches'");
  gem_mapper_cond_error_msg(parameters->select_parameters.max_reported_matches == 0,
      "Option '--max-reported-matches' must be greater than zero'");
  // Free
  string_destroy(getopt_short_string);
  mm_free(getopt_short_string);
  mm_free(getopt_options);
}
/*
 * Main
 */
int main(int argc,char** argv) {
  // Parsing command-line options
  mapper_parameters_t parameters;
  mapper_parameters_set_defaults(&parameters); // Set defaults
  parse_arguments(argc,argv,&parameters); // Parse cmd-line

  // Runtime setup
  gem_timer_t mapper_time;
  const mapper_parameters_cuda_t* const cuda = &parameters.cuda;
  gem_runtime_init(parameters.system.num_threads+1,parameters.system.max_memory,parameters.system.tmp_folder);
  PROFILE_START(GP_MAPPER_ALL,PHIGH); TIMER_RESTART(&mapper_time);

  // Open Input/Output File(s)
  gem_mapper_open_input(&parameters);
  gem_mapper_open_output(&parameters);

  // Initialize Statistics Report
	if(parameters.io.report_file_name) parameters.global_mapping_stats=mm_alloc(mapping_stats_t);

  // Launch mapper
  if (!cuda->cuda_enabled) {
    switch (parameters.mapper_type) {
      case mapper_se:
        mapper_SE_run(&parameters);
        break;
      case mapper_pe:
        mapper_PE_run(&parameters);
        break;
      case mapper_graph:
        GEM_NOT_IMPLEMENTED(); // TODO
        break;
      default:
        GEM_INVALID_CASE();
        break;
    }
  } else {
    switch (parameters.mapper_type) {
      case mapper_se:
        mapper_cuda_se_run(&parameters);
        break;
      case mapper_pe:
        mapper_cuda_pe_run(&parameters);
        break;
      case mapper_graph:
        GEM_NOT_IMPLEMENTED(); // TODO
        break;
      default:
        GEM_INVALID_CASE();
        break;
    }
  }
  PROFILE_STOP(GP_MAPPER_ALL,PHIGH); TIMER_STOP(&mapper_time);

  // Profile
  if (parameters.misc.profile) gem_mapper_print_profile(&parameters);

	// Mapping Statistics Report
	if(parameters.io.report_file_name) {
		 output_mapping_stats(&parameters,parameters.global_mapping_stats);
	}

	// CleanUP
  archive_delete(parameters.archive); // Delete archive
  gem_mapper_close_input(&parameters); // Close I/O files
  gem_mapper_close_output(&parameters); // Close I/O files
  gem_runtime_destroy();

  // Display end banner
  const uint64_t mapper_time_sec = (uint64_t)TIMER_GET_TOTAL_S(&mapper_time);
  gem_cond_log(parameters.misc.verbose_user,"[GEMMapper terminated successfully in %"PRIu64" s.]\n",mapper_time_sec);

  // Done!
  return 0;
}
