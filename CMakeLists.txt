#
# (C) 2024 Rhys Baker, spicyjpeg
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

cmake_minimum_required(VERSION 3.25)

# Set the path to the toolchain file, which will configure CMake to use the MIPS
# toolchain rather than its default compiler and proceed in turn to execute
# setup.cmake.
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain.cmake")

# Tell CMake about the project. The VERSION, DESCRIPTION and HOMEPAGE_URL fields
# are optional, but the project name and LANGUAGES field should be present.
project(
	ps1-bare-metal
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "PlayStation 1 bare-metal C examples"
	HOMEPAGE_URL "https://github.com/spicyjpeg/ps1-bare-metal"
)

# Locate a working Python installation in order to run the scripts in the tools
# directory.
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

# Build a "common" library containing code shared across all examples. We are
# going to link this library into each example.
add_library(
	common OBJECT
	src/libc/crt0.c
	src/libc/cxxsupport.cpp
	src/libc/malloc.c
	src/libc/memset.s
	src/libc/misc.c
	src/libc/misc.s
	src/libc/string.c
	src/vendor/printf.c

	# My own includes
	src/include/controller.c
	src/include/font.c
	src/include/gpu.c
	src/include/gte.c
	src/include/trig.c
	src/include/camera.c

)
target_include_directories(
	common PUBLIC
	src
	src/libc
	src/include
)

# Define a helper function to build each example.
function(addProject name)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} PRIVATE common)

	# Add a step to run convertExecutable.py after each example is compiled in
	# order to convert it into a PS1 executable. By default all custom commands
	# run from the build directory, so paths to files in the source directory
	# must be prefixed with ${PROJECT_SOURCE_DIR}.
	add_custom_command(
		TARGET     ${name} POST_BUILD
		BYPRODUCTS ${name}.psexe
		COMMAND
			"${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/convertExecutable.py"
			"$<TARGET_FILE:${name}>" ${name}.psexe
		VERBATIM
	)
endfunction()

# Define more helper functions to embed binary data into executables and convert
# images into raw texture and palette data.
function(addBinaryFile target name path)
	set(_file "${PROJECT_BINARY_DIR}/includes/${target}_${name}.s")
	cmake_path(ABSOLUTE_PATH path OUTPUT_VARIABLE _path)

	# Generate an assembly listing that uses the .incbin directive to embed the
	# file and add it to the executable's list of source files. This may look
	# hacky, but it works and lets us easily customize the symbol name (i.e. the
	# name of the "array" that will contain the file's data).
	file(
		CONFIGURE
		OUTPUT  "${_file}"
		CONTENT [[
.section .data.${name}, "aw"
.balign 8

.global ${name}
.type ${name}, @object
.size ${name}, (${name}_end - ${name})

${name}:
	.incbin "${_path}"
${name}_end:
		]]
		ESCAPE_QUOTES
		NEWLINE_STYLE LF
	)

	target_sources(${target} PRIVATE "${_file}")
	set_source_files_properties("${_file}" PROPERTIES OBJECT_DEPENDS "${_path}")
endfunction()

function(convertImage input bpp)
	add_custom_command(
		OUTPUT  ${ARGN}
		DEPENDS "${PROJECT_SOURCE_DIR}/${input}"
		COMMAND
			"${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/convertImage.py"
			-b ${bpp} "${PROJECT_SOURCE_DIR}/${input}" ${ARGN}
		VERBATIM
	)
endfunction()

# Project files
addProject(FirstPersonCamera src/FirstPersonCamera/main.c)


# 16 BPP Textures

#8 BPP Textures

#4 BPP Textures
convertImage(src/assets/textures/4/font.png 4 FirstPersonCamera/fontData.dat FirstPersonCamera/fontPalette.dat)
addBinaryFile(FirstPersonCamera fontData "${PROJECT_BINARY_DIR}/FirstPersonCamera/fontData.dat")
addBinaryFile(FirstPersonCamera fontPalette "${PROJECT_BINARY_DIR}/FirstPersonCamera/fontPalette.dat")

convertImage(src/assets/textures/4/reference_64.png 4 FirstPersonCamera/reference_64Data.dat FirstPersonCamera/reference_64Palette.dat)
addBinaryFile(FirstPersonCamera reference_64Data "${PROJECT_BINARY_DIR}/FirstPersonCamera/reference_64Data.dat")
addBinaryFile(FirstPersonCamera reference_64Palette "${PROJECT_BINARY_DIR}/FirstPersonCamera/reference_64Palette.dat")
