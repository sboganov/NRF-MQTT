#############################################################################
#
# Makefile for librf24 examples on Linux
#
# License: GPL (General Public License)
# Author:  gnulnulf <arco@appeltaart.mine.nu>
# Date:    2013/02/07 (version 1.0)
#
# Description:
# ------------
# use make all and make install to install the examples
#

CC=arm-linux-gnueabihf-gcc
CXX=arm-linux-gnueabihf-g++
CPUFLAGS=-march=armv8-a -mtune=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS=-march=armv8-a -mtune=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard -Ofast -Wall -pthread 


rx-example: RF24Network.cpp rx-example.o
	@echo "[Linking]"
	$(CXX) $(CPUFLAGS) $(CFLAGS) -pthread  -I/usr/local/include/RF24/.. -L/usr/local/lib -lrf24 -o rx-example $^


