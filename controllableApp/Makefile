TARGET = controllableAPP

SRC = main.cpp

CXX = g++

MINISTARTER_PROCESS_TYPE_ENV_VAR_NAME = MINISTARTER_PROCESS_TYPE

CXXFLAGS = -Wall -g -std=c++17

INCLUDES = -I/usr/local/include/comapi


LIBS = /usr/local/lib64/comapi/libcommapiengine.so /usr/local/lib64/comapi/libcommonapicontrollableprocess.so

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
