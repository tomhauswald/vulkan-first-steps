.SUFFIXES:
	
CXX       := clang++-10
CXX_STD   := c++20
CXX_FLAGS := \
	-std=$(CXX_STD) \
	-MMD -MP \
	-Wall -Wextra -Wpedantic -Werror \
	-Wno-unused-parameter \
	-Wno-unused-private-field \
	-Wno-c99-extensions \
	-Wno-deprecated-enum-enum-conversion \
	`libpng-config --cflags` \
	-I/usr/local/include/png++

ifdef RELEASE
CXX_FLAGS += -O3 -DNDEBUG
else
CXX_FLAGS += -g -O0 -D_DEBUG
endif

LD        := clang++-10
LD_FLAGS  := -std=$(CXX_STD) `libpng-config --ldflags` `pkg-config --static --libs glfw3` -lvulkan 

BINARY  := ./bin/$(if $(RELEASE),release,debug).elf
SOURCES := $(wildcard ../source/**/*.cc ../source/*.cc)
OBJDIR  := ./obj/$(if $(RELEASE),release,debug)
OBJECTS := $(patsubst ../source/%.cc,$(OBJDIR)/%.o,$(SOURCES))

-include $(OBJECTS:.o=.d)

$(BINARY): $(OBJECTS)
	mkdir -p $(dir $(@))
	$(LD) -o $(@) $(OBJECTS) $(LD_FLAGS)

$(OBJDIR)/%.o: ../source/%.cc
	mkdir -p $(dir $(@))
	$(CXX) $(CXX_FLAGS) -o $(@) -c $(<)

all : $(BINARY)

run : $(BINARY)
	$(BINARY)
clean:
	rm -rf ./bin
	rm -rf ./obj
