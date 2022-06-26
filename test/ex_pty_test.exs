defmodule ExPTYTest do
  use ExUnit.Case

  test "runs command in pseudo terminal" do
    {:ok, pty} = ExPTY.start_link()
    ExPTY.exec(pty, ["sh", "-c", "tty"])

    assert_receive {^pty, {:data, pty}}, 500
    assert pty =~ "/dev/pts/" or pty =~ "/dev/ttys"
  end

  test "sending data to the pty" do
    {:ok, pty} = ExPTY.start_link()
    ExPTY.exec(pty, ["cat"])

    ExPTY.send_data(pty, "echo\n")

    # pty echo
    assert_receive {^pty, {:data, "echo\r\n"}}
    # cat result
    assert_receive {^pty, {:data, "echo\r\n"}}
  end

  test "changing the window size" do
    {:ok, pty} = ExPTY.start_link()

    ExPTY.exec(
      pty,
      [
        "bash",
        "-i",
        "-c",
        "echo started; read x; echo LINES=$(tput lines) COLUMNS=$(tput cols)"
      ],
      ["TERM=xterm"]
    )

    assert_receive {^pty, {:data, "started\r\n"}}, 5000
    :ok = ExPTY.winsz(pty, 100, 50)
    ExPTY.send_data(pty, "\n")
    assert_receive {^pty, {:data, "LINES=100 COLUMNS=50\r\n"}}, 500
  end

  @tag only: true
  test "setting pty options" do
    {:ok, pty} = ExPTY.start_link()

    ExPTY.exec(pty, ["cat"])

    ExPTY.send_data(pty, "echo\n")

    # pty echo
    assert_receive {^pty, {:data, "echo\r\n"}}
    # cat result
    assert_receive {^pty, {:data, "echo\r\n"}}

    :ok = ExPTY.set_pty_opts(pty, echo: 0)

    ExPTY.send_data(pty, "no echo\n")
    # cat result
    assert_receive {^pty, {:data, "no echo\r\n"}}
    # no echo result
    refute_receive {^pty, {:data, "no echo\r\n"}}
  end
end
