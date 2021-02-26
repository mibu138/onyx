CC = gcc
GLC = glslc

CFLAGS = -Wall -Wno-missing-braces -fPIC
LDFLAGS = -L/opt/hfs18.6/dsolib
INFLAGS = -I$(HOME)/dev -I/usr/include/freetype2
LIBS = -lm -lcarbon -lvulkan -lxcb -lxcb-keysyms -lfreetype
LIB  = $(HOME)/lib
GLFLAGS = --target-env=vulkan1.2
BIN = bin
LIBNAME = obsidian

O = build
GLSL = shaders
SPV  = shaders/spv

NAME = viewer


DEPS =  \
	Makefile       \
    d_display.h    \
    v_video.h      \
    v_def.h        \
	v_loader.h     \
    v_memory.h     \
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
    i_input.h      \
    t_utils.h      \
    t_def.h        \
	t_text.h       

SHDEPS = \
	shaders/common.glsl \
	shaders/raycommon.glsl


OBJS =  \
    $(O)/d_display.o    \
    $(O)/v_video.o      \
    $(O)/v_memory.o     \
	$(O)/v_image.o      \
	$(O)/v_command.o    \
	$(O)/v_loader.o     \
    $(O)/r_render.o     \
    $(O)/r_pipeline.o   \
	$(O)/r_renderpass.o \
	$(O)/r_raytrace.o   \
	$(O)/r_geo.o        \
	$(O)/s_scene.o      \
	$(O)/f_file.o       \
	$(O)/u_ui.o         \
    $(O)/i_input.o      \
    $(O)/t_utils.o      \
	$(O)/t_text.o


debug: CFLAGS += -g -DVERBOSE=1
debug: all

nbp: CFLAGS += -DNO_BEST_PRACTICE
nbp: debug

release: CFLAGS += -DNDEBUG -O2 
release: all

all: lib tags shaders

FRAGS := $(patsubst %.frag,$(SPV)/%-frag.spv,$(notdir $(wildcard $(GLSL)/*.frag)))
VERTS := $(patsubst %.vert,$(SPV)/%-vert.spv,$(notdir $(wildcard $(GLSL)/*.vert)))

shaders: $(FRAGS) $(VERTS)

clean: 
	rm -f $(O)/* $(LIB)/$(LIBNAME) $(BIN)/*

tags:
	ctags -R .

bin: main.c $(OBJS) $(DEPS) shaders
	$(CC) $(CFLAGS) $(INFLAGS) $(LDFLAGS) $(OBJS) $< -o $(BIN)/$(NAME) $(LIBS)

lib: $(OBJS) $(DEPS) shaders
	$(CC) -shared -o $(LIB)/lib$(LIBNAME).so $(OBJS)

staticlib: $(OBJS) $(DEPS) shaders
	ar rcs $(LIB)/lib$(NAME).a $(OBJS)

$(O)/%.o:  %.c $(DEPS)
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
