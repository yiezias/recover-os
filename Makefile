
DISK=disk.img
BUILD_DIR=build

LD=ld
CC=gcc

LIB=-Ilib -Idevice -Ikernel
CFLAGS=-c -fno-builtin -W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-stack-protector $(LIB)
LDFLAGS=-e main -Ttext 0xffff800000000800 --no-relax


OBJS=$(BUILD_DIR)/main.o $(BUILD_DIR)/string.o $(BUILD_DIR)/print.o $(BUILD_DIR)/intr.o \
     $(BUILD_DIR)/init.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/intr_entry.o $(BUILD_DIR)/tss.o \
     $(BUILD_DIR)/timer.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/bitmap.o


run: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	bochs -q


$(DISK):
	bximage -func=create -hd=10M -imgmode=flat -q $(DISK)


clean:
	rm $(BUILD_DIR)/* -rf


# 下面这段用于自动生成c/S文件依赖
# 原理是利用gcc查找依赖写入d文件
# 然后include进Makefile中

DEPC=$(OBJS:.o=.d)

include $(DEPC)

$(BUILD_DIR)/%.d: kernel/%.[cS]
	$(CC) $(CFLAGS) -MM $< -MT $(@:.d=.o) -o $@
	@echo '	$$(CC) $$(CFLAGS) $$< -o $$@' >> $@

$(BUILD_DIR)/%.d: lib/%.[cS]
	$(CC) $(CFLAGS) -MM $< -MT $(@:.d=.o) -o $@
	@echo '	$$(CC) $$(CFLAGS) $$< -o $$@' >> $@

$(BUILD_DIR)/%.d: device/%.[cS]
	$(CC) $(CFLAGS) -MM $< -MT $(@:.d=.o) -o $@
	@echo '	$$(CC) $$(CFLAGS) $$< -o $$@' >> $@



# dd命令执行之前虚拟磁盘就已经存在了，
# 不产生新文件，不好用Makefile检查
# 更改boot.bin文件必然会写入虚拟磁盘，可以直接dd
# 因此要检查bin是否新于虚盘，是则写入
$(BUILD_DIR)/boot.bin: boot/boot.asm $(DISK)
	nasm $< -o $@
	dd if=$@ of=$(DISK) conv=notrunc

$(BUILD_DIR)/kernel.bin: $(OBJS) $(DISK)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
	dd if=$@ of=$(DISK) bs=512 seek=3 conv=notrunc


.PHONY: run clean
