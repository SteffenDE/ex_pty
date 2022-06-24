defmodule ExPty do
  @moduledoc """
  Documentation for `ExPty`.
  """

  @doc """
  Opens the specific program inside a pseudo terminal.

  ## Examples

      iex> port = ExPty.open(["bash", "-c", "stty"])
      iex> flush()
      {#Port<0.27>,
        {:data,
          "speed 9600 baud;\r\nlflags: echoe echoke echoctl\r\noflags: -oxtabs\r\ncflags: cs8 -parenb\r\n"}}
      :ok

  """
  def open(args \\ ["bash", "-i"]) do
    Port.open(
      {:spawn_executable, Application.app_dir(:ex_pty, "/priv/port_pty")},
      [
        :binary,
        :nouse_stdio,
        packet: 2,
        args: args
      ]
    )
  end

  def send_data(port, data) do
    Port.command(port, :erlang.term_to_binary({:data, data}))
  end

  def winsz(port, rows, cols) do
    Port.command(port, :erlang.term_to_binary({:winsz, rows, cols}))
  end
end
