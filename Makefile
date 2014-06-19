# These should be defined in .geckopaths
#GECKO_ROOT = /Volumes/fennec/gecko-desktop
#GECKO_OBJ = $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0
#For Linux:
#SDL_INCLUDE_PATH = path/to/sdl/header/files
#SDL_LIB_PATH = path/to/sdl/libs

include .geckopaths
PLATFORM:=$(shell uname)
include platforms/common.mk
include platforms/$(PLATFORM).mk

BUILD_DIR=./build-$(PLATFORM)

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
$(GECKO_OBJ)/media/webrtc/signalingtest/signaling_sipcc/libsipcc.a.desc \
$(GECKO_OBJ)/media/libyuv/libyuv_libyuv/libyuv.a.desc

LIB_ROLLUP = $(BUILD_DIR)/librollup.a

all: player

player: $(BUILD_DIR)/player.o $(LIB_ROLLUP) $(BUILD_DIR)/$(RENDERNAME).o
	$(CXX) $(BUILD_DIR)/player.o $(BUILD_DIR)/$(RENDERNAME).o $(LIB_ROLLUP) $(SDL_LFLAGS) $(LFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) $^ -c -o $@

$(LIB_ROLLUP): $(LIBS)
	@mkdir -p $(BUILD_DIR)
	$(AR) cr $@ `python ./tools/expand.py $(LIBS)`

clean:
	rm -f $(LIB_ROLLUP) $(BUILD_DIR)/player.o $(BUILD_DIR)/$(RENDERNAME).o

clobber: clean
	rm -f player
	rm -rf $(BUILD_DIR)
