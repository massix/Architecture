# Common stuff

CXX = clang++
CXXFLAGS = -Wall -I. -I/usr/local/include -DDEBUG=1

LIB_EXT = .dylib

PBUF_LDFLAGS = -L/usr/local/lib -lprotobuf
ZMQ_LDFLAGS = -L/usr/local/lib -lzmq -lz
MESSAGES_LDFLAGS = -L../protobuf -lmessages
COMPONENTS_LDFLAGS = -L../src -lreceptor -lfrontend
BOOST_LDFLAGS = -L/usr/local/lib -lboost_system-mt -lboost_program_options-mt

INSTALL_DIR = $(shell pwd)/../build

# vim: set filetype=make;
