# 主要的基础模块

1. 引导
- ./boot/boot.asm
2. 打印函数
- ./device/print.h
- ./device/print.c
3. 中断
- ./kernel/intr.c
- ./kernel/intr.h
- ./kernel/intr_entry.S

# 基本内存管理模块
提供对内存页的分配释放函数，虚拟内存管理相关结构。具体见./kernel/memory.h。
