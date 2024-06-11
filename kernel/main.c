
int main(void) {
	unsigned long display = 0xffff8000000b8000;
	*(short *)display = 0xc00 + 'K';
	while (1) {}
	return 0;
}
