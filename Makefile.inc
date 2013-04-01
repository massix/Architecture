# Common stuff

SYSTEM = $(shell uname)

CXX = clang++
FRAMEWORK_VERSION=0x04010D
VERSIONS = \
					 -D__FRONTEND_VERSION__=${FRAMEWORK_VERSION} \
					 -D__BACKEND_VERSION__=${FRAMEWORK_VERSION} \
					 -D__RECEPTOR_VERSION__=${FRAMEWORK_VERSION}

CXXFLAGS = -g -Wall -I. -I/usr/local/include -DDEBUG=1 ${VERSIONS}

ifeq ($(SYSTEM), Darwin)
LIB_EXT = .dylib
endif

ifeq ($(SYSTEM), Linux)
LIB_EXT = .so
CXXFLAGS += -fPIC -I/usr/include/google
endif

PBUF_LDFLAGS = -L/usr/local/lib -lprotobuf
ZMQ_LDFLAGS = -L/usr/local/lib -lzmq -lz
MESSAGES_LDFLAGS = -L../protobuf -lmessages
COMPONENTS_LDFLAGS = -L../src -lreceptor -lfrontend -lbackend
BOOST_LDFLAGS = -L/usr/local/lib -lboost_system-mt -lboost_program_options-mt -lboost_thread-mt

INSTALL_DIR = $(shell pwd)/../build

# vim: filetype=make:ts=2

