defmodule ExPtyTest do
  use ExUnit.Case

  test "runs command in pseudo terminal" do
    port = ExPty.open(["sh", "-c", "stty"])

    assert_receive {^port,
                    {:data,
                     "speed 9600 baud;\r\nlflags: echoe echoke echoctl\r\noflags: -oxtabs\r\ncflags: cs8 -parenb\r\n"}}
  end

  test "sending data to the pty" do
    port = ExPty.open(["cat"])

    ExPty.send_data(port, "echo\n")

    # pty echo
    assert_receive {^port, {:data, "echo\r\n"}}
    # cat result
    assert_receive {^port, {:data, "echo\r\n"}}
  end

  test "changing the window size" do
    port =
      ExPty.open([
        "bash",
        "-i",
        "-c",
        "echo started; read x; echo LINES=$(tput lines) COLUMNS=$(tput cols)"
      ])

    assert_receive {^port, {:data, "started\r\n"}}, 5000
    ExPty.winsz(port, 100, 50)
    ExPty.send_data(port, "\n")
    assert_receive {^port, {:data, "LINES=100 COLUMNS=50\r\n"}}, 500
  end
end
