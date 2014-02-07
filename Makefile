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

LIBNAMES = \
libxpcomglue_s.a \
libmtransport_s.a \
libecc.a \
libsipcc.a \
libwebrtc.a \
libgkmedias.a \
libnksrtp_s.a \
libmozz.a


# /home/randall/src/gecko-desktop/obj-x86_64-unknown-linux-gnu/dist/lib/libmozglue.a \
# /home/randall/src/gecko-desktop/obj-x86_64-unknown-linux-gnu/dist/lib/libmemory.a \

LIBS = $(addprefix $(BUILD_DIR)/, $(LIBNAMES))

testapp: $(BUILD_DIR)/testapp.o $(LIBS)
	$(CXX) $< $(LIBS) $(LIBS) $(LFLAGS) -o $@

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
