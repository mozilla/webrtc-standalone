-include .geckopaths
#GECKO_ROOT := /Volumes/fennec/gecko-desktop
#GECKO_OBJ := $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0

INCLUDE = \
-I$(GECKO_ROOT)/media/webrtc/signaling/test \
-I$(GECKO_ROOT)/media/webrtc/trunk/testing/gtest/include \
-I$(GECKO_ROOT)/ipc/chromium/src \
-I$(GECKO_ROOT)/media/mtransport \
-I$(GECKO_ROOT)/media/mtransport/test \
-I$(GECKO_ROOT)/media/webrtc/signaling/include \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/sipcc/core/sdp \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/sipcc/cpr/include \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/sipcc/core/includes \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/common/browser_logging \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/common/time_profiling \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/media \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/media-conduit \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/mediapipeline \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/sipcc/include \
-I$(GECKO_ROOT)/media/webrtc/signaling/src/peerconnection \
-I$(GECKO_ROOT)/media/webrtc/signaling/media-conduit \
-I$(GECKO_ROOT)/media/webrtc/trunk/third_party/libjingle/source/ \
-I$(GECKO_ROOT)/media/mtransport/third_party/nICEr/src/ice \
-I$(GECKO_ROOT)/media/mtransport/third_party/nICEr/src/net \
-I$(GECKO_ROOT)/media/mtransport/third_party/nICEr/src/stun \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/share \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/util/libekr \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/log \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/registry \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/stats \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/plugin \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/event \
-I$(GECKO_ROOT)/xpcom/base/ \
-I$(GECKO_OBJ)/dom/bindings/ \
-I$(GECKO_ROOT)/media/mtransport/third_party/nrappkit/src/port/darwin/include \
-I$(GECKO_ROOT)/ipc/chromium/src \
-I$(GECKO_ROOT)/ipc/glue \
-I$(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0/ipc/ipdl/_ipdlheaders \
-I$(GECKO_ROOT)/media/webrtc/trunk/webrtc \
-I$(GECKO_OBJ)/dist/include \
-I$(GECKO_OBJ)/dist/include/nspr \
-I$(GECKO_OBJ)/dist/include/nss \
-I$(GECKO_OBJ)/dist/include/testing \

CFLAGS = \
-fvisibility=hidden \
-DOS_POSIX=1 \
-DOS_MACOSX=1 \
-DNR_SOCKET_IS_VOID_PTR \
-DHAVE_STRDUP \
-DNO_NSPR_10_SUPPORT \
-DNO_X11 \
-DMOZILLA_CLIENT \
-DNDEBUG \
-DTRIMMED \
-DUSE_FAKE_MEDIA_STREAMS \
-DUSE_FAKE_PCOBSERVER \
-fPIC \
-Qunused-arguments \
-include $(GECKO_OBJ)/mozilla-config.h \
-MD \
-MP \
-Wall \
-Wpointer-arith \
-Woverloaded-virtual \
-Werror=return-type \
-Wtype-limits \
-Wempty-body \
-Wsign-compare \
-Wno-invalid-offsetof \
-Wno-c++0x-extensions \
-Wno-extended-offsetof \
-Wno-unknown-warning-option \
-Wno-return-type-c-linkage \
-Wno-mismatched-tags \
-fno-exceptions \
-fno-strict-aliasing \
-fno-rtti \
-fno-exceptions \
-fno-math-errno \
-std=gnu++0x \
-pthread \
-pipe \
-g \
-O3 \
-fomit-frame-pointer

LFLAGS = \
-L$(GECKO_OBJ)/dist/bin \
-L$(GECKO_OBJ)/dist/lib \
-Qunused-arguments \
-Wall \
-Wpointer-arith \
-Woverloaded-virtual \
-Werror=return-type \
-Wtype-limits \
-Wempty-body \
-Wsign-compare \
-Wno-invalid-offsetof \
-Wno-c++0x-extensions \
-Wno-extended-offsetof \
-Wno-unknown-warning-option \
-Wno-return-type-c-linkage \
-Wno-mismatched-tags \
-fno-exceptions \
-fno-strict-aliasing \
-fno-rtti \
-fno-exceptions \
-fno-math-errno \
-std=gnu++0x \
-pthread \
-DNO_X11 \
-pipe \
-DNDEBUG \
-DTRIMMED \
-g \
-O3 \
-fomit-frame-pointer \
-o a.out \
-framework Cocoa \
-lobjc \
-framework ExceptionHandling \
-Wl,-executable_path,$(GECKO_OBJ)/dist/bin \
-Wl,-dead_strip \
$(GECKO_OBJ)/dist/bin/XUL \
-lnspr4 \
-lplc4 \
-lplds4 \
-lcrmf \
-lsmime3 \
-lssl3 \
-lnss3 \
-lnssutil3 \
-framework AudioToolbox \
-framework AudioUnit \
-framework Carbon \
-framework CoreAudio \
-framework OpenGL \
-framework QTKit \
-framework QuartzCore \
-framework Security \
-framework SystemConfiguration \
-framework IOKit \
-F/System/Library/PrivateFrameworks \
-framework CoreUI \
-framework CoreLocation \
-framework QuartzCore \
-framework Carbon \
-framework CoreAudio \
-framework AudioToolbox \
-framework AudioUnit \
-framework AddressBook \
-framework OpenGL \
-lnspr4 \
-lplc4 \
-lplds4 \
-lmozglue \
-lmozalloc

vpath %.a.desc $(GECKO_OBJ)/dist/lib/
vpath %.a.desc $(GECKO_OBJ)/media/mtransport/standalone/
vpath %.a.desc $(GECKO_OBJ)/media/webrtc/signalingtest/signaling_sipcc
vpath %.a.desc $(GECKO_OBJ)/media/webrtc/signalingtest/signaling_ecc
vpath %.a.desc $(GECKO_OBJ)/layout/media/webrtc
vpath %.a.desc $(GECKO_OBJ)/netwerk/srtp/src
vpath %.a.desc $(GECKO_OBJ)/modules/zlib/src

vpath %.a ./build

BUILD_DIR=./build

LIBNAMES = \
libxpcomglue_s.a \
libmtransport_s.a \
libecc.a \
libsipcc.a \
libwebrtc.a \
libgkmedias.a \
libnksrtp_s.a \
libmozz.a

LIBS = $(addprefix $(BUILD_DIR)/, $(LIBNAMES))

testapp: $(BUILD_DIR)/testapp.o $(LIBS)
	$(CXX) $< $(LIBS) $(LFLAGS) -o $@

$(BUILD_DIR)/testapp.o: testapp.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(INCLUDE) $^ -c -o $@

$(BUILD_DIR)/%.a : %.a.desc
	@mkdir -p $(BUILD_DIR)
	$(AR) cr $@ `python ./tools/expand.py $<`

clean:
	rm -f $(LIBS) $(BUILD_DIR)/testapp.o

clobber: clean
	rm -f testapp
	rm -rf $(BUILD_DIR)
