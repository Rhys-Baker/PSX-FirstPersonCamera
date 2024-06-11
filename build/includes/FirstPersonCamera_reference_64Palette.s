.section .data.reference_64Palette, "aw"
.balign 8

.global reference_64Palette
.type reference_64Palette, @object
.size reference_64Palette, (reference_64Palette_end - reference_64Palette)

reference_64Palette:
	.incbin "/home/baker/Code/PSX/Projects/build/FirstPersonCamera/reference_64Palette.dat"
reference_64Palette_end:
		
