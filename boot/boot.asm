;	asmsyntax=nasm

	loader_base	equ	0x500
	mbr_size 	equ 	512
	page_base	equ 	0x100000

mbr_start:
	;0x13号中断，目的是将mbr后两个扇区内容
	;加载到内存loader_base处
	mov 	ax,0x0202	;ah功能读磁盘，al扇区数
	mov 	cx,0x0002	;ch柱面号，cl扇区号
	mov 	dh,0x00		;dh磁头号，dl驱动器号
	mov 	bx,loader_base	;es:bx缓冲区地址
	int 	0x13

	;打印'M'
	mov 	ax,0xb800
	mov 	ds,ax
	mov word [0],'M'+0xc00
	;跳转到loader_start
	jmp 	0:loader_start+loader_base-mbr_size

	;填充mbr扇区，并在结尾写上可启动标志
	times 510-($-$$) 	db 	0
	dw 	0xaa55

	;保护模式gdt
gdt_base32:
	dq 	0
code32_desc:
	dq 	0x00cf98000000ffff
data32_desc:
	dq 	0x00cf92000000ffff

gdt_ptr32:
	dw 	$-gdt_base32-1
	dd 	gdt_base32+loader_base-mbr_size

	sector_code32	equ	code32_desc-gdt_base32
	sector_data32	equ	data32_desc-gdt_base32

loader_start:
	mov word ds:[0],0x0c00+'l'
	xor 	ax,ax
	mov 	ds,ax

	;准备进入保护模式
	in 	al,0x92
	or 	al,0b10
	out 	0x92,al

	mov 	eax,cr0
	or 	eax,1
	mov 	cr0,eax

	cli

	lgdt 	[gdt_ptr32+loader_base-mbr_size]
	jmp 	sector_code32:loader_base+code32_start-mbr_size
	;进入保护模式并跳转到code32_start

	bits 	32
code32_start:
	mov 	ax,sector_data32
	mov 	ds,ax

	mov word [0xb8000],0x0c00+'P'

	;清零pml4
	mov 	ecx,0x1000
	pml4_clear:
	mov byte [ecx+page_base-1],0
	loop 	pml4_clear

	;每一级页目录项指向下一级页目录
	mov dword [page_base],page_base+0x1007
	mov dword [page_base+0x1000],page_base+0x2007
	mov dword [page_base+0x2000],page_base+0x3007
	;内核地址与用户地址指向相同位置
	mov dword [page_base+0x800],page_base+0x1007-4
	;为了分页之后还能访问到各级页目录
	mov dword [page_base+4088],page_base+7-4

	;创建pte
	mov 	ecx,256
	xor 	ebx,ebx
	mov 	eax,7
	create_pte:
	mov 	[8*ebx+page_base+0x3000],eax
	inc 	ebx
	add 	eax,0x1000
	loop 	create_pte

	jmp 	$
