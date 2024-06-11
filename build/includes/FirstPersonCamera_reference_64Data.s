.section .data.reference_64Data, "aw"
.balign 8

.global reference_64Data
.type reference_64Data, @object
.size reference_64Data, (reference_64Data_end - reference_64Data)

reference_64Data:
	.incbin "/home/baker/Code/PSX/Projects/build/FirstPersonCamera/reference_64Data.dat"
reference_64Data_end:
		
