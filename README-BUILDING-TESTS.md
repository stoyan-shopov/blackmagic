The tests are built with mingw, from an msys shell, in windows.
It is most convenient to put the directories, that contain the
different toolchains, along with the `blackmagic` sources in a
directory, and to create a substituted drive for this directory
in windows with `subst x: <directory-with-toolchains-and sources>.
For example, at the time of writing this note (03022019), this is
the layout that I am using:
```
blackmagic
gcc-arm-none-eabi-5_4-2016q3-20160926-win32
gcc-arm-none-eabi-6_2-2016q4-20161216-win32
gcc-arm-none-eabi-7-2017-q4-major-win32
gcc-arm-none-eabi-8-2018-q4-major-win32
```
I.e., I have put the source code in the `blackmagic` directory,
and 4 toolchains that will be used for the test.
After that, just run the `build-tests.sh` script to
build the test executables.

Make sure to substitute the directory containing the sources and
the toolchains as the `x:` drive in windows with the `subst` command
from above, because the `build-tests.sh` script relies on this!
