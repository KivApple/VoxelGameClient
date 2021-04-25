#pragma once

#if __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <unordered_set>
#include <unordered_map>
#include <SDL2/SDL.h>
#include "GameEngine.h"

class SDLGameEngine: public GameEngine {
	SDL_Window *m_window = nullptr;
	SDL_GLContext m_glContext = nullptr;
	bool m_isFullscreen = false;
	std::unordered_map<SDL_Keycode, KeyCode> m_keyMap;
	bool m_userInterfaceMouseLock = false;
	std::unordered_set<SDL_FingerID> m_userInterfaceFingerLocks;
	bool m_touchRotationStarted = false;
	SDL_FingerID m_rotationFingerId = 0;
	
	void handleWindowEvent(const SDL_WindowEvent &event);
	void handleKeyDownEvent(const SDL_KeyboardEvent &event);
	void handleKeyUpEvent(const SDL_KeyboardEvent &event);
	void handleMouseMotionEvent(SDL_MouseMotionEvent &event);
	void handleMouseButtonDownEvent(SDL_MouseButtonEvent &event);
	void handleMouseButtonUpEvent(SDL_MouseButtonEvent &event);
	void handleTouchFingerDownEvent(SDL_TouchFingerEvent &event);
	void handleTouchFingerMotionEvent(SDL_TouchFingerEvent &event);
	void handleTouchFingerUpEvent(SDL_TouchFingerEvent &event);
	void handleEvents();
	
protected:
	bool platformInit() override;

public:
	SDLGameEngine();
	int run() override;
	
};
