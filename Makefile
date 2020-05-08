CC := c++
CPP := cpp

OPENGLLIB := -lopengl32
GDILIB := -lgdi32
GLFWLIB := -lglfw3
FREETYPELIB := -lfreetype
LDLIBS := $(OPENGLLIB) $(GLFWLIB) $(GDILIB) $(FREETYPELIB)
LDFLAGS := -L.\lib -L"C:/FREETYPE/build"
CPPFLAGS := -I.\include -I"C:/FREETYPE/freetype-2.9/include" -I"C:/FREETYPE/freetype-2.9/include/freetype2"

EXE_NAME := tracer.exe
BIN_DIR := bin
EXE := $(BIN_DIR)\$(EXE_NAME)
SRC_DIR := src
OBJ_DIR := obj

SRC := $(wildcard $(SRC_DIR)/*.cpp)
SRCC := $(wildcard $(SRC_DIR)/*.c)

OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o) $(SRCC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

DEP := $(OBJ:$(OBJ_DIR)/%.o=$(OBJ_DIR)/%.d)

.PHONY: all clean cleandep

all: $(EXE)

-include $(DEP)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@$(CPP) $(CPPFLAGS) $< -MM -MT $(@:.d=.o) >$@

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $@

clean:
	if exist .\$(OBJ_DIR) rmdir .\$(OBJ_DIR) /q /s
	if exist .\$(EXE) del .\$(EXE) /q /s

cleandep:
	del /q /s $(subst /,\,$(DEP))