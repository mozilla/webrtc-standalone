RENDERNAME=renderGL

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
-DMOZILLA_MEDIA_STANDALONE \
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
-lcrmf \
-lnss3 \
-lnspr4 \
-lssl3 \
-lnssutil3 \
-framework QTKit \
-framework QuartzCore \
-framework Security \
-framework SystemConfiguration \
-lmozalloc

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

