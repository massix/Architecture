# Common stuff

SYSTEM = $(shell uname)

CXX = clang++
CXXFLAGS = -g -Wall -I. -I/usr/local/include -DDEBUG=1

ifeq ($(SYSTEM), Darwin)
LIB_EXT = .dylib
endif

ifeq ($(SYSTEM), Linux)
LIB_EXT = .so
endif

PBUF_LDFLAGS = -L/usr/local/lib -lprotobuf
ZMQ_LDFLAGS = -L/usr/local/lib -lzmq -lz
MESSAGES_LDFLAGS = -L../protobuf -lmessages
COMPONENTS_LDFLAGS = -L../src -lreceptor -lfrontend
BOOST_LDFLAGS = -L/usr/local/lib -lboost_system-mt -lboost_program_options-mt

INSTALL_DIR = $(shell pwd)/../build

# vi: ft=make
