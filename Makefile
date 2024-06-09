
DISK=disk.img
BUILD_DIR=build

run: $(BUILD_DIR)/boot.bin
	bochs -q


$(DISK):
	bximage -func=create -hd=10M -imgmode=flat -q $(DISK)


clean:
	rm $(BUILD_DIR)/* -rf

# dd命令本身不产生新文件，不好用Makefile检查
# 更改boot.bin文件必然会写入虚拟磁盘，可以直接dd
# 因此要检查bin是否新于虚盘，是则写入
$(BUILD_DIR)/boot.bin: boot/boot.asm $(DISK)
	nasm $< -o $@
	dd if=$@ of=$(DISK) conv=notrunc
