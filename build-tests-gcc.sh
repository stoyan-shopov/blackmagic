#!/bin/bash

# Make sure to read the notes in file `README-BUILDING-GCC-TESTS.md`

SAVED_PATH=$PATH

#############################################
#
# Tests for toolchain: 
# gcc-arm-none-eabi-5_4-2016q3-20160926-win32
#
#############################################
export PATH=/x/gcc-arm-none-eabi-5_4-2016q3-20160926-win32/bin/:$SAVED_PATH
#############################
make clean
CFLAGS="-O3 -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-O3-dw-5.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-O3-dw-4.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-O3-dw-3.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-O3-dw-2.elf

#############################
make clean
CFLAGS="-Os -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-Os-dw-5.elf
#############################
make clean
CFLAGS="-Os -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-Os-dw-4.elf
#############################
make clean
CFLAGS="-Os -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-Os-dw-3.elf
#############################
make clean
CFLAGS="-Os -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-5.4-Os-dw-2.elf


#############################################
#
# Tests for toolchain: 
# gcc-arm-none-eabi-6_2-2016q4-20161216-win32
#
#############################################
export PATH=/x/gcc-arm-none-eabi-6_2-2016q4-20161216-win32/bin/:$SAVED_PATH
#############################
make clean
CFLAGS="-O3 -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-O3-dw-5.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-O3-dw-4.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-O3-dw-3.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-O3-dw-2.elf

#############################
make clean
CFLAGS="-Os -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-Os-dw-5.elf
#############################
make clean
CFLAGS="-Os -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-Os-dw-4.elf
#############################
make clean
CFLAGS="-Os -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-Os-dw-3.elf
#############################
make clean
CFLAGS="-Os -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-6.2-Os-dw-2.elf


#############################################
#
# Tests for toolchain: 
# gcc-arm-none-eabi-7-2017-q4-major-win32
#
#############################################
export PATH=/x/gcc-arm-none-eabi-7-2017-q4-major-win32/bin/:$SAVED_PATH
#############################
make clean
CFLAGS="-O3 -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-O3-dw-5.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-O3-dw-4.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-O3-dw-3.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-O3-dw-2.elf

#############################
make clean
CFLAGS="-Os -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-Os-dw-5.elf
#############################
make clean
CFLAGS="-Os -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-Os-dw-4.elf
#############################
make clean
CFLAGS="-Os -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-Os-dw-3.elf
#############################
make clean
CFLAGS="-Os -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-7-Os-dw-2.elf


#############################################
#
# Tests for toolchain: 
# gcc-arm-none-eabi-8-2018-q4-major-win32
#
#############################################
export PATH=/x/gcc-arm-none-eabi-8-2018-q4-major-win32/bin/:$SAVED_PATH
#############################
make clean
CFLAGS="-O3 -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-O3-dw-5.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-O3-dw-4.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-O3-dw-3.elf
#############################
make clean
CFLAGS="-O3 -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-O3-dw-2.elf

#############################
make clean
CFLAGS="-Os -gdwarf-5 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-Os-dw-5.elf
#############################
make clean
CFLAGS="-Os -gdwarf-4 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-Os-dw-4.elf
#############################
make clean
CFLAGS="-Os -gdwarf-3 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-Os-dw-3.elf
#############################
make clean
CFLAGS="-Os -gdwarf-2 -ggdb3" make
cp src/blackmagic ./test-elf-files/blackmagic-gcc-8-Os-dw-2.elf

