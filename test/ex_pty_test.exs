defmodule ExPtyTest do
  use ExUnit.Case

  test "runs command in pseudo terminal" do
    port = ExPty.open(["sh", "-c", "stty"])

    assert_receive {^port,
                    {:data,
                     "speed 9600 baud;\r\nlflags: echoe echoke echoctl\r\noflags: -oxtabs\r\ncflags: cs8 -parenb\r\n"}}
  end
end
