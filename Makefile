
BUILD_DIR=build
DISK=disk.img
KERNEL=$(BUILD_DIR)/kernel.bin

OBJS=$(BUILD_DIR)/main.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/print.o $(BUILD_DIR)/string.o \
     $(BUILD_DIR)/init.o $(BUILD_DIR)/intr.o $(BUILD_DIR)/intr_entry.o $(BUILD_DIR)/memory.o \
     $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/task.o $(BUILD_DIR)/list.o \
     $(BUILD_DIR)/switch.o $(BUILD_DIR)/timer.o


CC=gcc
LD=ld

LIB=-I lib -I device -I kernel -I task
CFLAGS=-c -fno-builtin -W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector $(LIB)
LDFLAGS=-e main -Ttext 0xffff800000000800 --no-relax


DEPC=$(OBJS:.o=.d)

include $(DEPC)

run: all
	bochs -q

all: boot kernel


boot: $(BUILD_DIR)/boot.bin

kernel: $(KERNEL)


# 生成各个目录依赖文件

$(BUILD_DIR)/%.d: kernel/%.[cS]
	$(CC) $(CFLAGS) -MM $< -MT $(@:.d=.o) -o $@
	@echo '	$$(CC) $$(CFLAGS) $$< -o $$@' >> $@

$(BUILD_DIR)/%.d: device/%.[cS]
	$(CC) $(CFLAGS) -MM $< -MT $(@:.d=.o) -o $@
	@echo '	$$(CC) $$(CFLAGS) $$< -o $$@' >> $@

$(BUILD_DIR)/%.d: lib/%.[cS]
	$(CC) $(CFLAGS) -MM $< -MT $(@:.d=.o) -o $@
	@echo '	$$(CC) $$(CFLAGS) $$< -o $$@' >> $@

$(BUILD_DIR)/%.d: task/%.[cS]
	$(CC) $(CFLAGS) -MM $< -MT $(@:.d=.o) -o $@
	@echo '	$$(CC) $$(CFLAGS) $$< -o $$@' >> $@


# 生成引导文件内核文件及虚拟硬盘文件

$(BUILD_DIR)/boot.bin: boot/boot.asm $(DISK)
	nasm $< -o $@
	dd if=$@ of=$(DISK) conv=notrunc

$(KERNEL): $(OBJS) $(DISK)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
	dd if=$(KERNEL) of=$(DISK) bs=512 seek=3 conv=notrunc

$(DISK):
	bximage -func=create -hd=10M -imgmode=flat -q $(DISK)

# 清理生成文件

clean:
	rm -rf $(BUILD_DIR)/*


.PHONY: run clean all
