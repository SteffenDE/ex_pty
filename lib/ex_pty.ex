defmodule ExPTY do
  use GenServer, restart: :temporary

  @moduledoc """
  Documentation for `ExPTY`.
  """

  @target Mix.target()

  @doc """
  Opens the specific program inside a pseudo terminal.

  ## Examples

      iex> port = ExPTY.open(["bash", "-c", "stty"])
      iex> flush()
      {#Port<0.27>,
        {:data,
          "speed 9600 baud;\r\nlflags: echoe echoke echoctl\r\noflags: -oxtabs\r\ncflags: cs8 -parenb\r\n"}}
      :ok

  """
  def start_link do
    GenServer.start_link(__MODULE__, handler: self())
  end

  def start_link(args) do
    GenServer.start_link(__MODULE__, args)
  end

  @impl true
  def init(args) do
    handler = Keyword.fetch!(args, :handler)
    Process.flag(:trap_exit, true)

    port =
      Port.open(
        {:spawn_executable, Application.app_dir(:ex_pty, "/priv/#{@target}/port_pty")},
        [
          :binary,
          :nouse_stdio,
          packet: 2
        ]
      )

    {:ok, %{port: port, handler: handler, callers: %{}}}
  end

  @impl true
  def handle_cast({:exec, command, env}, state) do
    Port.command(state.port, :erlang.term_to_binary({:exec, command, env}))

    {:noreply, state}
  end

  @impl true
  def handle_cast({:data, data}, state) do
    Port.command(state.port, :erlang.term_to_binary({:data, data}))

    {:noreply, state}
  end

  @impl true
  def handle_call({:winsz, rows, cols}, from, state) do
    ref = make_ref()
    Port.command(state.port, :erlang.term_to_binary({:winsz, ref, rows, cols}))

    {:noreply, update_in(state, [:callers], &Map.put(&1, ref, from))}
  end

  def handle_call({:pty_opts, pty_opts}, from, state) do
    ref = make_ref()
    Port.command(state.port, :erlang.term_to_binary({:pty_opts, ref, pty_opts}))

    {:noreply, update_in(state, [:callers], &Map.put(&1, ref, from))}
  end

  @impl true
  def handle_info({port, {:data, data}}, state = %{port: port}) do
    case :erlang.binary_to_term(data) do
      {:response, ref, data} ->
        from = Map.fetch!(state.callers, ref)
        GenServer.reply(from, data)
        {:noreply, update_in(state, [:callers], &Map.delete(&1, from))}

      {:data, data} ->
        send(state.handler, {self(), {:data, data}})
        {:noreply, state}

      {:exit, code} ->
        send(state.handler, {self(), {:exit, code}})
        {:noreply, state}
    end
  end

  def handle_info({:EXIT, port, reason}, state = %{port: port}) do
    send(state.handler, {:EXIT, self(), reason})

    {:stop, reason, state}
  end

  def exec(server, command, env \\ []) do
    GenServer.cast(server, {:exec, command, Enum.map(env, &map_env/1)})
  end

  @doc """
  Send data to the pty.
  """
  def send_data(server, data) do
    GenServer.cast(server, {:data, data})
  end

  @doc """
  Set the pty_opts based on the provided keyword list

  ## Example

      iex> ExPTY.set_pty_opts(echo: 1, echoke: 1)
  """
  def set_pty_opts(server, pty_opts) do
    GenServer.call(server, {:pty_opts, pty_opts})
  end

  @doc """
  Change the window size of the pty.
  """
  def winsz(server, rows, cols) do
    GenServer.call(server, {:winsz, rows, cols})
  end

  defp map_env({key, value}), do: to_string(key) <> "=" <> to_string(value)
  defp map_env(str) when is_binary(str), do: str
end
