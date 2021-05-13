if(NOT SDL2_FIND_VERSION)
	set(SDL2_FIND_VERSION 2.0.14)
endif()

if(MSVC)
	set(SDL2_TARGET VC)
	set(SDL2_ARCHIVE_SUFFIX .zip)
else()
	set(SDL2_TARGET mingw)
	set(SDL2_ARCHIVE_SUFFIX .tar.gz)
endif()
set(SDL2_ARCHIVE_FILE_NAME "SDL2-devel-${SDL2_FIND_VERSION}-${SDL2_TARGET}${SDL2_ARCHIVE_SUFFIX}")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/sdl2/${SDL2_ARCHIVE_FILE_NAME}")
	file(
			DOWNLOAD
			"https://www.libsdl.org/release/${SDL2_ARCHIVE_FILE_NAME}"
			"${CMAKE_CURRENT_BINARY_DIR}/sdl2/${SDL2_ARCHIVE_FILE_NAME}"
			SHOW_PROGRESS
	)
endif()

set(SDL2_ARCHIVE_DIR "${CMAKE_CURRENT_BINARY_DIR}/sdl2/SDL2-devel-${SDL2_FIND_VERSION}-${SDL2_TARGET}")

file(MAKE_DIRECTORY "${SDL2_ARCHIVE_DIR}")
execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_CURRENT_BINARY_DIR}/sdl2/${SDL2_ARCHIVE_FILE_NAME}"
		WORKING_DIRECTORY "${SDL2_ARCHIVE_DIR}"
)

if(MSVC)
	set(SDL2_DIR "${SDL2_ARCHIVE_DIR}/SDL2-${SDL2_FIND_VERSION}")
	if (NOT EXISTS "${SDL2_ARCHIVE_DIR}/include")
		file(MAKE_DIRECTORY "${SDL2_ARCHIVE_DIR}/include")
		file(COPY "${SDL2_DIR}/include" DESTINATION "${SDL2_ARCHIVE_DIR}/include")
		file(RENAME "${SDL2_ARCHIVE_DIR}/include/include" "${SDL2_ARCHIVE_DIR}/include/SDL2")
	endif()
else()
	#if("${CMAKE_HOST_SYSTEM_PROCESSOR}" EQUAL "AMD64")
	#	set(SDL2_DIR "${SDL2_ARCHIVE_DIR}/SDL2-${SDL2_FIND_VERSION}/x86_64-w64-mingw32")
	#else()
		set(SDL2_DIR "${SDL2_ARCHIVE_DIR}/SDL2-${SDL2_FIND_VERSION}/i686-w64-mingw32")
	#endif()
endif()

if(MSVC)
	set(SDL2_INCLUDE_DIRS "${SDL2_ARCHIVE_DIR}/include")
	if(CMAKE_CL_64)
		set(SDL2_ARCH x64)
	else()
		set(SDL2_ARCH x86)
	endif()
	set(SDL2_SHARED_LIBRARIES "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll")
	set(
			SDL2_LIBRARIES
			"${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2main.lib" "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.lib"
			dinput8 dxguid user32 gdi32 winmm imm32 ole32 oleaut32 shell32 setupapi version uuid
	)
else()
	set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include")
	#set(SDL2_SHARED_LIBRARIES "${SDL2_DIR}/bin/SDL2.dll")
	set(
			SDL2_LIBRARIES mingw32
			"${SDL2_DIR}/lib/libSDL2main.a" "${SDL2_DIR}/lib/libSDL2.a"
			dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 setupapi version uuid
	)
endif()

foreach(SDL2_SHARED_LIBRARY ${SDL2_SHARED_LIBRARIES})
	get_filename_component(SDL2_SHARED_LIBRARY_FILENAME ${SDL2_SHARED_LIBRARY} NAME)
	get_filename_component(SDL2_SHARED_LIBRARY_PATH "${CMAKE_CURRENT_BINARY_DIR}/${SDL2_SHARED_LIBRARY_FILENAME}" ABSOLUTE)
	add_custom_target(
			"${SDL2_SHARED_LIBRARY_FILENAME}"
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_SHARED_LIBRARY}" "${SDL2_SHARED_LIBRARY_PATH}"
			BYPRODUCTS "${SDL2_SHARED_LIBRARY_PATH}"
	)
	if(NOT MSVC)
		list(APPEND SDL2_LIBRARIES "${SDL2_SHARED_LIBRARY_FILENAME}")
	endif()
endforeach()

add_library(SDL2::SDL2 INTERFACE IMPORTED)
target_compile_definitions(SDL2::SDL2 INTERFACE -Dmain=SDL_main)
target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
target_link_directories(SDL2::SDL2 INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})
