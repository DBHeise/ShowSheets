#~/user/bin/make -f
EXECUTABLE = showsheets
SO_LIBRARY = showsheets.so

CC        = gcc
CXX       = g++
CFLAGS    = -fPIC -Wno-enum-conversion -O3
CXXFLAGS  = -fPIC -std=c++11 -O3 -Wfatal-errors -Werror
LDFLAGS   = -pthread

SRC_SHOWSHEETS = \
	src/showsheets.cpp \
	src/pole.cpp


# Object files			
OBJS = \
    $(SRC_SHOWSHEETS:.cpp=.o)

# Rules
all: $(EXECUTABLE) $(SO_LIBRARY)

$(EXECUTABLE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CPP_FILES) -o $@ $^

$(SO_LIBRARY): $(OBJS)
	$(CXX) $(LDFLAGS) -shared -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf `find Source -name '*.o'` $(EXECUTABLE) $(SO_LIBRARY)


BINDIR ?= ${PREFIX}/bin

install:
	@cp -p bin/showsheets ${PREFIX}/showsheets
