PREFIX = $(MIX_APP_PATH)/priv/$(MIX_TARGET)

DEFAULT_TARGETS ?= $(PREFIX)

all: $(PREFIX) priv/port_pty

$(PREFIX):
	mkdir -p $@

priv/port_pty: c_src/port_pty.c
	$(CC) $^ $(LDFLAGS) -fPIC -I$(ERL_EI_INCLUDE_DIR) -L $(ERL_EI_LIBDIR) -o $(PREFIX)/port_pty -l ei
