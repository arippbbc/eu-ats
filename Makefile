UNAME:=$(shell uname)

CXX=g++
CXXFLAGS=-DIB_USE_STD_STRING -w -Wno-switch -Wno-sign-compare -Wfatal-errors -O2 -std=c++11
ROOT_DIR = PosixClient
BASE_SRC_DIR=${ROOT_DIR}/src
INCLUDES=-I${ROOT_DIR}/Shared/ -I${BASE_SRC_DIR}
ifeq ($(UNAME), Linux)
LFLAGS=
else
LFLAGS=-lws2_32
endif
CXXFLAGS:=$(CXXFLAGS) $(INCLUDES)

SOURCES=Data.cpp Instrument.cpp Client.cpp Main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
LIB=lib/EClientSocketBase.o lib/EPosixClientSocket.o

EXECUTABLE=fx.exe

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(CXXFLAGS) $(LIB) $(OBJECTS)  -o $@ $(LFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
