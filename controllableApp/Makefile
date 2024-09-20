TARGET = controllableAPP

SRC = main.cpp

CXX = g++

CXXFLAGS = -Wall  -g -std=c++17

LIBDIR = ./

LIBS = -lcommonpidaemon -lractor -lcommonpicontrollableprocess

LDFLAGS = -L$(LIBDIR)

INCLUDES = -I../include/event -I../commonapicontrollableprocess/include -I../include/timer -I../include/plugin  -I../include/signalMonitor -I../include/engine -I../include 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -L$(LIBDIR) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
