# Makefile para libfxcg (PrizmSDK) -> genera progresiones.g3a
.RECIPEPREFIX := >

FXCGSDK ?= $(HOME)/libfxcg
APP     := progresiones
SRC     := src/main.c

CC      := sh3eb-elf-gcc
MKG3A   := mkg3a

CFLAGS  := -m4a-nofpu -mb -mhitachi -Os -Wall -ffunction-sections -fdata-sections \
           -I$(FXCGSDK)/include
LDFLAGS := -nostartfiles -T$(FXCGSDK)/toolchain/prizm.x -Wl,-static -Wl,-gc-sections \
           -L$(FXCGSDK)/lib
LIBS    := -lfxcg -lgcc

all: $(APP).g3a

$(APP).bin: $(SRC) src/core.h
> $(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LIBS)

$(APP).g3a: $(APP).bin assets/selected.bmp assets/unselected.bmp
> $(MKG3A) -n basic:Progresiones -i uns:assets/unselected.bmp -i sel:assets/selected.bmp $< $@

test:
> gcc -Wall -o /tmp/test_core tests/test_core.c && /tmp/test_core

clean:
> rm -f $(APP).bin $(APP).g3a
.PHONY: all test clean
