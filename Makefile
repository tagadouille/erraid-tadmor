# --- Compilateur et options ---
CC      := gcc
CFLAGS  := -Wall -Wextra -pedantic -std=c99 -O2 -I include
LDFLAGS :=
LDLIBS  :=

# --- Répertoires ---
SRCDIR  := src
OBJDIR  := build

# --- Fichiers sources principaux ---
SRC_ERRAID_MAIN := $(SRCDIR)/main.c
SRC_TADMOR_MAIN := $(SRCDIR)/tadmor_main.c

# --- Tous les .c dans src (récursif) ---
SRC_ALL := $(shell find $(SRCDIR) -name "*.c")

# --- Sources communes (tout sauf les mains) ---
SRC_COMMON := $(filter-out $(SRC_ERRAID_MAIN) $(SRC_TADMOR_MAIN), $(SRC_ALL))

# --- Objets ---
obj_from_src = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$1)

OBJ_ERRAID := $(call obj_from_src,$(SRC_ERRAID_MAIN) $(SRC_COMMON))
OBJ_TADMOR := $(call obj_from_src,$(SRC_TADMOR_MAIN) $(SRC_COMMON))

# --- Executables ---
TARGETS := erraid tadmor

.PHONY: all clean distclean

# --- Cible par défaut ---
all: $(TARGETS)

# --- Règles pour les exécutables ---
erraid: $(OBJ_ERRAID)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tadmor: $(OBJ_TADMOR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# --- Règle générique pour compiler .c → .o ---
# Les headers sont utilisés via leurs chemins relatifs
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Nettoyage ---
clean:
	rm -rf $(OBJDIR)

distclean: clean
	rm -f $(TARGETS)
