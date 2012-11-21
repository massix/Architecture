# Makefile to build the Messages Injector

include ../Makefile.inc

CXXFLAGS += -I../protobuf

LDFLAGS = $(PBUF_LDFLAGS) $(ZMQ_LDFLAGS) $(BOOST_LDFLAGS) $(MESSAGES_LDFLAGS)

MESSAGESINJECTOR_SRC = $(wildcard *.cpp)
MESSAGESINJECTOR_OBJ = $(MESSAGESINJECTOR_SRC:.cpp=.o)

TARGET = messagesinjector

.PHONY: clean

all: $(TARGET)

$(TARGET): $(MESSAGESINJECTOR_OBJ)
	$(CXX) $(LDFLAGS) $(MESSAGESINJECTOR_OBJ) -o $(TARGET)

install: $(TARGET)
	@echo "Installing in ${INSTALL_DIR}"
	@mkdir -p ${INSTALL_DIR}
	cp $(TARGET) ${INSTALL_DIR}

clean:
	rm -f $(MESSAGESINJECTOR_OBJ) $(TARGET)
