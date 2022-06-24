EI_PATH = $(shell erl -eval 'application:load(erl_interface), io:format("~s", [lists:concat([code:root_dir(), "/lib/erl_interface-", element(2, application:get_key(erl_interface, vsn))])])' -s init stop -noshell)

all: priv/port_pty

priv/port_pty: c_src/port_pty.c
	cc -fPIC -I$(EI_PATH)/include -L $(EI_PATH)/lib -o priv/port_pty c_src/port_pty.c -l ei

