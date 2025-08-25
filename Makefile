CC = gcc
CFLAGS = -Wall -O2 -Wno-format-zero-length -MMD -MP
LIBZJD = ./zjpgd

BUILD_DEBUG = build-debug
BUILD_RELEASE = build-release

SRCS = main.c $(LIBZJD)/zjpgd.c

OBJS_DEBUG = $(addprefix $(BUILD_DEBUG)/,$(notdir $(SRCS:.c=.o)))
OBJS_RELEASE = $(addprefix $(BUILD_RELEASE)/,$(notdir $(SRCS:.c=.o)))
DEPS_DEBUG = $(OBJS_DEBUG:.o=.d)
DEPS_RELEASE = $(OBJS_RELEASE:.o=.d)

all: zjdcli_debug zjdcli

# -----------------------------
# Debug
# -----------------------------
zjdcli_debug: CFLAGS += -DZJD_DEBUG=1
zjdcli_debug: $(OBJS_DEBUG)
	@mkdir -p $(BUILD_DEBUG)
	$(CC) $(CFLAGS) -o $@ $(OBJS_DEBUG)

$(BUILD_DEBUG)/%.o: $(LIBZJD)/%.c
	@mkdir -p $(BUILD_DEBUG)
	$(CC) $(CFLAGS) -DZJD_DEBUG=1 -I $(LIBZJD) -c $< -o $@

$(BUILD_DEBUG)/main.o: main.c
	@mkdir -p $(BUILD_DEBUG)
	$(CC) $(CFLAGS) -DZJD_DEBUG=1 -I $(LIBZJD) -c $< -o $@

# -----------------------------
# Release
# -----------------------------
zjdcli: CFLAGS += -DZJD_DEBUG=0
zjdcli: $(OBJS_RELEASE)
	@mkdir -p $(BUILD_RELEASE)
	$(CC) $(CFLAGS) -o $@ $(OBJS_RELEASE)

$(BUILD_RELEASE)/%.o: $(LIBZJD)/%.c
	@mkdir -p $(BUILD_RELEASE)
	$(CC) $(CFLAGS) -DZJD_DEBUG=0 -I $(LIBZJD) -c $< -o $@

$(BUILD_RELEASE)/main.o: main.c
	@mkdir -p $(BUILD_RELEASE)
	$(CC) $(CFLAGS) -DZJD_DEBUG=0 -I $(LIBZJD) -c $< -o $@

clean:
	rm -rf $(BUILD_DEBUG) $(BUILD_RELEASE)

-include $(DEPS_DEBUG) $(DEPS_RELEASE)
