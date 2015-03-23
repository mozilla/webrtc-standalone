RENDERNAME=renderGL


CFLAGS = \
-fvisibility=hidden \
-DCHROMIUM_BUILD \
-DUSE_LIBJPEG_TURBO='1' \
-DENABLE_ONE_CLICK_SIGNIN \
-DENABLE_REMOTING='1' \
-DENABLE_WEBRTC='1' \
-DENABLE_CONFIGURATION_POLICY \
-DENABLE_INPUT_SPEECH \
-DENABLE_NOTIFICATIONS \
-DENABLE_HIDPI='1' \
-DENABLE_GPU='1' \
-DENABLE_EGLIMAGE='1' \
-DUSE_SKIA='1' \
-DENABLE_TASK_MANAGER='1' \
-DENABLE_WEB_INTENTS='1' \
-DENABLE_EXTENSIONS='1' \
-DENABLE_PLUGIN_INSTALLATION='1' \
-DENABLE_PROTECTOR_SERVICE='1' \
-DENABLE_SESSION_SERVICE='1' \
-DENABLE_THEMES='1' \
-DENABLE_BACKGROUND='1' \
-DENABLE_AUTOMATION='1' \
-DENABLE_PRINTING='1' \
-DENABLE_CAPTIVE_PORTAL_DETECTION='1' \
-DLOG4CXX_STATIC \
-D_NO_LOG4CXX \
-DUSE_SSLEAY \
-D_CPR_USE_EXTERNAL_LOGGER \
-DWEBRTC_RELATIVE_PATH \
-DHAVE_WEBRTC_VIDEO \
-DHAVE_WEBRTC_VOICE \
-DHAVE_STDINT_H='1' \
-DHAVE_STDLIB_H='1' \
-DHAVE_UINT8_T='1' \
-DHAVE_UINT16_T='1' \
-DHAVE_UINT32_T='1' \
-DHAVE_UINT64_T='1' \
-DMOZILLA_INTERNAL_API \
-DMOZILLA_XPCOMRT_API \
-DNO_CHROMIUM_LOGGING \
-DUSE_FAKE_MEDIA_STREAMS \
-DUSE_FAKE_PCOBSERVER \
-DOS_MACOSX \
-DSIP_OS_OSX \
-DOSX \
-D_FORTIFY_SOURCE='2' \
-D__STDC_FORMAT_MACROS \
-DDYNAMIC_ANNOTATIONS_ENABLED='1' \
-DWTF_USE_DYNAMIC_ANNOTATIONS='1' \
-DAB_CD=en-US \
-DNO_NSPR_10_SUPPORT \
-fPIC \
-Qunused-arguments \
-DMOZILLA_CLIENT \
-MD \
-MP \
-Qunused-arguments \
-Qunused-arguments \
-Wall \
-Wempty-body \
-Woverloaded-virtual \
-Wsign-compare \
-Wwrite-strings \
-Wno-invalid-offsetof \
-Wno-c++0x-extensions \
-Wno-extended-offsetof \
-Wno-unknown-warning-option \
-Wno-return-type-c-linkage \
-Wno-inline-new-delete \
-fno-exceptions \
-fno-strict-aliasing \
-fno-rtti \
-fno-exceptions \
-fno-math-errno \
-std=gnu++0x \
-pthread \
-DNO_X11 \
-pipe \
 -DDEBUG \
-DTRACING \
-g \
-fno-omit-frame-pointer

XXXXX_CFLAGS = \
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
-DMOZILLA_INTERNAL_API \
-DMOZILLA_XPCOMRT_API \
-fPIC \
-Qunused-arguments \
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
-Wno-inline-new-delete \
-fno-exceptions \
-fno-strict-aliasing \
-fno-rtti \
-fno-exceptions \
-fno-math-errno \
-std=gnu++0x \
-pipe \
-g \
-fomit-frame-pointer

LFLAGS = \
-L$(GECKO_OBJ)/dist/sdk/lib \
-L$(GECKO_OBJ)/dist/lib \
-pipe \
-framework Cocoa \
-lobjc \
-framework ExceptionHandling \
-Wl,-executable_path,$(GECKO_OBJ)/dist/bin \
-Wl,-rpath,$(GECKO_OBJ)/dist/bin \
$(GECKO_OBJ)/dist/lib/libssl.a \
$(GECKO_OBJ)/dist/lib/libplc4.a \
$(GECKO_OBJ)/dist/lib/libnspr4.a \
$(GECKO_OBJ)/dist/lib/libcertdb.a \
$(GECKO_OBJ)/dist/lib/libcerthi.a \
$(GECKO_OBJ)/dist/lib/libcryptohi.a \
$(GECKO_OBJ)/dist/lib/libpkixpki.a \
$(GECKO_OBJ)/dist/lib/libpkixtop.a \
$(GECKO_OBJ)/dist/lib/libpkixchecker.a \
$(GECKO_OBJ)/dist/lib/libpkixstore.a \
$(GECKO_OBJ)/dist/lib/libpkixcrlsel.a \
$(GECKO_OBJ)/dist/lib/libpkixpki.a \
$(GECKO_OBJ)/dist/lib/libpkixresults.a \
$(GECKO_OBJ)/dist/lib/libpkixutil.a \
$(GECKO_OBJ)/dist/lib/libpkixparams.a \
$(GECKO_OBJ)/dist/lib/libpkixcertsel.a \
$(GECKO_OBJ)/dist/lib/libpkixsystem.a \
$(GECKO_OBJ)/dist/lib/libpkixchecker.a \
$(GECKO_OBJ)/dist/lib/libpkixmodule.a \
$(GECKO_OBJ)/dist/lib/libnsspki.a \
$(GECKO_OBJ)/dist/lib/libnssdev.a \
$(GECKO_OBJ)/dist/lib/libnssb.a \
$(GECKO_OBJ)/dist/lib/libnss.a \
$(GECKO_OBJ)/dist/lib/libcerthi.a \
$(GECKO_OBJ)/dist/lib/libsmime.a \
$(GECKO_OBJ)/dist/lib/libnssutil.a \
$(GECKO_OBJ)/dist/lib/libplds4.a \
$(GECKO_OBJ)/dist/lib/libplc4.a \
$(GECKO_OBJ)/dist/lib/libsoftokn.a \
-lcrmf \
-framework QTKit \
-framework QuartzCore \
-framework Security \
-framework SystemConfiguration \
-lmozglue

CFLAGS+=-I$(SDL_INCLUDE_PATH) -D_THREAD_SAFE -DDARWIN_GL
LFLAGS+=-L$(SDL_LIB_PATH) \
-lSDL2 \
-lSDL2main

LFLAGS+=-lm \
-liconv \
-framework OpenGL \
-framework ForceFeedback \
-lobjc \
-framework Cocoa \
-framework Carbon \
-framework IOKit \
-framework CoreAudio \
-framework AudioToolbox \
-framework AudioUnit

