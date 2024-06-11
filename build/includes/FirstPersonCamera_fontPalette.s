.section .data.fontPalette, "aw"
.balign 8

.global fontPalette
.type fontPalette, @object
.size fontPalette, (fontPalette_end - fontPalette)

fontPalette:
	.incbin "/home/baker/Code/PSX/Projects/build/FirstPersonCamera/fontPalette.dat"
fontPalette_end:
		
