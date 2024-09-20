CXX = g++
CXXFLAGS = -Wall -Wextra -pthread -O2 -fPIC -std=c++17

LIBNAME = libcommonpidaemon

INCLUDES = -Iinclude/ -I../include/ -I../commonapicontrollableprocess/include

SRCS = src/Plugin.cpp

OBJS = $(SRCS:.cpp=.o)

STATIC_LIB = $(LIBNAME).a
SHARED_LIB = $(LIBNAME).so

all: $(STATIC_LIB) $(SHARED_LIB)

$(STATIC_LIB): $(OBJS)
	@echo "Creating static library $@"
	ar rcs $@ $(OBJS)

$(SHARED_LIB): $(OBJS)
	@echo "Creating shared library $@"
	$(CXX) -shared -o $@ $(OBJS)

src/%.o: src/%.cpp
	@echo "Compiling $< into $@"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Cleaning up"
	rm -f $(OBJS) $(STATIC_LIB) $(SHARED_LIB)

.PHONY: all clean
