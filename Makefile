PREFIX = $(MIX_APP_PATH)/priv/$(MIX_TARGET)

DEFAULT_TARGETS ?= $(PREFIX)

all: $(PREFIX) port_pty

$(PREFIX):
	mkdir -p $@

port_pty: c_src/erl_comm.c c_src/port_pty.c
	$(CC) $^ $(LDFLAGS) -fPIC -Wno-pointer-sign -I$(ERL_EI_INCLUDE_DIR) -L $(ERL_EI_LIBDIR) -o $(PREFIX)/port_pty -lei -lpthread
