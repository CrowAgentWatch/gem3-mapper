#==================================================================================================
# PROJECT: GEMMapper
# FILE: Makefile.mk
# DATE: 02/10/2012
# AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
# DESCRIPTION: Makefile definitions' file
#==================================================================================================

###############################################################################
# System
###############################################################################
PLATFORM=$(shell uname)

# Utilities
CC=gcc
AR=ar

###############################################################################
# Paths
###############################################################################

# Folders
FOLDER_BIN=$(ROOT_PATH)/bin
FOLDER_BUILD=$(ROOT_PATH)/build
FOLDER_DATASETS=$(ROOT_PATH)/datasets
FOLDER_INCLUDE=$(ROOT_PATH)/include
FOLDER_LIB=$(ROOT_PATH)/lib
FOLDER_RESOURCES=$(ROOT_PATH)/resources
FOLDER_RESOURCES_BUILD=$(ROOT_PATH)/resources/build

ifeq ($(PLATFORM),Darwin)
FOLDER_RESOURCES_LIB=$(ROOT_PATH)/resources/lib
else
FOLDER_RESOURCES_LIB=$(ROOT_PATH)/resources/lib
endif

FOLDER_RESOURCES_INCLUDE=$(ROOT_PATH)/resources/include
FOLDER_SOURCE=$(ROOT_PATH)/src
FOLDER_TEST=$(ROOT_PATH)/test
FOLDER_TOOLS=$(ROOT_PATH)/tools

###############################################################################
# Configure flags
###############################################################################

HAVE_ZLIB = 1
HAVE_BZLIB = 1
HAVE_OPENMP = 1
HAVE_CUDA = 1

###############################################################################
# General Flags
###############################################################################

# GEM Flags
LIB_GEM_CORE=$(FOLDER_LIB)/libgemcore_c.a
FLAGS_GEM_PROFILE=-DGEM_PROFILE
FLAGS_GEM_DEBUG=-DGEM_DEBUG

## Base flags (Generic Compiler)
FLAGS_GENERAL=-fPIC -Wall
FLAGS_SUPPRESS_CHECKS=-DNDEBUG
FLAGS_OPT=-O4
FLAGS_DEBUG=-g $(FLAGS_GEM_DEBUG)
FLAGS_PROFILE=$(FLAGS_GEM_PROFILE)

## GCC Compiler 
ifeq ($(CC),gcc) 
FLAGS_OPT=-Ofast -msse4.2
FLAGS_DEBUG=-g $(FLAGS_GEM_DEBUG) # -ggdb3 -rdynamic
endif

## ICC Compiler
ifeq ($(CC),icc)
FLAGS_OPT=-Ofast -msse4.2
FLAGS_DEBUG=-g $(FLAGS_GEM_DEBUG)
endif

###############################################################################
# Library Flags
###############################################################################

# Libs/Include Flags
PATH_INCLUDE=-I$(FOLDER_INCLUDE) -I$(FOLDER_RESOURCES_INCLUDE)
PATH_LIB=-L$(FOLDER_LIB) -L$(FOLDER_RESOURCES_LIB)

# Link Libs
LIBS=-lgemcore_c -lpthread -lm $(LIBS_ZLIB) $(LIBS_BZLIB)
ifeq ($(PLATFORM),Linux)
LIBS+=-lrt
endif

###############################################################################
# System dependent flags
###############################################################################

# OpenMP
ifeq ($(HAVE_OPENMP),1)
  DEF_OPENMP=-DHAVE_OPENMP
  LIB_OPENMP+=-fopenmp
  LIBS+=$(LIB_OPENMP)
endif

# GZip
ifeq ($(HAVE_ZLIB),1)
  DEF_ZLIB=-DHAVE_ZLIB
  LIB_ZLIB+=-lz
  LIBS+=$(LIB_ZLIB)
endif

# BZip
ifeq ($(HAVE_BZLIB),1)
  DEF_BZLIB=-DHAVE_BZLIB
  LIB_BZLIB+=-lbz2
  LIBS+=$(LIB_BZLIB)
endif

###############################################################################
# CUDA
###############################################################################
ifeq ($(HAVE_CUDA),1)
  # Paths
  DEF_CUDA=-DHAVE_CUDA
  PATH_CUDA=/usr/local/cuda
  NVCC=$(PATH_CUDA)/bin/nvcc
  # CUDA Libs
  CUDA_PATH_INCLUDE+=-I$(PATH_CUDA)/include
  CUDA_PATH_LIB+=-L$(PATH_CUDA)/lib64
  CUDA_PROFILE_LIB=-lnvToolsExt
  CUDA_LIB=-lcuda -lcudart -lstdc++
endif

###############################################################################
# VTune
###############################################################################
VTUNE_PROFILE_LIB=-ldl
LIBITTNOTIFY=/opt/intel/vtune_amplifier_xe_2013/lib64/libittnotify.a
ifeq ($(wildcard LIBITTNOTIFY),)
  VTUNE_PROFILE_LIB+=$(LIBITTNOTIFY)
endif
VTUNE_PROFILE_INCLUDE=-I/opt/intel/vtune_amplifier_xe_2013/include/


