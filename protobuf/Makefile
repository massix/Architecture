# Makefile to build libmessage.dylib

include ../Makefile.inc

LDFLAGS = $(PBUF_LDFLAGS)

PBBIN = $(shell which protoc)

PBSRC = $(wildcard *.proto)
PBGEN = $(PBSRC:.proto=.pb.cc) $(PBSRC:.proto=.pb.h)
PBOBJ = $(PBSRC:.proto=.pb.o)

TARGET = libmessages${LIB_EXT}

all: $(TARGET)

.PRECIOUS: $(PBGEN)
.PHONY: clean

%.pb.cc: %.proto
	$(PBBIN) --cpp_out=. $<

%.pb.o: %.pb.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TARGET): $(PBOBJ)
	$(CXX) $(LDFLAGS) $(PBOBJ) -shared -o $(TARGET)

install: $(TARGET)
	@echo "Generating links in ${INSTALL_DIR}"
	@mkdir -p ${INSTALL_DIR}
	@ln -s `pwd`/$(TARGET) ${INSTALL_DIR}/${TARGET}

clean:
	rm -f $(TARGET)
	rm -f $(PBGEN)
	rm -f $(PBOBJ)
