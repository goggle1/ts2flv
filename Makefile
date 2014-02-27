
###############################################################################
#
# Generic Makefile for C/C++ Program
#
# Author: guoqiang (guoqiang AT funshion DOT com)
# Date:   2012/01/31
# Description:
# The makefile searches in <SRCDIRS> directories for the source files
# with extensions specified in <SOURCE_EXT>, then compiles the sources
# and finally produces the <PROGRAM>, the executable file, by linking
# the objectives.
# Usage:
#   $ make           compile and link the program.
#   $ make objs      compile only (no linking. Rarely used).
#   $ make clean     clean the objectives and dependencies.
#   $ make cleanall  clean the objectives, dependencies and executable.
#   $ make rebuild   rebuild the program. The same as make clean && make all.
#==============================================================================
## Customizing Section: adjust the following if necessary.
##=============================================================================
# The executable file name.
# It must be specified.
# PROGRAM   := a.out    # the executable name
PROGRAM   := ts2flv

# The directories in which source files reside.
# At least one path should be specified.
# SRCDIRS   := .        # current directory
SRCDIRS   := . ./BaseServer

# The source file types (headers excluded).
# At least one type should be specified.
# The valid suffixes are among of .c, .C, .cc, .cpp, .CPP, .c++, .cp, or .cxx.
# SRCEXTS   := .c      # C program
# SRCEXTS   := .cpp    # C++ program
# SRCEXTS   := .c .cpp # C/C++ program
SRCEXTS   := .c .cpp

# The flags used by the cpp (man cpp for more).
# CPPFLAGS  := -Wall -Werror # show all warnings and take them as errors
CPPFLAGS  := -g -Wall -Wno-write-strings -O0 #-include BaseServer/PlatformHeader.h

# The compiling flags used only for C.
# If it is a C++ program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C parts.
CFLAGS    := -g -Wall -Wno-write-strings -O0 #-include BaseServer/PlatformHeader.h

# The compiling flags used only for C++.
# If it is a C program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C++ parts.
CXXFLAGS  := -g -Wall -Wno-write-strings -O0 #-include BaseServer/PlatformHeader.h

# The library and the link options ( C and C++ common).
LDFLAGS   :=
#LDFLAGS   += -lpthread -lxml2
#LDFLAGS   += -lxml2 -Wl,-Bstatic -L./librtmp/ -lrtmp -Wl,-Bdynamic # link static librtmp.a
#LDFLAGS   += -lz -lgnutls  -lgcrypt -lssl -lcrypto # .so used by librtmp.a

## Implict Section: change the following only when necessary.
##=============================================================================
# The C program compiler. Uncomment it to specify yours explicitly.
CC      = gcc
# The C++ program compiler. Uncomment it to specify yours explicitly.
CXX     = g++
# Uncomment the 2 lines to compile C programs as C++ ones.
#CC      = $(CXX)
#CFLAGS  = $(CXXFLAGS)
# The command used to delete file.
#RM        = rm -f
## Stable Section: usually no need to be changed. But you can add more.
##=============================================================================

SHELL   = /bin/sh
SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS    = $(foreach x,$(SRCEXTS), \
      $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS    = $(patsubst %.o,%.d,$(OBJS))

.PHONY : all objs clean cleanall rebuild 

all : $(PROGRAM)
	
# Rules for creating the dependency files (.d).
#---------------------------------------------------

%.d : %.c
	@$(CC) -MM -MD $(CFLAGS) $< -o $@

%.d : %.C
	@$(CC) -MM -MD $(CXXFLAGS) $< -o $@

%.d : %.cc
	@$(CC) -MM -MD $(CXXFLAGS) $< -o $@

%.d : %.cpp
	@$(CC) -MM -MD $(CXXFLAGS) $< -o $@

%.d : %.CPP
	@$(CC) -MM -MD $(CXXFLAGS) $< -o $@

%.d : %.c++
	@$(CC) -MM -MD $(CXXFLAGS) $< -o $@

%.d : %.cp
	@$(CC) -MM -MD $(CXXFLAGS) $< -o $@

%.d : %.cxx
	@$(CC) -MM -MD $(CXXFLAGS) $< -o $@

# Rules for producing the objects.
#---------------------------------------------------
objs : $(OBJS)

%.o : %.c
	$(CC) -c $(CFLAGS) $<  -o $@

%.o : %.C
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

%.o : %.CPP
	$(CXX) -c $(CPPFLAGS) $< -o $@

%.o : %.c++
	$(CXX -c $(CPPFLAGS) $< -o $@

%.o : %.cp
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.o : %.cxx
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Rules for producing the executable.
#----------------------------------------------
$(PROGRAM) : $(OBJS)
ifeq ($(strip $(SRCEXTS)), .c)  # C file
	$(CC) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
else                            # C++ file
	$(CXX) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
endif

-include $(DEPS)

rebuild: clean all

clean :	
	@$(RM) $(OBJS) $(DEPS) $(PROGRAM)

cleanall: clean
	@$(RM) $(PROGRAM) $(PROGRAM).exe

### End of the Makefile ##  Suggestions are welcome  ## All rights reserved ###
###############################################################################

