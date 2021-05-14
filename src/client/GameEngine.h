#pragma once

#include <chrono>
#include <memory>
#include <unordered_set>
#include "Asset.h"
#include "ShaderProgram.h"
#include "client/ui/TextRenderer.h"
#include "ui/UIRoot.h"
#include "ui/VoxelOutline.h"
#include "world/VoxelTypeRegistry.h"
#include "world/VoxelWorld.h"
#include "client/world/VoxelWorldRenderer.h"
#include "world/Entity.h"
#include "client/net/ClientTransport.h"
#include "OpenGL.h"

enum class KeyCode {
	MOVE_FORWARD,
	MOVE_BACKWARD,
	MOVE_LEFT,
	MOVE_RIGHT,
	JUMP,
	CLIMB_DOWN,
	SPEEDUP,
	TOGGLE_DEBUG_INFO,
	RESET_PERFORMANCE_COUNTERS,
	INVENTORY_1,
	INVENTORY_2,
	INVENTORY_3,
	INVENTORY_4,
	INVENTORY_5,
	INVENTORY_6,
	INVENTORY_7,
	INVENTORY_8,
	
	PRIMARY_CLICK,
	SECONDARY_CLICK
};

class GameEngine: VoxelChunkListener {
	static GameEngine *s_instance;
	bool m_running = true;
	int m_viewportWidth = 0;
	int m_viewportHeight = 0;
	std::chrono::milliseconds m_lastRenderAt = std::chrono::milliseconds(0);
	float m_framePerSecond = 0;
	std::unique_ptr<AssetLoader> m_assetLoader;
	std::unique_ptr<CommonShaderPrograms> m_commonShaderPrograms;
	std::unique_ptr<BitmapFont> m_font;
	std::unique_ptr<TextRenderer> m_debugTextRenderer;
	bool m_showDebugInfo = true;
	std::unordered_set<KeyCode> m_pressedKeys;
	std::unique_ptr<VoxelTypeRegistry> m_voxelTypeRegistry;
	std::unique_ptr<VoxelWorld> m_voxelWorld;
	std::unique_ptr<VoxelWorldRenderer> m_voxelWorldRenderer;
	std::unique_ptr<VoxelOutline> m_voxelOutline;
	std::unique_ptr<UserInterface> m_userInterface;
	glm::mat4 m_projection = glm::mat4(1.0f);
	std::unique_ptr<Entity> m_player;
	std::chrono::milliseconds m_lastPlayerPositionUpdateTime = std::chrono::milliseconds(0);
	glm::vec3 m_playerSpeed = glm::vec3(0.0f);
	std::string m_debugStr;
	bool m_mouseClicked = false;
	bool m_mouseSecondaryClicked = false;
	
	std::unique_ptr<GL::Texture> m_cowTexture;
	std::unique_ptr<Model> m_cowModel;
	std::unique_ptr<Entity> m_cowEntity;
	
	std::unique_ptr<ClientTransport> m_transport;
	
	void chunkUnlocked(const VoxelChunkLocation &chunkLocation, VoxelChunkLightState lightState) override;

protected:
	virtual bool platformInit() = 0;
	virtual std::string prefix() = 0;
	void handleResize(int width, int height);
	void render();
	
public:
	static GameEngine &instance();
	GameEngine();
	GameEngine(const GameEngine&) = delete;
	GameEngine &operator=(const GameEngine&) = delete;
	~GameEngine() override;
	
	bool init();
	virtual int run() = 0;
	[[nodiscard]] bool isRunning() const {
		return m_running;
	}
	void quit();
	
	void keyDown(KeyCode keyCode);
	void keyUp(KeyCode keyCode);
	void mouseWheel(int delta);
	void updatePlayerDirection(float dx, float dy);
	void updatePlayerMovement(const float *dx, const float *dy, const float *dz);
	void updatePlayerPosition();
	void updatePointingAt(const glm::mat4 &view);
	void setPlayerPosition(const glm::vec3 &position);
	void updateDebugInfo();
	
	[[nodiscard]] int viewportWidth() const {
		return m_viewportWidth;
	}
	[[nodiscard]] int viewportHeight() const {
		return m_viewportHeight;
	}
	[[nodiscard]] float viewportWidthOverHeight() const;
	[[nodiscard]] AssetLoader &assetLoader() const {
		return *m_assetLoader;
	}
	[[nodiscard]] const CommonShaderPrograms &commonShaderPrograms() const {
		return *m_commonShaderPrograms;
	}
	[[nodiscard]] UserInterface &userInterface() {
		return *m_userInterface;
	}
	[[nodiscard]] VoxelTypeRegistry &voxelTypeRegistry() {
		return *m_voxelTypeRegistry;
	}
	[[nodiscard]] VoxelWorld &voxelWorld() {
		return *m_voxelWorld;
	}
	[[nodiscard]] VoxelWorldRenderer &voxelWorldRenderer() {
		return *m_voxelWorldRenderer;
	}
	std::string &debugStr() {
		return m_debugStr;
	}
	
	void setTransport(std::unique_ptr<ClientTransport> transport);
	
};
