CXX=gcc
CXXSTD=
CXXOPTIMFLAGS=-O3 -g -flto -msse4.1 -mfma
CXXDEBUGFLAGS=-O0 -g -rdynamic
CXXFLAGS=-Wall $(CXXOPTIMFLAGS)
LIBS=-lpng -lm

# find source files
SOURCEDIR := $(shell pwd)
BUILDDIR := $(shell pwd)
SOURCES := $(shell find $(SOURCEDIR) -path $(SOURCEDIR)/benchmark -prune -o -name '*.c' -print)
#OBJECTS := $(addprefix $(BUILDDIR)/,$(notdir $(SOURCES:.cpp=.o)))
OBJECTS := $(SOURCES:.c=.o)

# link
main.x: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o termpng $(LIBS)

# pull dependencies for existing .o files
-include $(OBJECTS:.o=.d)

# compile and generate dependency info
%.o: %.c %.d
	$(CXX) $(CXXSTD) -c $(CXXFLAGS) $*.c -o $*.o
	$(CXX) $(CXXSTD) -MM $(CXXFLAGS) $*.c > $*.d

# empty rule for dependency files
%.d: ;

clean:
	rm -f *core *.o *.d src/*.o src/*.d
