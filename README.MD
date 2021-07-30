# Linux watchdog daemon
Simple watchdog daemon derived from [Linux watchdog tooltest](https://code.woboq.org/linux/linux/tools/testing/selftests/watchdog/watchdog-test.c.html), simplified and daemonizable.
Init script for systemd provided.

To compile use following command :<br>
 cmake -DCMAKE_BUILD_TYPE=["BUILD_TYPE"] -DCMAKE_TOOLCHAIN_FILE=["PATH_TO_TOOLCHAIN.CMAKE_FILE"]

Example :

`cmake -DCMAKE_BUILD_TYPE="" -DCMAKE_TOOLCHAIN_FILE=toolchain-arm-linux.cmake `

## sources
[simple watchdog daemon](https://github.com/everyday-open-solution/simple-watchdog-daemon)

[Linux watchdog tooltest](https://code.woboq.org/linux/linux/tools/testing/selftests/watchdog/watchdog-test.c.html)

