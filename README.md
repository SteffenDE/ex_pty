# ExPTY

For something that can be used in production, have a look at [erlexec](https://github.com/saleyn/erlexec).

Run any system command in a pseudo terminal inside a GenServer process.

**Be warned**: my C code seems to work, but is very ugly

## Installation

The package can be installed by adding `ex_pty` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:ex_pty, github: "SteffenDE/ex_pty"}
  ]
end
```

## Usage

```elixir
iex()> {:ok, pty} = ExPTY.start_link(handler: self())
iex()> flush()
{#PID<0.257.0>, {:data, "bash-3.2$ "}}
iex()> ExPTY.send_data(pty, "echo cool\n")
:ok
iex()> flush()
{#PID<0.257.0>, {:data, "echo cool\r\ncool\r\nbash-3.2$ "}}
iex()> ExPTY.send_data(pty, "exit\n")
:ok
iex()> flush()
{:EXIT, #PID<0.257.0>, :normal}
```
