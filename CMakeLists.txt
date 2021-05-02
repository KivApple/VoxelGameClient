cmake_minimum_required(VERSION 3.16)
project(VoxelGameClient)

set(CMAKE_CXX_STANDARD 20)

add_library(easyloggingpp::easyloggingpp INTERFACE IMPORTED)
target_include_directories(easyloggingpp::easyloggingpp INTERFACE lib/easyloggingpp/src)
target_compile_definitions(easyloggingpp::easyloggingpp INTERFACE ELPP_THREAD_SAFE ELPP_NO_DEFAULT_LOG_FILE)
target_sources(easyloggingpp::easyloggingpp INTERFACE lib/easyloggingpp/src/easylogging++.cc)

add_subdirectory(lib/glm)
add_subdirectory(lib/bitsery)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
	message("Detected usage of Emscripten")
	
	set(
			EMSCRIPTEN_FLAGS
			"-s ALLOW_MEMORY_GROWTH=1 -s USE_SDL=2 -s NO_EXIT_RUNTIME=1 -s EXTRA_EXPORTED_RUNTIME_METHODS=['callMain']"
	)
	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS})
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS})
	set(
			CMAKE_EXE_LINKER_FLAGS
			"${CMAKE_EXE_LINKER_FLAGS} --embed-file \"${CMAKE_CURRENT_LIST_DIR}/assets@assets\" --use-preload-plugins ${EMSCRIPTEN_FLAGS}"
	)
else()
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	
	add_library(asio::asio INTERFACE IMPORTED)
	target_include_directories(asio::asio INTERFACE lib/asio/asio/include)
	target_compile_definitions(asio::asio INTERFACE ASIO_STANDALONE)
	#target_sources(asio::asio INTERFACE lib/asio/asio/src/asio.cpp)
	
	add_library(websocketpp::websocketpp INTERFACE IMPORTED)
	target_include_directories(websocketpp::websocketpp INTERFACE lib/websocketpp)
	target_compile_definitions(websocketpp::websocketpp INTERFACE _WEBSOCKETPP_CPP11_THREAD_)
	
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
		src/client/main.cpp src/client/SDLGameEngine.cpp
		src/client/net/BinaryClientTransport.cpp src/client/net/WebSocketClientTransport.cpp
		src/client/GameEngine.cpp src/client/net/ClientTransport.cpp
		src/client/OpenGL.cpp src/client/ShaderProgram.cpp src/client/ui/TextRenderer.cpp src/client/world/Model.cpp
		src/client/ui/UIBase.cpp src/client/ui/UIJoystick.cpp src/client/ui/UIRoot.cpp
		src/client/ui/Crosshair.cpp src/client/ui/VoxelOutline.cpp
		src/client/world/Entity.cpp
		src/client/world/VoxelWorldRenderer.cpp
		${COMMON_SRC}
)
target_link_libraries(VoxelGameClient easyloggingpp::easyloggingpp glm::glm Bitsery::bitsery)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
	target_link_libraries(VoxelGameClient websocket.js)
else()
	target_link_libraries(VoxelGameClient GLEW::GLEW websocketpp::websocketpp asio::asio Threads::Threads)
	
	if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
		target_link_libraries(VoxelGameClient SDL2::SDL2 opengl32 ws2_32 wsock32)
	else()
		target_link_libraries(VoxelGameClient ${SDL2_LIBRARIES} OpenGL::OpenGL)
	endif()
	
	add_executable(
			VoxelGameServer
			src/server/main.cpp src/server/GameServerEngine.cpp src/server/net/WebSocketServerTransport.cpp
			src/server/net/ClientConnection.cpp src/server/net/BinaryServerTransport.cpp
			src/server/world/VoxelWorldGenerator.cpp src/server/world/VoxelLightComputer.cpp
			${COMMON_SRC}
	)
	target_compile_definitions(VoxelGameServer PUBLIC HEADLESS)
	target_link_libraries(
			VoxelGameServer
			easyloggingpp::easyloggingpp glm::glm Bitsery::bitsery websocketpp::websocketpp asio::asio Threads::Threads
	)
	if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
		target_link_libraries(VoxelGameServer ws2_32 wsock32)
	endif()
endif()

if(TARGET gtest)
	message("Detected GoogleTest")
	
	add_executable(
			VoxelGameClient_tst
			tst/main.cpp
			tst/VoxelLocation.cpp tst/VoxelWorld.cpp tst/VoxelSerialization.cpp
			${COMMON_SRC}
	)
	target_compile_definitions(VoxelGameClient_tst PUBLIC HEADLESS)
	target_link_libraries(VoxelGameClient_tst easyloggingpp::easyloggingpp glm::glm Bitsery::bitsery gtest gmock)
	
	add_test(NAME VoxelGameClient COMMAND VoxelGameClient_tst)
endif()
