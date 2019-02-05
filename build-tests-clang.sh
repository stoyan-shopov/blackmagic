#!/bin/bash

# Make sure to read the notes in file `README-BUILDING-CLANG-TESTS.md`

# Apply patches needed to build with clang
patch -p1 < clang-build-blackmagic.patch
cd libopencm3/
patch -p1 < ../clang-build-libopencm3.patch
cd ../

SAVED_PATH=$PATH

#############################################
#
# Tests for toolchain: 
# clang-7.0.1
#
# The toolchain is expected to be in directory '/x/llvm/'
#
#############################################
export PATH=/x/llvm/bin/:$SAVED_PATH
#############################
#
# gdb debug information tests
#
#############################
make clean
CFLAGS="-gdwarf-5 -ggdb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw5-gdb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-5 -ggdb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw5-gdb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-4 -ggdb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw4-gdb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-4 -ggdb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw4-gdb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-3 -ggdb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw3-gdb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-3 -ggdb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw3-gdb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-2 -ggdb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw2-gdb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-2 -ggdb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw2-gdb-debug-info.elf
#############################


##############################
#
# lldb debug information tests
#
##############################
make clean
CFLAGS="-gdwarf-5 -glldb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw5-lldb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-5 -glldb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw5-lldb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-4 -glldb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw4-lldb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-4 -glldb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw4-lldb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-3 -glldb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw3-lldb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-3 -glldb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw3-lldb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-2 -glldb -Os -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-Os-dw2-lldb-debug-info.elf
#############################
make clean
CFLAGS="-gdwarf-2 -glldb -O3 -fshort-enums --sysroot=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/arm-none-eabi/" make
cp src/blackmagic ./test-elf-files/blackmagic-clang-7.0.1-O3-dw2-lldb-debug-info.elf
#############################

# Undo patches needed to build with clang
patch --reverse -p1 < clang-build-blackmagic.patch
cd libopencm3/
patch --reverse -p1 < ../clang-build-libopencm3.patch
cd ../

