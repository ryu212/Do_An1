CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
MCU = cortex-m3
LS = linker_script.ld

CFLAGS = -mcpu=$(MCU) -mthumb -std=gnu11 -O0 -Wall
LFLAGS = -nostartfiles -T $(LS)
BFLAGS = -O binary

TARGET := myprogram
SRC_DIR = src
OBJ_DIR = obj

# Tự động lấy tất cả file .c và .s trong Src/
SRC_C := $(wildcard $(SRC_DIR)/*.c)
SRC_S := $(wildcard $(SRC_DIR)/*.s)
OBJS  := $(SRC_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) \
         $(SRC_S:$(SRC_DIR)/%.s=$(OBJ_DIR)/%.o)

all: $(TARGET).bin

$(TARGET).elf: $(OBJS)
	@echo "🔗 Linking..."
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) $(BFLAGS) $< $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(OBJ_DIR)
	$(CC) -mcpu=$(MCU) -mthumb -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(TARGET).elf $(TARGET).hex $(TARGET).bin
