# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CC       := /path/to/gcc-4.8.2/host-x86_64-unknown-linux-gnu/gcc/xgcc -B /path/to/gcc-4.8.2/host-x86_64-unknown-linux-gnu/gcc/
CFLAGS   += -g -w -pthread -fgnu-tm -mrtm -fpermissive -O2
CFLAGS   += -I$(LIB)
CPP      := /path/to/gcc-4.8.2/host-x86_64-unknown-linux-gnu/gcc/xg++ -B /path/to/gcc-4.8.2/host-x86_64-unknown-linux-gnu/gcc/
CPPFLAGS += $(CFLAGS)
LD       := /path/to/gcc-4.8.2/host-x86_64-unknown-linux-gnu/gcc/xg++ -B /path/to/gcc-4.8.2/host-x86_64-unknown-linux-gnu/gcc/ -B /path/to/gcc-4.8.2/x86_64-unknown-linux-gnu/libitm/
LIBS     += -lpthread -L /path/to/gcc-4.8.2/x86_64-unknown-linux-gnu/libitm/.libs/ -Wl,-rpath=/path/to/gcc-4.8.2/x86_64-unknown-linux-gnu/libitm/.libs/ -fgnu-tm -L /path/to/gcc-4.8.2/x86_64-unknown-linux-gnu/libstdc++-v3/src/.libs/

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib-itm

# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
