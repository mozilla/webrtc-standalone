-include .geckopaths
PLATFORM=$(shell uname)
include platforms/common.mk
include platforms/$(PLATFORM).mk

# These should be defined in .geckopaths
#GECKO_ROOT := /Volumes/fennec/gecko-desktop
#GECKO_OBJ := $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0

vpath %.a.desc $(GECKO_OBJ)/dist/lib/
vpath %.a.desc $(GECKO_OBJ)/media/mtransport/standalone/
vpath %.a.desc $(GECKO_OBJ)/media/webrtc/signalingtest/signaling_sipcc
vpath %.a.desc $(GECKO_OBJ)/media/webrtc/signalingtest/signaling_ecc
vpath %.a.desc $(GECKO_OBJ)/layout/media/webrtc
vpath %.a.desc $(GECKO_OBJ)/netwerk/srtp/src
vpath %.a.desc $(GECKO_OBJ)/modules/zlib/src

vpath %.a ./build

BUILD_DIR=./build

LIBS = \
$(GECKO_OBJ)/dist/lib/libxpcomglue_s.a.desc \
$(GECKO_OBJ)/media/mtransport/standalone/libmtransport_s.a.desc \
$(GECKO_OBJ)/media/webrtc/signalingtest/signaling_ecc/libecc.a.desc \
$(GECKO_OBJ)/media/webrtc/signalingtest/signaling_sipcc/libsipcc.a.desc \
$(GECKO_OBJ)/layout/media/webrtc/libwebrtc.a.desc \
$(GECKO_OBJ)/dist/lib/libgkmedias.a.desc \
$(GECKO_OBJ)/netwerk/srtp/src/libnksrtp_s.a.desc \
$(GECKO_OBJ)/modules/zlib/src/libmozz.a.desc

LIB_ROLLUP = $(BUILD_DIR)/librollup.a


testapp: $(BUILD_DIR)/testapp.o $(LIB_ROLLUP)
	$(CXX) $< $(LIB_ROLLUP) $(LFLAGS) -o $@

$(BUILD_DIR)/testapp.o: testapp.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(INCLUDE) $^ -c -o $@

$(LIB_ROLLUP): $(LIBS)
	@mkdir -p $(BUILD_DIR)
	$(AR) cr $@ `python ./tools/expand.py $(LIBS)`

clean:
	rm -f $(LIB_ROLLUP) $(BUILD_DIR)/testapp.o

clobber: clean
	rm -f testapp
	rm -rf $(BUILD_DIR)
