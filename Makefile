# --- Compilator and options ---
CC      := gcc
CFLAGS  := -Wall -Wextra -pedantic -std=c99 -O2 -I include
LDFLAGS :=
LDLIBS  :=

# --- Directories ---
SRCDIR  := src
TESTDIR := tests
OBJDIR  := build

# --- Pricipal source file ---
SRC_ERRAID_MAIN := $(SRCDIR)/main.c
SRC_TADMOR_MAIN := $(SRCDIR)/tadmors/tadmor_main.c

# --- all the .c in src ---
SRC_ALL := $(shell find $(SRCDIR) -name "*.c")

# --- communes souces (all exept mains) ---
SRC_COMMON := $(filter-out $(SRC_ERRAID_MAIN) $(SRC_TADMOR_MAIN), $(SRC_ALL))

# --- Objects ---
obj_from_src = $(foreach f,$1,$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(f)))

OBJ_ERRAID := $(call obj_from_src,$(SRC_ERRAID_MAIN) $(SRC_COMMON))
OBJ_TADMOR := $(call obj_from_src,$(SRC_TADMOR_MAIN) $(SRC_COMMON))

# --- Executables ---
TARGETS := erraid tadmor

.PHONY: all clean distclean test_daemon test_client

# --- Default target ---
all: $(TARGETS)

# --- Executables rules ---
erraid: $(OBJ_ERRAID)
	$(CC) $(LDFLAGS) -o ./erraid $^ $(LDLIBS)

tadmor: $(OBJ_TADMOR)
	$(CC) $(LDFLAGS) -o ./tadmor $^ $(LDLIBS)

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
		$(SRCDIR)/types/my_string.c 


# --- Règle générique pour compiler .c → .o ---
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Cleaning ---
clean:
	rm -rf $(OBJDIR)
	rm -f test_daemon test_client
	find $(SRCDIR) -name "*.o" -delete

distclean: clean
	rm -f $(TARGETS)
