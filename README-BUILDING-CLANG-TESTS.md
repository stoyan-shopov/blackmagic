Building the test executables with clang may require minor tweaks.
Most importantly, the name of the cross compiler should be adjusted,
and any unsupported compiler flags should be removed.

!!! WARNING: the most important is to set the target sysroot when invoking clang !!!
Here are some examples for mingw, you will need to tweak them for your setup:
```
CFLAGS="-gdwarf-4 -ggdb3 -Os --sysroot=/c/Program\ Files\ \(x86\)/GNU\ Tools\ ARM\ Embedded/8\ 2018-q4-major/arm-none-eabi/" make
CFLAGS="-gdwarf-4 -ggdb3 -O3 --sysroot=/c/Program\ Files\ \(x86\)/GNU\ Tools\ ARM\ Embedded/8\ 2018-q4-major/arm-none-eabi/" make
```

As of 05022019, I used these patches to build with clang 3.8.0 and 7.0.1.
The patches are very quick and dirty and only should be used for reference!
They should be cleaned up in the future. I had to manually run the linkage
editor stage of the build in order to build the executables, but compiling
with clang should be fine.

diff --git a/Makefile b/Makefile
index d0f95323..6e52a090 100644
--- a/Makefile
+++ b/Makefile
@@ -27,15 +27,7 @@ space:=
 space+=
 SRCLIBDIR:= $(subst $(space),\$(space),$(realpath lib))
 
-TARGETS ?=	stm32/f0 stm32/f1 stm32/f2 stm32/f3 stm32/f4 stm32/f7 \
-		stm32/l0 stm32/l1 stm32/l4 \
-		lpc13xx lpc17xx lpc43xx/m4 lpc43xx/m0 \
-		lm3s lm4f msp432/e4 \
-		efm32/tg efm32/g efm32/lg efm32/gg efm32/hg efm32/wg \
-		efm32/ezr32wg \
-		sam/3a sam/3n sam/3s sam/3u sam/3x sam/4l \
-		sam/d \
-		vf6xx
+TARGETS ?=	stm32/f0 stm32/f1
 
 # Be silent per default, but 'make V=1' will show all compiler calls.
 ifneq ($(V),1)
diff --git a/lib/stm32/f0/Makefile b/lib/stm32/f0/Makefile
index a5883643..bfc8071b 100644
--- a/lib/stm32/f0/Makefile
+++ b/lib/stm32/f0/Makefile
@@ -22,7 +22,7 @@ SRCLIBDIR	?= ../..
 
 PREFIX		?= arm-none-eabi
 #PREFIX		?= arm-elf
-CC		= $(PREFIX)-gcc
+CC		= clang -target $(PREFIX)
 AR		= $(PREFIX)-ar
 TGT_CFLAGS	= -Os \
 		  -Wall -Wextra -Wimplicit-function-declaration \
diff --git a/lib/stm32/f1/Makefile b/lib/stm32/f1/Makefile
index c61f737b..856ee5aa 100755
--- a/lib/stm32/f1/Makefile
+++ b/lib/stm32/f1/Makefile
@@ -22,7 +22,7 @@ SRCLIBDIR	?= ../..
 
 PREFIX		?= arm-none-eabi
 
-CC		= $(PREFIX)-gcc
+CC		= clang -target $(PREFIX)
 AR		= $(PREFIX)-ar
 TGT_CFLAGS	= -Os \
 		  -Wall -Wextra -Wimplicit-function-declaration \
diff --git a/Makefile b/Makefile
index 358c6870..8bfd0c49 100644
--- a/Makefile
+++ b/Makefile
@@ -1,6 +1,6 @@
 ifneq ($(V), 1)
 MFLAGS += --no-print-dir
-Q := @
+Q :=
 endif

 all:
diff --git a/libopencm3 b/libopencm3
--- a/libopencm3
+++ b/libopencm3
@@ -1 +1 @@
-Subproject commit 8064f6d0cbaca9719c25ee74af115f90deb2b3a0
+Subproject commit 8064f6d0cbaca9719c25ee74af115f90deb2b3a0-dirty
diff --git a/src/platforms/vx-stm32f070/Makefile.inc b/src/platforms/vx-stm32f070/Makefile.inc
index c0bd8da2..b3c324cd 100644
--- a/src/platforms/vx-stm32f070/Makefile.inc
+++ b/src/platforms/vx-stm32f070/Makefile.inc
@@ -1,8 +1,8 @@
 CROSS_COMPILE ?= arm-none-eabi-
-CC = $(CROSS_COMPILE)gcc
+CC = clang -target arm-none-eabi
 OBJCOPY = $(CROSS_COMPILE)objcopy

-OPT_FLAGS = -finline-functions -fvar-tracking-assignments -fvar-tracking
+OPT_FLAGS =

 CFLAGS += -mcpu=cortex-m0 -mthumb \
        -DSTM32F0 -I../libopencm3/include \
diff --git a/src/platforms/vx-stm32f070/platform.c b/src/platforms/vx-stm32f070/platform.c
index 9f8eca63..06655158 100644
--- a/src/platforms/vx-stm32f070/platform.c
+++ b/src/platforms/vx-stm32f070/platform.c
@@ -275,6 +275,9 @@ void sv_call_handler(void)
 }


+
+#define asm(...)
+
 static uint32_t swdptap_seq_in_32bits_optimized_asm(struct sw_driving_data * sw) __attribute__((naked));
 static uint32_t swdptap_seq_in_32bits_optimized_asm(struct sw_driving_data * sw)
 {
diff --git a/src/platforms/vx-stm32f070/stm32f070.ld b/src/platforms/vx-stm32f070/stm32f070.ld
index 1ee604d3..6d86731f 100644
--- a/src/platforms/vx-stm32f070/stm32f070.ld
+++ b/src/platforms/vx-stm32f070/stm32f070.ld
@@ -6,5 +6,6 @@ MEMORY
 }

 /* Include the common ld script. */
-INCLUDE libopencm3_stm32f0.ld
+INCLUDE cortex-m-generic.ld
+

diff --git a/src/sforth b/src/sforth
--- a/src/sforth
+++ b/src/sforth
@@ -1 +1 @@
-Subproject commit a3e7e15e7b5afa18dc429091726b73d0f1a12942
+Subproject commit a3e7e15e7b5afa18dc429091726b73d0f1a12942-dirty

