# nasm build rule:
%.lo: %.s
	$(NASM) -f elf -d ELF $< -o $@

noinst_LTLIBRARIES = libFLAC-asm.la
libFLAC_asm_la_SOURCES = \
	cpu_asm.s \
	fixed_asm.s \
	lpc_asm.s
