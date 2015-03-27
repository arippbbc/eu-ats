UNAME:=$(shell uname)

CXX=g++
CXXFLAGS=-DIB_USE_STD_STRING -w -Wno-switch -Wno-sign-compare -O2
BASE_SRC_DIR=${ROOT_DIR}/PosixClient/src
ifeq ($(UNAME), Linux)
ROOT_DIR = ${HOME}/tws/IBJts/source
INCLUDES=-I${ROOT_DIR}/PosixClient/Shared -I${BASE_SRC_DIR}
LFLAGS=
else
ROOT_DIR=C:/TWSAPI/source/PosixClient
BASE_SRC_DIR=${ROOT_DIR}/src
INCLUDES=-I${ROOT_DIR}/Shared/ -I${BASE_SRC_DIR}
#ROOT_DIR=c:/TWSAPI/source
#INCLUDES=-I${ROOT_DIR}/CppClient/Shared -I${BASE_SRC_DIR}
LFLAGS=-lws2_32
endif
CXXFLAGS:=$(CXXFLAGS) $(INCLUDES)

SOURCES=Client.cpp Data.cpp Contracts.cpp Main.cpp
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
