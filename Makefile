# -------------------------
# Project configuration
# -------------------------
SRC_DIR        := src
OBJ_DIR        := objs
INC_DIR        := include
# List C sources
SRC            := $(SRC_DIR)/malloc.c \
                  $(SRC_DIR)/zone.c \
                  $(SRC_DIR)/block.c \
                  $(SRC_DIR)/fit.c \
                  $(SRC_DIR)/utils.c \
                  $(SRC_DIR)/show.c

# -------------------------
# Test harness
# -------------------------
TEST_SRC       := tests/main.c
TEST_BIN       := test_runner

# -------------------------
# Toolchain & platform
# -------------------------
OS             := $(shell uname -s)
ARCH           := $(shell uname -m)
HOSTTYPE      ?= $(ARCH)_$(OS)

CC             ?= cc
RM             := rm -rf
LN             := ln -sf
MKDIR_P        := mkdir -p

# -------------------------
# Output names
# -------------------------
NAME           := libft_malloc_$(HOSTTYPE).so
LINK_NAME      := libft_malloc.so

# -------------------------
# Flags
# -------------------------
CFLAGS         := -Wall -Wextra -Werror -pedantic -O3 -fPIC -MMD -MP -I$(INC_DIR) -g -fsanitize=leak
LDFLAGS        :=
LDLIBS         :=
LDLINK         := LD_PRELOAD=${LINK_NAME}

# -------------------------
# Tests
# -------------------------
TEST_LDFLAGS = -Wl,--no-as-needed -Wl,--as-needed

# OS-specific settings for shared libraries
ifeq ($(OS),Darwin)
  # macOS uses -dynamiclib; keep .so filename as requested
  SHARED_FLAG  := -dynamiclib
  # (Optional) install_name for @rpath loading:
  # LDFLAGS     += -Wl,-install_name,@rpath/$(LINK_NAME)
else
  SHARED_FLAG  := -shared
  CFLAGS     += -D LINUX
endif

# Optional toggles:
#   make DEBUG=1     -> adds -g and disables -O3 unless SAN=1
ifeq ($(DEBUG),1)
  CFLAGS += -g
endif

# Add logger when LOGGING=1
ifeq ($(LOGGING),1)
  CFLAGS += -D MALLOC_LOGGING=1
  SRC += $(SRC_DIR)/mem_logger.c
endif

ifeq ($(SHOW_MORE),1)
  CFLAGS += -D SHOW_MORE=1
endif


ifeq ($(USE_MALLOC_LOCK),1)
  CFLAGS += -D USE_MALLOC_LOCK=1
endif

# -------------------------
# Derived variables (MUST be after SRC modifications)
# -------------------------
OBJ            := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEP            := $(OBJ:.o=.d)

ifeq ($(LIB_G),1)
	LDLINK         =
else
	CFLAGS += -D FT_MODE
	TEST_LDFLAGS += -L. -lft_malloc
endif

# -------------------------
# Default target
# -------------------------
.PHONY: all
all: $(NAME)

.PHONY: help
help:
	@printf "\033[1;34m===========================================\033[0m\n"
	@printf "\033[1;34m  FT_MALLOC - Available Make Commands\033[0m\n"
	@printf "\033[1;34m===========================================\033[0m\n"
	@printf "\n\033[1;33mBuild Commands:\033[0m\n"
	@printf "  \033[0;32mmake\033[0m or \033[0;32mmake all\033[0m    - Build the malloc library\n"
	@printf "  \033[0;32mmake clean\033[0m          - Remove object files\n"
	@printf "  \033[0;32mmake fclean\033[0m         - Remove all generated files\n"
	@printf "  \033[0;32mmake re\033[0m             - Rebuild everything from scratch\n"
	@printf "\n\033[1;33mTest Commands:\033[0m\n"
	@printf "  \033[0;32mmake build-tests\033[0m    - Build all test executables\n"
	@printf "  \033[0;32mmake run-tests\033[0m      - Build and run all tests sequentially\n"
	@printf "  \033[0;32mmake test\033[0m           - Build and run basic test suite\n"
	@printf "  \033[0;32mmake test-comprehensive\033[0m - Build and run comprehensive malloc tests\n"
	@printf "  \033[0;32mmake test-inplace\033[0m   - Build and run in-place realloc optimization test\n"
	@printf "  \033[0;32mmake test-logger\033[0m    - Build and run memory logger test (generates malloc_log.json)\n"
	@printf "  \033[0;32mmake test-all\033[0m       - Alias for run-tests\n"
	@printf "\n\033[1;33mDebug Commands:\033[0m\n"
	@printf "  \033[0;32mmake check_malloc_lib\033[0m - Verify malloc symbols in library\n"
	@printf "  \033[0;32mmake show\033[0m           - Display build configuration\n"
	@printf "\n\033[1;33mEnvironment:\033[0m\n"
	@printf "  HOSTTYPE=$(HOSTTYPE)\n"
	@printf "  Library: $(NAME)\n"
	@printf "\n\033[1;33mUsage Examples:\033[0m\n"
	@printf "  make && make test-all     - Build and run all tests\n"
	@printf "  make DEBUG=1              - Build with debug symbols\n"
	@printf "  make LIB_G=1              - Build with glibc malloc\n"
	@printf "  make SHOW_MORE=1          - To show detailed info and exact user size\n"
	@printf "  make LOGGING=1            - Build with logging enabled\n"
	@printf "  make USE_MALLOC_LOCK=1    - Build with malloc lock enabled\n"
	@printf "\033[1;34m===========================================\033[0m\n"

# -------------------------
# Build rules
# -------------------------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	@printf "\033[0;33mLinking $@\033[0m ...\n"
	$(CC) $(SHARED_FLAG) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)
	@printf "\033[0;32mDONE: [$@]\033[0m\n"
	@printf "\033[0;33mCreating symbolic link -> $(LINK_NAME)\033[0m\n"
	@$(LN) $(NAME) $(LINK_NAME)
	@printf "\033[0;32mDONE: [$(LINK_NAME)]\033[0m\n"
	$(MAKE) circle.svg

# -------------------------
# Test executables
# -------------------------
COMPREHENSIVE_TEST := comprehensive_test
INPLACE_TEST       := test_inplace_realloc
LOGGER_TEST        := test_logger
ALL_TESTS          := $(TEST_BIN) $(COMPREHENSIVE_TEST) $(INPLACE_TEST) $(LOGGER_TEST)

# -------------------------
# Build rules for test executables
# -------------------------
$(TEST_BIN): $(TEST_SRC) $(NAME)
	@printf "\033[0;33mBuilding $@\033[0m ...\n"
	$(CC) -o $@  $(CPPFLAGS) $(CFLAGS) $< $(TEST_LDFLAGS)
	@printf "\033[0;32mDONE: [$@]\033[0m\n"

$(COMPREHENSIVE_TEST): tests/comprehensive_test.c $(NAME)
	@printf "\033[0;33mBuilding $@\033[0m ...\n"
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $< -L. -lft_malloc -Wl,-rpath,.
	@printf "\033[0;32mDONE: [$@]\033[0m\n"

$(INPLACE_TEST): tests/test_inplace_realloc.c $(NAME)
	@printf "\033[0;33mBuilding $@\033[0m ...\n"
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $< -L. -lft_malloc -Wl,-rpath,.
	@printf "\033[0;32mDONE: [$@]\033[0m\n"

$(LOGGER_TEST): tests/test_logger.c $(NAME)
	@printf "\033[0;33mBuilding $@ (with LOGGING=1)\033[0m ...\n"
	$(MAKE) clean
	$(MAKE) LOGGING=1
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) -D MALLOC_LOGGING  $< -L. -lft_malloc -Wl,-rpath,.
	@printf "\033[0;32mDONE: [$@]\033[0m\n"

# Build all test executables
.PHONY: build-tests
build-tests: $(ALL_TESTS)
	@printf "\n\033[0;32mAll test executables built successfully!\033[0m\n"

# -------------------------
# Individual test execution rules
# -------------------------
.PHONY: run-test
run-test: $(TEST_BIN)
	@printf "\n\033[0;34mExecutabe loaded shared libraries\033[0m\n"
	ldd ./$(TEST_BIN)
	@printf "\n\033[0;34m************** Main Test **************\033[0m\n"
	-${LDLINK} ./$(TEST_BIN) $(ARGS)
	@printf "\n\033[0;34m***************************************\033[0m\n"

.PHONY: run-comprehensive
run-comprehensive: $(COMPREHENSIVE_TEST)
	@printf "\n\033[0;34m************** Comprehensive Test **************\033[0m\n"
	./$(COMPREHENSIVE_TEST)
	@printf "\n\033[0;34m************************************************\033[0m\n"

.PHONY: run-inplace
run-inplace: $(INPLACE_TEST)
	@printf "\n\033[0;34m************** In-Place Realloc Test **************\033[0m\n"
	./$(INPLACE_TEST)
	@printf "\n\033[0;34m***************************************************\033[0m\n"

.PHONY: run-logger
run-logger: $(LOGGER_TEST)
	@printf "\n\033[0;34m************** Memory Logger Test **************\033[0m\n"
	./$(LOGGER_TEST)
	@printf "\n\033[0;34m***************************************************\033[0m\n"

# Run all tests sequentially
.PHONY: run-tests
run-tests: $(ALL_TESTS)
	@printf "\n\033[1;36m========================================\033[0m\n"
	@printf "\033[1;36m  Running All Tests Sequentially\033[0m\n"
	@printf "\033[1;36m========================================\033[0m\n"
	@$(MAKE) run-test
	@$(MAKE) run-comprehensive
	@$(MAKE) run-inplace
	@$(MAKE) run-logger
	@printf "\n\033[0;32mAll tests completed successfully!\033[0m\n"

# -------------------------
# Convenience aliases (backward compatibility)
# -------------------------
.PHONY: test test-comprehensive test-inplace test-logger test-all
test: run-test
test-comprehensive: run-comprehensive
test-inplace: run-inplace
test-logger: run-logger
test-all: run-tests

# -------------------------
# Housekeeping
# -------------------------
.PHONY: clean fclean re show check_malloc_lib
clean:
	$(RM) $(OBJ_DIR)
	@printf "\033[0;31mCleaning objs\033[0m\n"

fclean: clean
	$(RM) $(NAME) $(LINK_NAME) $(TEST_BIN) $(COMPREHENSIVE_TEST) $(LOGGER_TEST) $(INPLACE_TEST) *valgrind-out.txt *.d malloc_*.json
	@printf "\033[0;31mDeleted Everything\033[0m\n"

re: fclean all

check_malloc_lib:
	@printf "\033[0;32mChecking if malloc is loaded in $(LINK_NAME)\033[0m\n"
	-nm -D $(LINK_NAME) | grep 'malloc' --color=always
	@printf "\033[0;32m--------------------------------------------\033[0m\n"

circle.svg: FORCE
	lsphere -o . -q --svg --ignore-file .gitignore --ignore ".* *.o Libft" --palette tableau10 --ext-colors -q && rm circle.html && rm circle.json

FORCE: ;

show: check_malloc_lib
	@printf "\033[0;32m"
	@printf "OS        : $(OS)\n"
	@printf "ARCH      : $(ARCH)\n"
	@printf "HOSTTYPE  : $(HOSTTYPE)\n"
	@printf "CC        : $(CC)\n"
	@printf "CFLAGS    : $(CFLAGS)\n"
	@printf "LDFLAGS   : $(LDFLAGS)\n"
	@printf "LDLINK    : $(LDLINK)\n"
	@printf "SRC       : \033[0;33m$(SRC)\033[0;32m\n"
	@printf "OBJ       : \033[0;33m$(OBJ)\033[0m\n"

# Include auto-generated dependency files
-include $(DEP)

