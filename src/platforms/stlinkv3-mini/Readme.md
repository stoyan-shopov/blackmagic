Stlink v3 mini support notes.

As of today (16.11.2020), blackmagic support for the stlinkv3-mini probe is
still a work in progress. Here are some notes of how to build, what has been
done, and what remains to be done. Thanks user RadioOperator, who provides
pinout documents for the stlink v3 mini here:
https://github.com/RadioOperator/CMSIS-DAP_for_STLINK-V3MINI/tree/master/STLINK-V3MINI

Building.

To be up to date with the main blackmagic repository, you can first clone
the main blackmagic repository, and then add this remote repository.
Then, just checkout the `stlink-v3-mini` branch, update the `libopencm3`
submodule reference, and build the firmware for the `stlinkv3-mini` host:
```
# Clone the blackmagic master repository
git clone https://github.com/blacksphere/blackmagic.git
cd blackmagic
# Add this remote repository
git remote add v3mini-origin https://github.com/stoyan-shopov/blackmagic.git
# Fetch this remote repository
git fetch v3mini-origin
# Checkout the `stlinkv3-mini-clean` repository
git checkout -b stlinkv3-mini-clean v3mini-origin/stlinkv3-mini-clean
# Initialize and update references to the libopencm3 submodule
git submodule init
git submodule update
# Build the firmware for the stlinkv3-mini host
make PROBE_HOST=stlinkv3-mini
```

What has been done so far?

- The libopencm3 usb driver for the stm32f723 device has been tweaked, and
hardware initialization for the stm32f723 usb high-speed phy controller has
been added. The libopencm3 usb driver works well, but is probably suboptimal
at this moment. There is something odd with usb at this time.
USB CDCACM speeds vary wildly with seemingly unrelated firmware changes.
USB CDCACM Loopback data rates of upto 3.8 Megabytes/second
have been observed, which seems quite low for a high speed usb device.

- The SWD bus driving works. It uses a mixture of bit-banging and hardware SPI
driving at this time. Eventually, it should use only hardware SPI driving, if
possible.

- The data uart interface works. It does not use DMA, however, and it would
benefit from switching to DMA, as in this pull request:
https://github.com/blacksphere/blackmagic/pull/719

- Trace capture works. Data rates upto 2 Megabits/second have been tested. The
theoretic maximum of UART5 used for trace capture is 27 Megabits/second.
IN-endpoint usb data rate of 8 Megabytes/second are observed. That is 2 usb
packets of size 512 bytes, each usb microframe (125 us).

What remains to be done?

Quite much. Here is (an unexhaustive) list:
- code needs to be cleaned up
- more testing is needed
- documentation needs improving
- DFU
- the reset line is not being driven
- the jtag pins are not used
- the linkage editor descriptor file used is not the correct one
- leds are not driven
- ...and more

