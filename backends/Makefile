# Makefile to build the Backends included in this subfolder

include ../Makefile.inc

CXXFLAGS += -I../include -I../protobuf -I/usr/local/include/google
LDFLAGS = -L../src -lbackend $(ZMQ_LDFLAGS) $(MESSAGES_LDFLAGS) $(BOOST_LDFLAGS) $(PBUF_LDFLAGS)

BACKEND_SRC = RegistrationBackend.cpp
BACKEND_OBJ = $(BACKEND_SRC:.cpp=.o)
BACKEND_TARGET = registration_backend

all: $(BACKEND_TARGET)

.PHONY: clean install

$(BACKEND_TARGET): $(BACKEND_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -L. -lfrontend $(BACKEND_OBJ) -o $(BACKEND_TARGET)

install:
	@echo "Copying stuff in ${INSTALL_DIR}"
	@cp $(BACKEND_TARGET) ${INSTALL_DIR}
	@cp RegistrationBackendBE.xml ${INSTALL_DIR}

clean:
	rm -f $(BACKEND_TARGET) $(BACKEND_OBJ)
