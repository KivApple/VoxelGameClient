#include <stdexcept>
#include <sstream>
#include <easylogging++.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <GL/glew.h>
#include "GameEngine.h"
#include "world/VoxelTypes.h"

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
		LOG(ERROR) << "Failed to initialize GLEW";
		return false;
	}
	
	m_commonShaderPrograms = std::make_unique<CommonShaderPrograms>();
	m_font = std::make_unique<BitmapFont>("assets/fonts/ter-u32n.png");
	m_debugTextRenderer = std::make_unique<BitmapFontRenderer>(*m_font);
	m_userInterface = std::make_unique<UserInterface>();

	m_voxelTypeRegistry = std::make_unique<VoxelTypeRegistry>();
	m_voxelWorld = std::make_unique<VoxelWorld>(static_cast<VoxelChunkListener*>(this));
	m_voxelWorldRenderer = std::make_unique<VoxelWorldRenderer>(*m_voxelWorld);
	m_voxelOutline = std::make_unique<VoxelOutline>();
	
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
	
	m_voxelTypeRegistry->make<AirVoxelType>("air");
	m_voxelTypeRegistry->make<SimpleVoxelType>("grass", "grass", "assets/textures/grass.png", true);
	m_voxelTypeRegistry->make<SimpleVoxelType>("dirt", "dirt", "assets/textures/mud.png");
	m_voxelTypeRegistry->make<SimpleVoxelType>("stone", "stone", "assets/textures/stone.png");
	
	LOG(INFO) << "Game engine initialized";
	return true;
}

void GameEngine::quit() {
	m_running = false;
	LOG(INFO) << "Quitting...";
}

float GameEngine::viewportWidthOverHeight() const {
	return viewportHeight() > 0 ? (float) viewportWidth() / (float) viewportHeight() : 1.0f;
}

void GameEngine::handleResize(int width, int height) {
	m_viewportWidth = width;
	m_viewportHeight = height;
	LOG(INFO) << "Viewport set to " << width << "x" << height;
	
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
	updatePointingAt(view);
	m_voxelOutline->render(view, m_projection);
	
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

void GameEngine::updatePointingAt(const glm::mat4 &view) {
	m_debugStr.clear();
	auto playerPosition = m_player->position();
	glm::vec3 playerDirection = m_player->direction(true);
	VoxelChunkRef chunk;
	VoxelLocation prevLocation;
	for (float r = 0.0f; r < 4.0f; r += 0.1f) {
		auto position = playerPosition + playerDirection * r;
		VoxelLocation location((int) roundf(position.x), (int) roundf(position.y), (int) roundf(position.z));
		if (r == 0.0f || location != prevLocation) {
			prevLocation = location;
		} else {
			continue;
		}
		
		auto chunkLocation = location.chunk();
		if (!chunk || chunk.location() != chunkLocation) {
			chunk = m_voxelWorld->chunk(chunkLocation);
		}
		if (!chunk) {
			break;
		}

		auto &voxel = chunk.at(location.inChunk());
		if (m_voxelOutline->set(voxel, location, playerPosition, playerDirection)) {
			break;
		}
	}
	if (chunk) {
		chunk.unlock();
	}
}

void GameEngine::updateDebugInfo() {
	std::stringstream ss;
	ss << "FPS: " << m_framePerSecond << "\n";
	ss << "X=" << m_player->position().x << ", Y=" << m_player->position().y << ", Z=" << m_player->position().z;
	VoxelLocation playerLocation(
			(int) roundf(m_player->position().x),
			(int) roundf(m_player->position().y),
			(int) roundf(m_player->position().z)
	);
	auto playerChunkLocation = playerLocation.chunk();
	VoxelLightLevel lightLevel = MAX_VOXEL_LIGHT_LEVEL;
	{
		auto chunk = m_voxelWorld->chunk(playerChunkLocation);
		if (chunk) {
			lightLevel = chunk.at(playerLocation.inChunk()).lightLevel();
		}
	}
	ss << " (chunk X=" << playerChunkLocation.x << ", Y=" << playerChunkLocation.y <<
			", Z=" << playerChunkLocation.z << ") lightLevel=" << (int) lightLevel;
	ss << " yaw=" << m_player->yaw() << ", pitch=" << m_player->pitch() << "\n";
	if (m_voxelOutline->voxelDetected()) {
		auto &l = m_voxelOutline->voxelLocation();
		ss << "Pointing at X=" << l.x << ",Y=" << l.y << ",Z=" << l.z << ": " << m_voxelOutline->text();
		auto &d = m_voxelOutline->direction();
		ss << " (direction X=" << d.x << ",Y=" << d.y << ",Z=" << d.z << ")\n";
	}
	ss << "Loaded " << m_voxelWorld->chunkCount() << " chunks";
	ss << " (" << m_voxelWorldRenderer->queueSize() << " chunk(s) in mesh build queue)\n";
	ss << "Used " << m_voxelWorldRenderer->usedBufferCount() << " voxel mesh buffers";
	ss << " (" << m_voxelWorldRenderer->availableBufferCount() << " available)\n";
	ss << "World render time (ms): " << m_voxelWorldRenderer->renderPerformanceCounter() << "\n";
	ss << "Chunk mesh build time (ms): " << m_voxelWorldRenderer->buildPerformanceCounter() << "\n";
	if (m_transport && m_transport->isConnected()) {
		ss << "Connected to the server\n";
	} else {
		ss << "!!! NOT CONNECTED TO THE SERVER !!!\n";
	}
	if (!m_debugStr.empty()) {
		ss << m_debugStr;
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
			m_voxelWorldRenderer->reset();
			break;
		case KeyCode::PRIMARY_CLICK:
			if (m_mouseClicked) break;
			m_mouseClicked = true;
			if (!m_voxelOutline->voxelDetected()) break;
			if (!m_transport) break;
			m_transport->digVoxel(m_voxelOutline->voxelLocation());
			break;
		case KeyCode::SECONDARY_CLICK:
			if (m_mouseSecondaryClicked) break;
			m_mouseSecondaryClicked = true;
			if (!m_voxelOutline->voxelDetected()) break;
			if (!m_transport) break;
			m_transport->placeVoxel(VoxelLocation(
				m_voxelOutline->voxelLocation().x + (int) m_voxelOutline->direction().x,
				m_voxelOutline->voxelLocation().y + (int) m_voxelOutline->direction().y,
				m_voxelOutline->voxelLocation().z + (int) m_voxelOutline->direction().z
			));
			break;
		default:;
	}
}

void GameEngine::keyUp(KeyCode keyCode) {
	m_pressedKeys.erase(keyCode);
	switch (keyCode) {
		case KeyCode::PRIMARY_CLICK:
			m_mouseClicked = false;
			break;
		case KeyCode::SECONDARY_CLICK:
			m_mouseSecondaryClicked = false;
			break;
		default:;
	}
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

void GameEngine::chunkInvalidated(
		const VoxelChunkLocation &chunkLocation,
		std::vector<InChunkVoxelLocation> &&locations,
		bool lightComputed
) {
	m_voxelWorldRenderer->invalidate(chunkLocation);
	m_voxelWorldRenderer->invalidate({chunkLocation.x - 1, chunkLocation.y, chunkLocation.z});
	m_voxelWorldRenderer->invalidate({chunkLocation.x, chunkLocation.y - 1, chunkLocation.z});
	m_voxelWorldRenderer->invalidate({chunkLocation.x, chunkLocation.y, chunkLocation.z - 1});
	m_voxelWorldRenderer->invalidate({chunkLocation.x + 1, chunkLocation.y, chunkLocation.z});
	m_voxelWorldRenderer->invalidate({chunkLocation.x, chunkLocation.y + 1, chunkLocation.z});
	m_voxelWorldRenderer->invalidate({chunkLocation.x, chunkLocation.y, chunkLocation.z + 1});
}
