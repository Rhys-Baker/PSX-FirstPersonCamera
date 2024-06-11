.section .data.fontData, "aw"
.balign 8

.global fontData
.type fontData, @object
.size fontData, (fontData_end - fontData)

fontData:
	.incbin "/home/baker/Code/PSX/Projects/build/FirstPersonCamera/fontData.dat"
fontData_end:
		
