cmake_minimum_required(VERSION 3.17)
project(VoxelGameClient)

set(CMAKE_CXX_STANDARD 20)

include(GoogleTest)

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
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	message("Detected Windows")
	
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")

	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" ${CMAKE_MODULE_PATH})
	find_package(SDL2 REQUIRED)
	find_package(GLEW REQUIRED)
endif()

include_directories(lib)

add_executable(
		VoxelGameClient
		src/main.cpp
		src/GameEngine.cpp src/SDLGameEngine.cpp
		src/OpenGL.cpp src/ShaderProgram.cpp src/TextRenderer.cpp src/world/Model.cpp
		src/ui/UIBase.cpp src/ui/UIJoystick.cpp src/ui/UIRoot.cpp
		src/world/Entity.cpp src/world/Voxel.cpp src/world/VoxelChunk.cpp src/world/VoxelWorld.cpp
		src/world/VoxelWorldRenderer.cpp
)
target_link_libraries(VoxelGameClient glm::glm)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	target_link_libraries(VoxelGameClient SDL2::SDL2 GLEW::GLEW opengl32 ws2_32)
endif()
