#include <cstdio>
#include <stdexcept>
#include <sstream>
#include "GameEngine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

GameEngine *GameEngine::s_instance = nullptr;

GameEngine &GameEngine::instance() {
	if (s_instance == nullptr) {
		throw std::runtime_error("Attempt to obtain GameEngine instance without active engine running");
	}
	return *s_instance;
}

GameEngine::GameEngine() {
	if (s_instance != nullptr) {
		throw std::runtime_error("Attempt to create more than one GameEngine instance");
	}
	s_instance = this;
}

GameEngine::~GameEngine() {
	if (m_transport) {
		m_transport->shutdown();
	}
	s_instance = nullptr;
}

bool GameEngine::init() {
	if (!platformInit()) {
		return false;
	}
	if (glewInit() != GLEW_OK) {
		log("Failed to initialize GLEW");
		return false;
	}
	
	m_commonShaderPrograms = std::make_unique<CommonShaderPrograms>();
	m_font = std::make_unique<BitmapFont>("assets/fonts/ter-u32n.png");
	m_debugTextRenderer = std::make_unique<BitmapFontRenderer>(*m_font);
	m_userInterface = std::make_unique<UserInterface>();

	m_voxelTypeRegistry = std::make_unique<VoxelTypeRegistry>();
	m_voxelWorld = std::make_unique<VoxelWorld>();
	m_voxelWorldRenderer = std::make_unique<VoxelWorldRenderer>(*m_voxelWorld);
	
	m_player = std::make_unique<Entity>(
			glm::vec3(1.0f, 1.0f, -1.0f),
			45.0f,
			0.0f,
			nullptr
	);
	
	m_cowTexture = std::make_unique<GLTexture>("assets/textures/cow.png");
	m_cowModel = std::make_unique<Model>("assets/models/cow.obj", commonShaderPrograms().texture, m_cowTexture.get());
	m_cowEntity = std::make_unique<Entity>(
			glm::vec3(2.0f, 0.0f, 0.0f),
			0.0f,
			0.0f,
			m_cowModel.get()
	);
	
	m_voxelTypeRegistry->add("grass", std::make_unique<SimpleVoxelType>("grass", "assets/textures/grass_top.png"));
	m_voxelTypeRegistry->add("dirt", std::make_unique<SimpleVoxelType>("dirt", "assets/textures/mud.png"));
	{
		auto &grass = m_voxelTypeRegistry->get("grass");
		auto chunk = m_voxelWorld->mutableChunk({0, 0, 0}, true);
		chunk.at(0, 0, 0).setType(grass);
		chunk.at(0, 1, 0).setType(grass);
		chunk.at(0, 0, 1).setType(grass);
	}
	{
		auto &grass = m_voxelTypeRegistry->get("grass");
		auto &dirt = m_voxelTypeRegistry->get("dirt");
		auto chunk = m_voxelWorld->mutableChunk({0, -1, 0}, true);
		for (int x = 0; x < VOXEL_CHUNK_SIZE; x++) {
			for (int z = 0; z < VOXEL_CHUNK_SIZE; z++) {
				chunk.at(x, VOXEL_CHUNK_SIZE - 1, z).setType(grass);
				chunk.at(x, VOXEL_CHUNK_SIZE - 2, z).setType(dirt);
				chunk.at(x, VOXEL_CHUNK_SIZE - 3, z).setType(dirt);
			}
		}
		
		chunk.at(3, VOXEL_CHUNK_SIZE - 3, 3).setType(m_voxelTypeRegistry->get("invalid"));
	}
	
	log("Game engine initialized");
	return true;
}

void GameEngine::quit() {
	m_running = false;
	log("Quitting...");
}

float GameEngine::viewportWidthOverHeight() const {
	return viewportHeight() > 0 ? (float) viewportWidth() / (float) viewportHeight() : 1.0f;
}

void GameEngine::handleResize(int width, int height) {
	m_viewportWidth = width;
	m_viewportHeight = height;
	log("Viewport set to %i x %i", width, height);
	
	glViewport(0, 0, m_viewportWidth, m_viewportHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	
	glDepthFunc(GL_LEQUAL);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	m_projection = glm::perspective(
			glm::radians(70.0f),
			viewportWidthOverHeight(),
			0.05f,
			100.0f
	);
}

void GameEngine::render() {
	updatePlayerPosition();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_DEPTH_TEST);
	
	const glm::vec3 playerDirection = m_player->direction(true);
	const glm::vec3 playerPosition = m_player->position();
	const glm::mat4 view = glm::lookAt(
			playerPosition,
			playerPosition + playerDirection,
			m_player->upDirection()
	);
	
	m_cowEntity->render(view, m_projection);
	m_voxelWorldRenderer->render(playerPosition, 2, view, m_projection);
	
	glDisable(GL_DEPTH_TEST);
	
	m_userInterface->render();
	if (m_showDebugInfo) {
		m_debugTextRenderer->render(-1.0f, 1.0f, 0.05f);
	}
	
	auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()
	);
	auto renderTime = now - m_lastRenderAt;
	m_lastRenderAt = now;
	m_framePerSecond = 1000.0f / renderTime.count();
	
	updateDebugInfo();
}

void GameEngine::updateDebugInfo() {
	std::stringstream ss;
	ss << "FPS: " << m_framePerSecond << "\n";
	ss << "X=" << m_player->position().x << ", Y=" << m_player->position().y << ", Z=" << m_player->position().z << "\n";
	ss << "Loaded " << m_voxelWorld->chunkCount() << " chunks";
	ss << " (" << m_voxelWorldRenderer->queueSize() << " chunk(s) in mesh build queue)\n";
	ss << "Used " << m_voxelWorldRenderer->usedBufferCount() << " voxel mesh buffers";
	ss << " (" << m_voxelWorldRenderer->availableBufferCount() << " available)\n";
	ss << "World render time (ms): " << m_voxelWorldRenderer->renderPerformanceCounter() << "\n";
	ss << "Chunk mesh build time (ms): " << m_voxelWorldRenderer->buildPerformanceCounter() << "\n";
	if (m_transport && m_transport->isConnected()) {
		ss << "Connected to the server";
	} else {
		ss << "!!! NOT CONNECTED TO THE SERVER !!!";
	}
	m_debugTextRenderer->setText(ss.str(), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void GameEngine::keyDown(KeyCode keyCode) {
	m_pressedKeys.insert(keyCode);
	switch (keyCode) {
		case KeyCode::TOGGLE_DEBUG_INFO:
			m_showDebugInfo = !m_showDebugInfo;
			break;
		case KeyCode::RESET_PERFORMANCE_COUNTERS:
			m_voxelWorldRenderer->renderPerformanceCounter().reset();
			m_voxelWorldRenderer->buildPerformanceCounter().reset();
			break;
		default:;
	}
}

void GameEngine::keyUp(KeyCode keyCode) {
	m_pressedKeys.erase(keyCode);
}

void GameEngine::updatePlayerDirection(float dx, float dy) {
	static const float sensitivity = 100.0f;
	m_player->adjustRotation(dx * sensitivity, dy * sensitivity);
}

void GameEngine::updatePlayerMovement(const float *dx, const float *dy, const float *dz) {
	if (dx) {
		m_playerSpeed.x = *dx;
	}
	if (dy) {
		m_playerSpeed.y = *dy;
	}
	if (dz) {
		m_playerSpeed.z = *dz;
	}
}

void GameEngine::updatePlayerPosition() {
	float SPEED = m_pressedKeys.count(KeyCode::SPEEDUP) ? 6.0f : 3.0f; // player moving distance per second
	
	auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()
	);
	float delta = (float) (now - m_lastPlayerPositionUpdateTime).count() / 1000.0f;
	m_lastPlayerPositionUpdateTime = now;
	
	glm::vec3 moveDirection = m_playerSpeed * SPEED * delta;
	if (m_pressedKeys.count(KeyCode::MOVE_FORWARD)) {
		moveDirection.z += SPEED * delta;
	}
	if (m_pressedKeys.count(KeyCode::MOVE_BACKWARD)) {
		moveDirection.z -= SPEED * delta;
	}
	if (m_pressedKeys.count(KeyCode::MOVE_LEFT)) {
		moveDirection.x -= SPEED * delta;
	}
	if (m_pressedKeys.count(KeyCode::MOVE_RIGHT)) {
		moveDirection.x += SPEED * delta;
	}
	if (m_pressedKeys.count(KeyCode::JUMP)) {
		moveDirection.y += SPEED * delta;
	}
	if (m_pressedKeys.count(KeyCode::CLIMB_DOWN)) {
		moveDirection.y -= SPEED * delta;
	}
	m_player->move(moveDirection);
	
	if (m_transport) {
		m_transport->updatePlayerPosition(m_player->position(), m_player->yaw(), m_player->pitch(), 2);
	}
}

void GameEngine::setPlayerPosition(const glm::vec3 &position) {
	m_player->setPosition(position);
}

void GameEngine::setTransport(std::unique_ptr<ClientTransport> transport) {
	if (m_transport) {
		m_transport->shutdown();
	}
	m_transport = std::move(transport);
	m_transport->start();
}

void GameEngine::log(const char *fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	logv(fmt, arg);
	va_end(arg);
}

void GameEngine::logv(const char *fmt, va_list arg) {
	va_list argCopy;
	va_copy(argCopy, arg);
	int count = vsnprintf(nullptr, 0, fmt, argCopy);
	va_end(argCopy);
	char *buffer = new char[count + 1];
	vsnprintf(buffer, count + 1, fmt, argCopy);
	fprintf(stdout, "%s\n", buffer);
	delete[] buffer;
}
