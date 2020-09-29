CC = gcc
GLC = glslc

CFLAGS = -Wall -Wno-missing-braces -Wno-attributes -fPIC
LDFLAGS = -L/opt/hfs18.0/dsolib
INFLAGS = -I/opt/hfs18.0/toolkit/include/HAPI
LIBS = -lm -lvulkan -lxcb -lxcb-keysyms -lHAPIL
LIB  = /home/michaelb/lib
GLFLAGS = --target-env=vulkan1.2
BIN = bin

O = build
GLSL = shaders
SPV  = shaders/spv

NAME = viewer


DEPS =  \
    d_display.h  \
    v_video.h    \
    v_def.h      \
    v_memory.h   \
    r_render.h   \
    r_commands.h \
    r_pipeline.h \
	r_raytrace.h \
	r_geo.h      \
    i_input.h    \
	g_game.h     \
    m_math.h     \
    utils.h      \
    def.h        \
	t_viewer.h  


OBJS =  \
    $(O)/d_display.o  \
    $(O)/v_video.o    \
	$(O)/v_def.o      \
    $(O)/v_memory.o   \
    $(O)/r_render.o   \
    $(O)/r_commands.o \
    $(O)/r_pipeline.o \
	$(O)/r_raytrace.o \
	$(O)/r_geo.o      \
    $(O)/i_input.o    \
	$(O)/g_game.o     \
    $(O)/m_math.o     \
    $(O)/utils.o      \
	$(O)/t_viewer.o


SHADERS =                         \
		$(SPV)/default-vert.spv    \
		$(SPV)/default-frag.spv    \
		$(SPV)/raytrace-rchit.spv  \
		$(SPV)/raytrace-rgen.spv   \
        $(SPV)/raytrace-rmiss.spv  \
		$(SPV)/raytraceShadow-rmiss.spv \
		$(SPV)/post-frag.spv       \
		$(SPV)/post-vert.spv

debug: CFLAGS += -g -DVERBOSE=1
debug: all

release: CFLAGS += -DNDEBUG -O2
release: all

all: bin lib tags

shaders: $(SHADERS)

clean: 
	rm -f $(O)/* $(LIB)/* $(BIN)/*

tags:
	ctags -R .

bin: main.c $(OBJS) $(DEPS) shaders
	$(CC) $(CFLAGS) $(INFLAGS) $(LDFLAGS) $(OBJS) $< -o $(BIN)/$(NAME) $(LIBS)

lib: $(OBJS) $(DEPS) shaders
	$(CC) -shared -o $(LIB)/lib$(NAME).so $(OBJS)

staticlib: $(OBJS) $(DEPS) shaders
	ar rcs $(LIB)/lib$(NAME).a $(OBJS)

$(O)/%.o:  %.c $(DEPS)
	$(CC) $(CFLAGS) $(INFLAGS) -c $< -o $@

$(SPV)/%-vert.spv: $(GLSL)/%.vert
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-frag.spv: $(GLSL)/%.frag
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rchit.spv: $(GLSL)/%.rchit
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rgen.spv: $(GLSL)/%.rgen
	$(GLC) $(GLFLAGS) $< -o $@

$(SPV)/%-rmiss.spv: $(GLSL)/%.rmiss
	$(GLC) $(GLFLAGS) $< -o $@
