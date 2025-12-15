# --- Compilateur et options ---
CC      := gcc
CFLAGS  := -Wall -Wextra -pedantic -std=c99 -O2 -I include
LDFLAGS :=
LDLIBS  :=

# --- Répertoires ---
SRCDIR  := src
TESTDIR := tests
OBJDIR  := build

# --- Fichiers sources principaux ---
SRC_ERRAID_MAIN := $(SRCDIR)/main.c
SRC_TADMOR_MAIN := $(SRCDIR)/tadmor_main.c

# --- Tous les .c dans src (récursif) ---
SRC_ALL := $(shell find $(SRCDIR) -name "*.c")

# --- Sources communes (tout sauf les mains) ---
SRC_COMMON := $(filter-out $(SRC_ERRAID_MAIN) $(SRC_TADMOR_MAIN), $(SRC_ALL))

# --- Objets ---
obj_from_src = $(foreach f,$1,$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(f)))

OBJ_ERRAID := $(call obj_from_src,$(SRC_ERRAID_MAIN) $(SRC_COMMON))
OBJ_TADMOR := $(call obj_from_src,$(SRC_TADMOR_MAIN) $(SRC_COMMON))

# --- Executables ---
TARGETS := erraid tadmor

.PHONY: all clean distclean test_daemon test_client

# --- Cible par défaut ---
all: $(TARGETS)

# --- Règles pour les exécutables ---
erraid: $(OBJ_ERRAID)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tadmor: $(OBJ_TADMOR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# --- Tests (compilation indépendante) ---

test_daemon: $(TESTDIR)/test_daemon.c
	$(CC) $(CFLAGS) -I include -o $@ \
	    $(TESTDIR)/test_daemon.c \
	    $(SRCDIR)/pipes.c \
	    $(SRCDIR)/serialization.c \
	    $(SRCDIR)/communication/communication.c \
	    $(SRCDIR)/communication/answer.c \
	    $(SRCDIR)/communication/request.c \
		$(SRCDIR)/tree-reading/tree-reader.c \
		$(SRCDIR)/types/my_string.c 



test_client: $(TESTDIR)/test_client.c
	$(CC) $(CFLAGS) -I include -o $@ \
	    $(TESTDIR)/test_client.c \
	    $(SRCDIR)/pipes.c \
	    $(SRCDIR)/serialization.c \
	    $(SRCDIR)/communication/communication.c \
	    $(SRCDIR)/communication/answer.c \
	    $(SRCDIR)/communication/request.c \
	    $(SRCDIR)/tree-reading/tree-reader.c \
	    $(SRCDIR)/tree-reading/cmd_reader.c \
	    $(SRCDIR)/tree-reading/times-reader.c \
	    $(SRCDIR)/types/task.c \
	    $(SRCDIR)/types/my_string.c


# --- Règle générique pour compiler .c → .o ---
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Nettoyage ---
clean:
	rm -rf $(OBJDIR)
	rm -f test_daemon test_client

distclean: clean
	rm -f $(TARGETS)
