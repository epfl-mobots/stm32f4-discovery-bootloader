
PROJNAME = bootloader
Q = @
COLOR		= \033[1;31m
COLOR_CLEAR = \033[0m
PRINT = @printf "$(COLOR)%s$(COLOR_CLEAR)\n"

CC = arm-none-eabi-gcc
CPP = arm-none-eabi-g++
AS  = arm-none-eabi-gcc -x assembler-with-cpp
LD = arm-none-eabi-g++
AR = arm-none-eabi-ar
OC = arm-none-eabi-objcopy
OD = arm-none-eabi-objdump
NM = arm-none-eabi-nm
SZ = arm-none-eabi-size

INCDIR += -I./CMSIS/include
INCDIR += -I./libopencm3/include
INCDIR += -I./

# C compiler flags
CFLAGS = -Wall -Os
CFLAGS += -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -fno-common -ffunction-sections -fdata-sections -fomit-frame-pointer
CFLAGS += -fno-stack-protector -ftracer -ftree-loop-distribute-patterns -frename-registers -freorder-blocks -fconserve-stack
CFLAGS += -MD
CFLAGS += $(INCDIR)
CFLAGS += -DSTM32F4 -DCPU_CFG_CPU_CORE_FREQ=168000000
CFLAGS += -g3

ASMFLAGS = $(CFLAGS)


# Linker flags
LDFLAGS = -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections -nostartfiles -nodefaultlibs --specs=nano.specs
LDFLAGS += -Tlinkerscript.ld
LDFLAGS += -lc -lnosys -lgcc
LDFLAGS	+= -L./libopencm3/lib
LDFLAGS	+= -L./CMSIS/lib/
LDFLAGS	+= -lopencm3_stm32f4

include src.mk

OBJS = $(CSRC:.c=.o) $(ASMSRC:.s=.o) $(CXXSRC:.cpp=.o)


all: $(PROJNAME).elf $(PROJNAME).bin $(PROJNAME).hex $(PROJNAME).size.txt
	$(Q) rm -f $(PROJNAME).lst
	$(Q) $(SZ) $(PROJNAME).elf

$(PROJNAME).elf: $(OBJS)
	$(PRINT) "> linking"
	$(Q) $(LD) -o $(PROJNAME).elf $(OBJS) $(LDFLAGS)


%.o: %.c Makefile
	$(PRINT) "> compiling $<"
	$(Q) $(CC) $(CFLAGS) -c ${<} -o ${@}

%.o: %.s Makefile
	$(PRINT) "> assembling $<"
	$(Q) $(AS) $(ASMFLAGS) -c ${<} -o ${@}

%.o: %.cpp Makefile
	$(PRINT) "> compiling $<"
	$(Q) $(CPP) $(CPPFLAGS) -c ${<} -o ${@}

-include $(OBJS:.o=.d)


clean:
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d)
	rm -f $(OBJS:.o=.lst)
	rm -f $(PROJNAME).elf $(PROJNAME).size.txt $(PROJNAME).lst $(PROJNAME).hex $(PROJNAME).bin

# binary file
$(PROJNAME).bin: $(PROJNAME).elf
	$(Q) $(OC) -O binary -j .text -j .rodata -j .data $(PROJNAME).elf $(PROJNAME).bin

$(PROJNAME).hex: $(PROJNAME).elf
	$(Q) $(OC) -O ihex $(PROJNAME).elf $(PROJNAME).hex

# assembly listing
$(PROJNAME).lst: $(PROJNAME).elf
	$(PRINT) "> generating assembly listing"
	$(Q) $(OD) -D -h $(PROJNAME).elf > $(PROJNAME).lst

# space usage
$(PROJNAME).size.txt: $(PROJNAME).elf
	$(PRINT) "> calculating space usage"
	$(Q) $(SZ) $(PROJNAME).elf > $(PROJNAME).size.txt
	$(Q) $(NM) --size-sort --print-size -S $(PROJNAME).elf >> $(PROJNAME).size.txt

rebuild: clean all

.PHONY: flash
flash: $(PROJNAME).elf
	openocd -f board/stm32f4discovery.cfg -c "program $(PROJNAME).elf verify reset" -c "shutdown"

