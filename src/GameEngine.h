#pragma once

#include <cstdarg>
#include <chrono>
#include <memory>
#include <unordered_set>
#include "ShaderProgram.h"
#include "TextRenderer.h"
#include "ui/UIRoot.h"
#include "world/VoxelWorld.h"
#include "world/VoxelWorldRenderer.h"
#include "world/Entity.h"

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
	
	PRIMARY_CLICK,
	SECONDARY_CLICK
};

class GameEngine {
	static GameEngine *s_instance;
	bool m_running = true;
	int m_viewportWidth = 0;
	int m_viewportHeight = 0;
	std::chrono::milliseconds m_lastRenderAt = std::chrono::milliseconds(0);
	float m_framePerSecond = 0;
	std::unique_ptr<CommonShaderPrograms> m_commonShaderPrograms;
	std::unique_ptr<BitmapFont> m_font;
	std::unique_ptr<TextRenderer> m_debugTextRenderer;
	bool m_showDebugInfo = true;
	std::unordered_set<KeyCode> m_pressedKeys;
	std::unique_ptr<UserInterface> m_userInterface;
	std::unique_ptr<SimpleVoxelType> m_grassVoxelType;
	std::unique_ptr<SimpleVoxelType> m_dirtVoxelType;
	std::unique_ptr<VoxelWorld> m_voxelWorld;
	std::unique_ptr<VoxelWorldRenderer> m_voxelWorldRenderer;
	glm::mat4 m_projection = glm::mat4(1.0f);
	std::unique_ptr<Entity> m_player;
	std::chrono::milliseconds m_lastPlayerPositionUpdateTime = std::chrono::milliseconds(0);
	glm::vec3 m_playerSpeed = glm::vec3(0.0f);
	
	std::unique_ptr<GLTexture> m_cowTexture;
	std::unique_ptr<Model> m_cowModel;
	std::unique_ptr<Entity> m_cowEntity;

protected:
	virtual bool platformInit() = 0;
	void handleResize(int width, int height);
	void render();
	
public:
	static GameEngine &instance();
	GameEngine();
	GameEngine(const GameEngine&) = delete;
	GameEngine &operator=(const GameEngine&) = delete;
	~GameEngine();
	
	bool init();
	virtual int run() = 0;
	[[nodiscard]] bool isRunning() const {
		return m_running;
	}
	void quit();
	
	void keyDown(KeyCode keyCode);
	void keyUp(KeyCode keyCode);
	void updatePlayerDirection(float dx, float dy);
	void updatePlayerMovement(const float *dx, const float *dy, const float *dz);
	void updatePlayerPosition();
	void updateDebugInfo();
	
	[[nodiscard]] int viewportWidth() const {
		return m_viewportWidth;
	}
	[[nodiscard]] int viewportHeight() const {
		return m_viewportHeight;
	}
	[[nodiscard]] float viewportWidthOverHeight() const;
	[[nodiscard]] const CommonShaderPrograms &commonShaderPrograms() const {
		return *m_commonShaderPrograms;
	}
	[[nodiscard]] UserInterface &userInterface() {
		return *m_userInterface;
	}
	
	virtual void log(const char *fmt, ...);
	virtual void logv(const char *fmt, va_list arg);
	
};
