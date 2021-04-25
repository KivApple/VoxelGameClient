cmake_minimum_required(VERSION 3.16)
project(VoxelGameClient)

set(CMAKE_CXX_STANDARD 20)

set(glm_DIR lib/glm/cmake/glm)
find_package(glm REQUIRED)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
	message("Detected usage of Emscripten")
	
	set(
			EMSCRIPTEN_FLAGS
			"-s USE_SDL=2 -s NO_EXIT_RUNTIME=1 -s EXTRA_EXPORTED_RUNTIME_METHODS=['callMain']"
	)
	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS})
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS})
	set(
			CMAKE_EXE_LINKER_FLAGS
			"${CMAKE_EXE_LINKER_FLAGS} --embed-file \"${CMAKE_CURRENT_LIST_DIR}/assets@assets\" --use-preload-plugins ${EMSCRIPTEN_FLAGS}"
	)
else()
	add_subdirectory(lib/googletest)
	
	if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
		message("Detected Windows")
		
		set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
		
		set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" ${CMAKE_MODULE_PATH})
	endif()

	find_package(SDL2 REQUIRED)
	find_package(GLEW REQUIRED)
	
	if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
		find_package(OpenGL REQUIRED)
		
		include_directories(${SDL2_INCLUDE_DIRS})
	endif()
endif()

include_directories(lib)
include_directories(src)

set(
		COMMON_SRC
		src/world/Voxel.cpp src/world/VoxelChunk.cpp src/world/VoxelWorld.cpp src/world/VoxelTypeRegistry.cpp
)

add_executable(
		VoxelGameClient
		src/main.cpp src/SDLGameEngine.cpp
		src/GameEngine.cpp
		src/OpenGL.cpp src/ShaderProgram.cpp src/TextRenderer.cpp src/world/Model.cpp
		src/ui/UIBase.cpp src/ui/UIJoystick.cpp src/ui/UIRoot.cpp
		src/world/Entity.cpp
		src/world/VoxelWorldRenderer.cpp
		${COMMON_SRC}
)
target_link_libraries(VoxelGameClient glm::glm)

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
	target_link_libraries(VoxelGameClient GLEW::GLEW)
	
	if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
		target_link_libraries(VoxelGameClient SDL2::SDL2 opengl32 ws2_32)
	else()
		target_link_libraries(VoxelGameClient ${SDL2_LIBRARIES} OpenGL::OpenGL)
	endif()
endif()

if (TARGET gtest)
	message("Detected GoogleTest")
	
	add_executable(
			VoxelGameClient_tst
			tst/main.cpp
			tst/VoxelLocation.cpp tst/VoxelWorld.cpp
			${COMMON_SRC}
	)
	target_compile_definitions(VoxelGameClient_tst PUBLIC HEADLESS)
	target_link_libraries(VoxelGameClient_tst glm::glm gtest gmock)
	
	add_test(NAME VoxelGameClient COMMAND VoxelGameClient_tst)
endif()
