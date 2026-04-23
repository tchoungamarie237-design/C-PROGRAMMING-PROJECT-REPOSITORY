# =============================================================================
#  WIGMS — Wedding & Invitation Guest Management System
#  Makefile for MSYS2 / MinGW-w64 (UCRT64 terminal) on Windows
#
#  Usage (from the UCRT64 terminal, in the folder containing this Makefile):
#
#    make              — build everything (console modules + all GUI apps)
#    make all          — same as above
#
#    make person       — build only the console Person module
#    make category     — build only the console Category module
#    make person_gtk   — build only the GTK4 Person GUI
#    make category_gtk — build only the GTK4 Category GUI
#    make wigms_home   — build only the GTK4 Home launcher
#    make gui          — build all three GTK4 GUI apps
#    make console      — build all console modules
#
#    make run_person       — build + run the console Person module
#    make run_category     — build + run the console Category module
#    make run_person_gtk   — build + run the GTK4 Person GUI
#    make run_category_gtk — build + run the GTK4 Category GUI
#    make run_wigms_home   — build + run the GTK4 Home launcher
#
#    make clean        — remove all compiled .exe and .o files
#    make help         — show this usage summary
#
# =============================================================================

# ── Compiler & flags ──────────────────────────────────────────────────────────
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11

# GTK4 flags retrieved automatically from pkg-config
GTK_CFLAGS  = $(shell pkg-config --cflags gtk4)
GTK_LIBS    = $(shell pkg-config --libs   gtk4)

# ── Source files ──────────────────────────────────────────────────────────────

# Console module: Person (standalone, has its own main)
PERSON_SRC   = Person_.c

# Console module: Category (modular — Category.c depends on Category.h + Type.h)
CATEGORY_SRC = Category.c
CATEGORY_HDR = Category.h Type.h

# GTK4 GUI sources
PERSON_GTK_SRC   = person_gtk.c
CATEGORY_GTK_SRC = category_gtk.c
WIGMS_HOME_SRC   = wigms_home.c

# ── Output executables ────────────────────────────────────────────────────────
PERSON_EXE       = person.exe
CATEGORY_EXE     = category.exe
PERSON_GTK_EXE   = person_gtk.exe
CATEGORY_GTK_EXE = category_gtk.exe
WIGMS_HOME_EXE   = wigms_home.exe

# ── Object files (for modular Category build) ─────────────────────────────────
CATEGORY_OBJ = Category.o

# =============================================================================
#  DEFAULT TARGET — build everything
# =============================================================================
.PHONY: all
all: console gui
	@echo.
	@echo ============================================================
	@echo  All targets built successfully.
	@echo  Console : $(PERSON_EXE)  $(CATEGORY_EXE)
	@echo  GUI     : $(PERSON_GTK_EXE)  $(CATEGORY_GTK_EXE)  $(WIGMS_HOME_EXE)
	@echo ============================================================

# =============================================================================
#  CONSOLE MODULE TARGETS
# =============================================================================
.PHONY: console
console: person category

# ── Person console module ─────────────────────────────────────────────────────
.PHONY: person
person: $(PERSON_EXE)

$(PERSON_EXE): $(PERSON_SRC)
	@echo [BUILD] Compiling Person console module...
	$(CC) $(CFLAGS) -o $(PERSON_EXE) $(PERSON_SRC)
	@echo [OK]    $(PERSON_EXE) created.

# ── Category console module (modular: .c -> .o -> .exe) ──────────────────────
.PHONY: category
category: $(CATEGORY_EXE)

$(CATEGORY_EXE): $(CATEGORY_OBJ)
	@echo [LINK]  Linking Category console module...
	$(CC) $(CFLAGS) -o $(CATEGORY_EXE) $(CATEGORY_OBJ)
	@echo [OK]    $(CATEGORY_EXE) created.

$(CATEGORY_OBJ): $(CATEGORY_SRC) $(CATEGORY_HDR)
	@echo [CC]    Compiling $(CATEGORY_SRC)...
	$(CC) $(CFLAGS) -c -o $(CATEGORY_OBJ) $(CATEGORY_SRC)

# =============================================================================
#  GTK4 GUI TARGETS
# =============================================================================
.PHONY: gui
gui: person_gtk category_gtk wigms_home

# ── GTK4 Person GUI ───────────────────────────────────────────────────────────
.PHONY: person_gtk
person_gtk: $(PERSON_GTK_EXE)

$(PERSON_GTK_EXE): $(PERSON_GTK_SRC)
	@echo [BUILD] Compiling GTK4 Person GUI...
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $(PERSON_GTK_EXE) $(PERSON_GTK_SRC) $(GTK_LIBS)
	@echo [OK]    $(PERSON_GTK_EXE) created.

# ── GTK4 Category GUI ─────────────────────────────────────────────────────────
.PHONY: category_gtk
category_gtk: $(CATEGORY_GTK_EXE)

$(CATEGORY_GTK_EXE): $(CATEGORY_GTK_SRC)
	@echo [BUILD] Compiling GTK4 Category GUI...
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $(CATEGORY_GTK_EXE) $(CATEGORY_GTK_SRC) $(GTK_LIBS)
	@echo [OK]    $(CATEGORY_GTK_EXE) created.

# ── GTK4 Home Launcher ────────────────────────────────────────────────────────
.PHONY: wigms_home
wigms_home: $(WIGMS_HOME_EXE)

$(WIGMS_HOME_EXE): $(WIGMS_HOME_SRC)
	@echo [BUILD] Compiling GTK4 Home Launcher...
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $(WIGMS_HOME_EXE) $(WIGMS_HOME_SRC) $(GTK_LIBS)
	@echo [OK]    $(WIGMS_HOME_EXE) created.

# =============================================================================
#  RUN TARGETS
# =============================================================================
.PHONY: run_person
run_person: $(PERSON_EXE)
	@echo [RUN]   Launching Person console module...
	./$(PERSON_EXE)

.PHONY: run_category
run_category: $(CATEGORY_EXE)
	@echo [RUN]   Launching Category console module...
	./$(CATEGORY_EXE)

.PHONY: run_person_gtk
run_person_gtk: $(PERSON_GTK_EXE)
	@echo [RUN]   Launching GTK4 Person GUI...
	./$(PERSON_GTK_EXE)

.PHONY: run_category_gtk
run_category_gtk: $(CATEGORY_GTK_EXE)
	@echo [RUN]   Launching GTK4 Category GUI...
	./$(CATEGORY_GTK_EXE)

.PHONY: run_wigms_home
run_wigms_home: $(WIGMS_HOME_EXE)
	@echo [RUN]   Launching WIGMS Home...
	./$(WIGMS_HOME_EXE)

# =============================================================================
#  CLEAN
# =============================================================================
.PHONY: clean
clean:
	@echo [CLEAN] Removing compiled files...
	-del /Q $(PERSON_EXE) $(CATEGORY_EXE) $(CATEGORY_OBJ) \
	         $(PERSON_GTK_EXE) $(CATEGORY_GTK_EXE) $(WIGMS_HOME_EXE) 2>NUL
	@echo [OK]    Clean complete.

# =============================================================================
#  HELP
# =============================================================================
.PHONY: help
help:
	@echo.
	@echo  WIGMS Makefile — Available targets:
	@echo  -------------------------------------------------------
	@echo  make                  Build everything
	@echo  make console          Build console modules only
	@echo  make gui              Build all GTK4 GUI apps only
	@echo  make person           Build Person console module
	@echo  make category         Build Category console module
	@echo  make person_gtk       Build GTK4 Person GUI
	@echo  make category_gtk     Build GTK4 Category GUI
	@echo  make wigms_home       Build GTK4 Home launcher
	@echo  make run_person       Build + run Person console
	@echo  make run_category     Build + run Category console
	@echo  make run_person_gtk   Build + run GTK4 Person GUI
	@echo  make run_category_gtk Build + run GTK4 Category GUI
	@echo  make run_wigms_home   Build + run GTK4 Home launcher
	@echo  make clean            Remove all .exe and .o files
	@echo  make help             Show this message
	@echo  -------------------------------------------------------
	@echo  NOTE: Run from the MSYS2 UCRT64 terminal.
	@echo  All source files must be in the same folder as this Makefile.
	@echo.