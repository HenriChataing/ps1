
CC        := gcc
CXX       := g++
LD        := g++

Q ?= @

OBJDIR    := obj
EXE       := ps1

# Enable gcc profiling.
PROFILE   ?= 0

# Select optimisation level.
OPTIMISE  ?= 2

# Enable address sanitizer.
ENABLE_SANITIZER ?= 0

# Enable recompiler.
ENABLE_RECOMPILER ?= 1

# Enable capturing a trace of executed cpu and rsp instructions.
# NB: recompiled instructions are never captured.
ENABLE_TRACE ?= 1

# Enable breakpoints.
ENABLE_BREAKPOINTS ?= 1

INCLUDE   := \
    include \
    src \
    src/gui \
    src/interpreter \
    external/fmt/include \
    external/imgui \
    external/cxxopts/include \
    external/tomlplusplus/include

DEFINE    := \
    ENABLE_RECOMPILER=$(ENABLE_RECOMPILER) \
    ENABLE_TRACE=$(ENABLE_TRACE) \
    ENABLE_BREAKPOINTS=$(ENABLE_BREAKPOINTS) \
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS

CFLAGS    := -Wall -Wno-unused-function -std=gnu11 -g -msse2
CFLAGS    += -O$(OPTIMISE) $(addprefix -I,$(INCLUDE)) $(addprefix -D,$(DEFINE))

CXXFLAGS  := -Wall -Wno-unused-function -std=c++17 -g -msse2
CXXFLAGS  += -O$(OPTIMISE) $(addprefix -I,$(INCLUDE)) $(addprefix -D,$(DEFINE))

LDFLAGS   :=

# Enable address sanitizer.
ifeq ($(ENABLE_SANITIZER),1)
CFLAGS    += -fsanitize=address
CXXFLAGS  += -fsanitize=address
LIBS      += -lasan
endif

# Enable gprof profiling.
# Output file gmon.out can be formatted with make gprof2dot
ifeq ($(PROFILE),1)
CFLAGS    += -pg
CXXFLAGS  += -pg
LDFLAGS   += -pg
endif

# Options for multithreading.
LIBS      += -lpthread

# Additional library dependencies.
LIBS      += -lpng

# Options for linking imgui with opengl3 and glfw3
LIBS      += -lGL -lGLEW `pkg-config --static --libs glfw3`
CFLAGS    += `pkg-config --cflags glfw3`
CXXFLAGS  += `pkg-config --cflags glfw3`

.PHONY: all
all: $(EXE)

UI_OBJS := \
    $(OBJDIR)/src/gui/gui.o \
    $(OBJDIR)/src/gui/graphics.o \
    $(OBJDIR)/src/gui/imgui_impl_glfw.o \
    $(OBJDIR)/src/gui/imgui_impl_opengl3.o

HW_OBJS := \
    $(OBJDIR)/src/psx/state.o \
    $(OBJDIR)/src/psx/memory.o \
    $(OBJDIR)/src/psx/hw.o \
    $(OBJDIR)/src/psx/cdrom.o \
    $(OBJDIR)/src/psx/gpu.o \
    $(OBJDIR)/src/psx/core.o

EXTERNAL_OBJS := \
    $(OBJDIR)/external/fmt/src/format.o \
    $(OBJDIR)/external/imgui/imgui.o \
    $(OBJDIR)/external/imgui/imgui_draw.o \
    $(OBJDIR)/external/imgui/imgui_widgets.o \
    $(OBJDIR)/external/imgui/imgui_tables.o

OBJS      := \
    $(HW_OBJS) \
    $(UI_OBJS) \
    $(EXTERNAL_OBJS) \
    $(OBJDIR)/src/main.o \
    $(OBJDIR)/src/debugger.o \
    $(OBJDIR)/src/interpreter/cpu.o \
    $(OBJDIR)/src/interpreter/cp0.o \
    $(OBJDIR)/src/interpreter/cp2.o \
    $(OBJDIR)/src/assembly/disassembler.o

DEPS      := $(patsubst %.o,%.d,$(OBJS))

-include $(DEPS)

$(OBJDIR)/%.o: %.cc
	@echo "  CXX     $<"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) -c $< -MMD -MF $(@:.o=.d) -o $@

$(OBJDIR)/%.o: %.cpp
	@echo "  CXX     $*.cpp"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) -c $< -MMD -MF $(@:.o=.d) -o $@

$(OBJDIR)/%.o: %.c
	@echo "  CC      $*.c"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -c $< -MMD -MF $(@:.o=.d) -o $@

$(EXE): $(OBJS)
	@echo "  LD      $@"
	$(Q)$(LD) -o $@ $(LDFLAGS) $^ $(LIBS)

.PHONY: gprof2dot
gprof2dot:
	gprof $(EXE) | gprof2dot | dot -Tpng -o n64-prof.png

.PHONY: clean
clean:
	@rm -rf $(OBJDIR) $(EXE)
