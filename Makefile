ifeq ($(OS), Windows_NT)
	OS = WIN
else 
	OS = UNIX
endif

CC = gcc
GLC = glslc

CFLAGS = -Wall -Wno-missing-braces -fPIC
LIBS = -lm -lcoal -lhell -lfreetype
ifeq ($(OS), WIN)
	OS_HEADERS = $(WIN_HEADERS)
	LIBEXT = dll
	LIBS += -lvulkan-1
	HOMEDIR = C:
	INEXTRA = -IC:\VulkanSDK\1.2.170.0\Include -IC:\msys64\mingw64\include\freetype2
	LDFLAGS = -L$(HOMEDIR)/lib -LC:\VulkanSDK\1.2.170.0\Lib
else
	OS_HEADERS = $(UNIX_HEADERS)
	LIBEXT = so
	LIBS += -lvulkan
	HOMEDIR =  $(HOME)
	INEXTRA = -I/usr/include/freetype2 
	LDFLAGS = -L$(HOMEDIR)/lib -L/opt/hfs18.6/dsolib
endif
LIBDIR  = $(HOMEDIR)/lib
DEV = $(HOMEDIR)/dev
INFLAGS = -I$(DEV) $(INEXTRA)
GLFLAGS = --target-env=vulkan1.2
BIN = bin
LIBNAME = obsidian
LIBPATH = $(LIBDIR)/lib$(LIBNAME).$(LIBEXT)

O = $(PWD)/build
GLSL = shaders
SPV  = shaders/spv

NAME = viewer


DEPS =  \
	Makefile       \
    v_video.h      \
    v_def.h        \
	v_loader.h     \
    v_memory.h     \
    v_swapchain.h  \
	v_image.h      \
	v_command.h    \
    r_render.h     \
    r_pipeline.h   \
	r_renderpass.h \
	r_raytrace.h   \
	r_geo.h        \
	s_scene.h      \
	f_file.h       \
	u_ui.h         \
    t_def.h        \
	t_text.h       \
	locations.h

SHDEPS = \
	shaders/common.glsl \
	shaders/raycommon.glsl


OBJS =  \
    $(O)/v_video.o      \
    $(O)/v_memory.o     \
	$(O)/v_image.o      \
	$(O)/v_command.o    \
	$(O)/v_loader.o     \
    $(O)/v_swapchain.o  \
    $(O)/r_render.o     \
    $(O)/r_pipeline.o   \
	$(O)/r_renderpass.o \
	$(O)/r_raytrace.o   \
	$(O)/r_geo.o        \
	$(O)/s_scene.o      \
	$(O)/f_file.o       \
	$(O)/u_ui.o         \
	$(O)/t_text.o       \
	$(O)/locations.o    


debug: CFLAGS += -g -DVERBOSE=1
debug: all

nbp: CFLAGS += -DNO_BEST_PRACTICE
nbp: debug

release: CFLAGS += -DNDEBUG -O2 
release: all

all: lib shaders

FRAGS := $(patsubst %.frag,$(SPV)/%-frag.spv,$(notdir $(wildcard $(GLSL)/*.frag)))
VERTS := $(patsubst %.vert,$(SPV)/%-vert.spv,$(notdir $(wildcard $(GLSL)/*.vert)))

.PHONY: hell clean
hell:
	make -C $(DEV)/hell

shaders: $(FRAGS) $(VERTS)

clean: 
	rm -f $(O)/* $(LIBPATH) $(BIN)/*

tags:
	ctags -R .

bin: main.c $(OBJS) $(DEPS) shaders
	$(CC) $(CFLAGS) $(INFLAGS) $(LDFLAGS) $(OBJS) $< -o $(BIN)/$(NAME) $(LIBS)

lib: $(OBJS) $(DEPS) shaders
	$(CC) $(LDFLAGS) -shared -o $(LIBPATH) $(OBJS) $(LIBS)

$(O)/%.o:  $(PWD)/%.c $(DEPS)
	$(CC) $(CFLAGS) $(INFLAGS) -c $< -o $@

$(SPV)/%-vert.spv: $(GLSL)/%.vert $(SHDEPS)
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-frag.spv: $(GLSL)/%.frag $(SHDEPS)
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rchit.spv: $(GLSL)/%.rchit $(SHDEPS)
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rgen.spv: $(GLSL)/%.rgen $(SHDEPS)
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rmiss.spv: $(GLSL)/%.rmiss $(SHDEPS)
	$(GLC) $(GLFLAGS) $< -o $@
