# These should be defined in .geckopaths
#GECKO_ROOT = /Volumes/fennec/gecko-desktop
#GECKO_OBJ = $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0

include .geckopaths
PLATFORM=$(shell uname)
include platforms/common.mk
include platforms/$(PLATFORM).mk

BUILD_DIR=./build

LIBS = \
$(GECKO_OBJ)/mfbt/libmfbt.a.desc \
$(GECKO_OBJ)/layout/media/webrtc/libwebrtc.a.desc \
$(GECKO_OBJ)/media/libopus/libmedia_libopus.a.desc \
$(GECKO_OBJ)/media/libvpx/libmedia_libvpx.a.desc \
$(GECKO_OBJ)/media/libjpeg/libmedia_libjpeg.a.desc \
$(GECKO_OBJ)/media/libspeex_resampler/src/libmedia_libspeex_resampler_src.a.desc \
$(GECKO_OBJ)/netwerk/srtp/src/libnksrtp_s.a.desc \
$(GECKO_OBJ)/media/mtransport/standalone/libmtransport_s.a.desc \
$(GECKO_OBJ)/media/webrtc/signalingtest/signaling_ecc/libecc.a.desc \
$(GECKO_OBJ)/media/webrtc/signalingtest/signaling_sipcc/libsipcc.a.desc

LIB_ROLLUP = $(BUILD_DIR)/librollup.a

testapp: $(BUILD_DIR)/testapp.o $(BUILD_DIR)/WebRTCCall.o $(LIB_ROLLUP)
	$(CXX) $(BUILD_DIR)/testapp.o $(BUILD_DIR)/WebRTCCall.o $(LIB_ROLLUP) $(LFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(INCLUDE) $^ -c -o $@

$(LIB_ROLLUP): $(LIBS)
	@mkdir -p $(BUILD_DIR)
	$(AR) cr $@ `python ./tools/expand.py $(LIBS)`

clean:
	rm -f $(LIB_ROLLUP) $(BUILD_DIR)/testapp.o $(BUILD_DIR)/WebRTCCall.o

clobber: clean
	rm -f testapp
	rm -rf $(BUILD_DIR)
