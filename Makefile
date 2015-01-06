# These should be defined in .config
#GECKO_ROOT = /Volumes/fennec/gecko-desktop
#GECKO_OBJ = $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0
#For Linux:
#SDL_INCLUDE_PATH = path/to/sdl/header/files
#SDL_LIB_PATH = path/to/sdl/libs

include .config
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
$(GECKO_OBJ)/media/libspeex_resampler/src/libspeex.a.desc \
$(GECKO_OBJ)/netwerk/srtp/src/libnksrtp_s.a.desc \
$(GECKO_OBJ)/media/mtransport/standalone/libmtransport_standalone.a.desc \
$(GECKO_OBJ)/media/webrtc/signalingstandalone/signaling_ecc/libecc.a.desc \
$(GECKO_OBJ)/xpcom/libxpcomrt/libxpcomrt.a.desc \
$(GECKO_OBJ)/dom/media/standalone/libmedia_standalone.a.desc \
$(GECKO_OBJ)/netwerk/standalone/libnecko_standalone.a.desc \
$(GECKO_OBJ)/intl/unicharutil/util/standalone/libunicharutil_standalone.a.desc \
$(GECKO_OBJ)/security/nss/lib/pk11wrap/static/libpk11wrap_s.a.desc \
$(GECKO_OBJ)/security/nss/lib/freebl/static/libfreebl_s.a.desc \
$(GECKO_OBJ)/db/sqlite3/src/libdb_sqlite3_src.a.desc \
$(GECKO_OBJ)/media/libyuv/libyuv_libyuv/libyuv.a.desc

LIB_ROLLUP = $(BUILD_DIR)/librollup.a

JSON = $(BUILD_DIR)/libyajl.a

all: player glplayer

$(JSON):
	cd 3rdparty/yajl; make

OBJS = \
$(BUILD_DIR)/player.o \
$(BUILD_DIR)/json.o

player: $(OBJS) $(LIB_ROLLUP) $(BUILD_DIR)/renderSDL2.o $(JSON)
	$(CXX) $(OBJS) $(BUILD_DIR)/renderSDL2.o $(LIB_ROLLUP) $(JSON) $(SDL_LFLAGS) $(LFLAGS) -o $@

glplayer: $(OBJS) $(LIB_ROLLUP) $(BUILD_DIR)/renderGL.o $(JSON)
	$(CXX) $(OBJS) $(BUILD_DIR)/renderGL.o $(LIB_ROLLUP) $(JSON) $(SDL_LFLAGS) $(LFLAGS) -o $@

stubplayer: $(OBJS) $(LIB_ROLLUP) $(BUILD_DIR)/renderStub.o $(JSON)
	$(CXX) $(OBJS) $(BUILD_DIR)/renderStub.o $(LIB_ROLLUP) $(JSON) $(SDL_LFLAGS) $(LFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) $^ -c -o $@

$(LIB_ROLLUP): $(LIBS)
	@mkdir -p $(BUILD_DIR)
	$(AR) cr $@ `python ./tools/expand.py $(LIBS)`

clean:
	rm -f $(LIB_ROLLUP) $(JSON) $(BUILD_DIR)/*.o

clobber: clean
	rm -f player
	rm -f glplayer
	rm -rf $(BUILD_DIR)
