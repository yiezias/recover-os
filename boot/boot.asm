;	asmsyntax=nasm
	mov 	ax,0xb800
	mov 	ds,ax
	mov word [0],'M'+0xc00
	jmp 	$

	times 510-($-$$) 	db 	0
	dw 	0xaa55
