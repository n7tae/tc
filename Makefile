#Copyright (C) 2021 by Thomas A. Early, N7TAE

include configure.mk

# If you are going to change this path, you will
# need to update the systemd service script
BINDIR = /usr/local/bin

GCC = g++

ifeq ($(debug), true)
CFLAGS = -ggdb3 -W -Werror -Icodec2 -MMD -MD -std=c++11
else
CFLAGS = -W -Werror -Icodec2 -MMD -MD -std=c++11
endif

LDFLAGS = -lftd2xx -pthread

SRCS = Controller.cpp  DV3000.cpp  DV3003.cpp  DVSIDevice.cpp  Main.cpp  TranscoderPacket.cpp  UnixDgramSocket.cpp $(wildcard codec2/*.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all : tcd tc

tc : tc.o UnixDgramSocket.o
	$(GCC) -o $@ tc.o UnixDgramSocket.o -pthread

tcd : $(OBJS)
	$(GCC) -o $@ $(OBJS) $(LDFLAGS)

%.o : %.cpp
	$(GCC) $(CFLAGS) -c $< -o $@

clean :
	$(RM) tcd volume tc.o tc.d $(OBJS) $(DEPS)

-include $(DEPS)

# The install and uninstall targets need to be run by root
install : tcd
	cp tcd $(BINDIR)
	cp systemd/tcd.service /etc/systemd/system/
	systemctl enable tcd
	systemctl daemon-reload
	systemctl start tcd

uninstall :
	systemctl stop tcd
	systemctl disable tcd
	systemctl daemon-reload
	rm -f /etc/systemd/system/tcd.service
	rm -f $(BINDIR)/tcd
	systemctl daemon-reload
