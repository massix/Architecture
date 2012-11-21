# Makefile to build the test unit

include ../Makefile.inc

CXXFLAGS += -I../protobuf -I../include -I/usr/local/include/google
LDFLAGS = $(PBUF_LDFLAGS) $(MESSAGES_LDFLAGS) $(COMPONENTS_LDFLAGS) $(ZMQ_LDFLAGS) $(BOOST_LDFLAGS)
TEST_SRC = $(wildcard *.cpp)
TEST_OBJ = $(TEST_SRC:.cpp=.o)
TARGET = test

all: $(TARGET)

.PHONY: clean

$(TARGET): $(TEST_OBJ)
	$(CXX) $(LDFLAGS) $(TEST_OBJ) -o $(TARGET)

install:
	@echo "Installing in ${INSTALL_DIR}"
	@mkdir -p ${INSTALL_DIR}
	cp $(TARGET) ${INSTALL_DIR}

clean:
	rm -f $(TARGET)
	rm -f $(TEST_OBJ)
