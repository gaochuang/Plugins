CXX = g++
CXXFLAGS = -Wall -Wextra -pthread -O2 -fPIC -std=c++17

PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/libexec

LIBNAME = libcommonapidaemon

INCLUDES = -I/usr/local/include/comapi -Iinclude

SRCS = src/UdpLoggerPlugin.cpp    \
       src/LoggerPluginCreator.cpp \
	   src/Message.cpp

OBJS = $(SRCS:.cpp=.o)

SHARED_LIB = $(LIBNAME).so

all: $(SHARED_LIB)

$(SHARED_LIB): $(OBJS)
	@echo "Creating shared library $@"
	$(CXX) -shared -o $@ $(OBJS)

src/%.o: src/%.cpp
	@echo "Compiling $< into $@"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

install: all
	install -m 0755 $(SHARED_LIB) $(LIBDIR)

uninstall:
	rm -f $(LIBDIR)/$(SHARED_LIB)

clean:
	@echo "Cleaning up"
	rm -f $(OBJS) $(SHARED_LIB)

.PHONY: all clean
