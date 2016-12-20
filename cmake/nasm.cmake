# Written by Mário Kašuba (mario.kasuba at it-academy dot sk), 2016

if(WIN32)
	set(NASM_COMPILER_PATH "nasm.exe" CACHE FILEPATH "NASM assembler program path")
	set(NASM_OBJ_FORMAT win32)
else()
	set(NASM_COMPILER_PATH "nasm" CACHE FILEPATH "NASM assembler program path")
	set(NASM_OBJ_FORMAT elf32)
endif()

set(CMAKE_ASM_MASM_COMPILER ${NASM_COMPILER_PATH})

function(nasm_command SRC_LIST_NAME OUT_LIST_NAME)
	set(ASM_OUT_LIST "")
	foreach(ASM_SRC ${${SRC_LIST_NAME}})
		get_filename_component(ASM_WE ${ASM_SRC} NAME_WE)
		get_filename_component(ASM_PATH ${ASM_SRC} DIRECTORY)
		if (WIN32)
			set(ASM_OUT "${CMAKE_BINARY_DIR}/${ASM_PATH}/${ASM_WE}.obj")
		else()
			set(ASM_OUT "${CMAKE_BINARY_DIR}/${ASM_PATH}/${ASM_WE}.o")
		endif()
		list(APPEND ASM_OUT_LIST ${ASM_OUT})

		add_custom_command(
			OUTPUT ${ASM_OUT}
			COMMAND ${CMAKE_ASM_MASM_COMPILER}
			ARGS -f ${NASM_OBJ_FORMAT} -d OBJ_FORMAT_${NASM_OBJ_FORMAT} "${CMAKE_CURRENT_SOURCE_DIR}/${ASM_SRC}" -o "${ASM_OUT}" -I "${CMAKE_CURRENT_SOURCE_DIR}/${ASM_PATH}/"
			MAIN_DEPENDENCY ${ASM_SRC}
			COMMENT "generate ${ASM_OUT}"
		)
	endforeach()
	set (${OUT_LIST_NAME} ${ASM_OUT_LIST} PARENT_SCOPE)
endfunction()