CC = g++
CFLAGS = -c -Wall -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual\
-Wchar-subscripts -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat=2 -Winline\
-Wnon-virtual-dtor -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo\
-Wstrict-overflow=2 -Wsuggest-override -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused -Wvariadic-macros\
-Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -Wsuggest-override\
-Wlong-long -fopenmp -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -fno-omit-frame-pointer\
-Wlarger-than=8192 -Wstack-protector -fPIE -Werror=vla -MP -MMD
INC_DIR = ./includes
SRC_DIR = ./source
OBJ_DIR = ./build
CFLAGS += -I $(INC_DIR)
LDFLAGS =
SOURCES = $(addprefix $(SRC_DIR)/, tester_main.cpp stack.cpp protection.cpp)
OBJECTS = $(addprefix $(OBJ_DIR)/, $(notdir $(SOURCES:.cpp=.o)) )
DEPS    = $(addsuffix .d, $(basename $(OBJECTS)))
EXECUTABLE = stk.exe

all: debug

-include $(DEPS)

debug: CFLAGS += -D DEBUG
debug: $(EXECUTABLE)

release: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) $< -o $@

lib_debug: CFLAGS += -D DEBUG
lib_debug: $(OBJ_DIR)/stack.o
	ar rcs $(INC_DIR)/stack_debug.a $<

lib_release: $(OBJ_DIR)/stack.o
	ar rcs $(INC_DIR)/stack_release.a $<


clean:
	del /f /s *.o *.d
