CXXFLAGS = -std=c++14 -Wall

# Be default, the Release version will be made.
# 'make CONFIG=Debug all' will change this.

CONFIG=Release

ifeq ($(CONFIG),Release)
CXXFLAGS += -O2
else ifeq ($(CONFIG),Debug)
CXXFLAGS += -O0 -g
else
$(error Unknown build configuration: $(CONFIG))
endif

outputDir = $(CONFIG)

# On Fedora Linux (and maybe other distros) if the package boost is installed then
# there is no need to download and/or build your own and reference it.
# Set BOOSTDIR to the location you extracted to and built at.  You may need to mess
# with the values for LDLIBS and LDFLAGS.

# This is known to build with 1.58.  All later versions should work.

ifdef BOOSTDIR

CPPFLAGS += -I $(BOOSTDIR)

LDFLAGS += -L $(BOOSTDIR)/stage/lib

endif

LDLIBS += -l:libboost_program_options.a

sources = pcodedump.cpp options.cpp textio.cpp directory.cpp text.cpp basecode.cpp pcode.cpp native6502.cpp linkage.cpp

objects = $(addprefix $(outputDir)/,$(sources:.cpp=.o))

target = $(outputDir)/pcodedump

all: $(target)

$(target): $(objects)
	$(CXX) $(LDFLAGS) -o $(target) $(objects) $(LDLIBS)

$(outputDir)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(objects): | $(outputDir)
 
$(outputDir):
	mkdir $(outputDir)

clean:
	rm -Rf $(outputDir)

.PHONY: clean all
