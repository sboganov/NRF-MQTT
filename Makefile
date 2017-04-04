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
CFLAGS=-Ofast -Wall -pthread -lwiringPi

OBJ     = $(addprefix bin/,$(patsubst %.cpp, %.o, $(wildcard *.cpp)))

bin/%.o : %.cpp
	$(CXX) -c $(CPUFLAGS) $(CFLAGS) -I/usr/local/include/RF24/.. $^ -o $@


nrf-mqtt: ${OBJ}
	@echo "[Linking]"
	$(CXX) $(CFLAGS) -pthread  -I/usr/local/include/RF24/.. -L/usr/local/lib -lrf24 -lmosquitto -o $@ $^

clean:
	rm bin/*
	rm nrf-mqtt


