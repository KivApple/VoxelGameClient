#include <easylogging++.h>
#include "SDLGameEngine.h"

#ifdef __EMSCRIPTEN__
EM_JS(void, emscripten_reloadPage, (), {
	window.location.reload();
});

extern "C" void SDLGameEngine_pointerLockChanged(bool locked) {
	if (SDL_GetRelativeMouseMode() != (locked ? SDL_TRUE : SDL_FALSE)) {
		SDL_SetRelativeMouseMode(locked ? SDL_TRUE : SDL_FALSE);
	}
}
#endif

SDLGameEngine::SDLGameEngine(): m_keyMap({
		{SDLK_UP, KeyCode::MOVE_FORWARD},
		{SDLK_w, KeyCode::MOVE_FORWARD},
		{SDLK_DOWN, KeyCode::MOVE_BACKWARD},
		{SDLK_s, KeyCode::MOVE_BACKWARD},
		{SDLK_LEFT, KeyCode::MOVE_LEFT},
		{SDLK_a, KeyCode::MOVE_LEFT},
		{SDLK_RIGHT, KeyCode::MOVE_RIGHT},
		{SDLK_d, KeyCode::MOVE_RIGHT},
		{SDLK_SPACE, KeyCode::JUMP},
		{SDLK_LSHIFT, KeyCode::CLIMB_DOWN},
		{SDLK_RSHIFT, KeyCode::CLIMB_DOWN},
		{SDLK_F1, KeyCode::TOGGLE_DEBUG_INFO},
		{SDLK_F2, KeyCode::RESET_PERFORMANCE_COUNTERS},
		{SDLK_LCTRL, KeyCode::SPEEDUP},
		{SDLK_RCTRL, KeyCode::SPEEDUP},
		{SDLK_1, KeyCode::INVENTORY_1},
		{SDLK_2, KeyCode::INVENTORY_2},
		{SDLK_3, KeyCode::INVENTORY_3},
		{SDLK_4, KeyCode::INVENTORY_4},
		{SDLK_5, KeyCode::INVENTORY_5},
		{SDLK_6, KeyCode::INVENTORY_6},
		{SDLK_7, KeyCode::INVENTORY_7},
		{SDLK_8, KeyCode::INVENTORY_8}
}) {
}

bool SDLGameEngine::platformInit() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		LOG(ERROR) << "Failed to initialize SDL: " << SDL_GetError();
		return false;
	}
	if (!SDL_SetHint("SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH", "1")) {
		LOG(WARNING) << "SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH) failed";
	}
	m_window = SDL_CreateWindow(
			"VoxelGame",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			640, 480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_SHOWN
#ifndef __EMSCRIPTEN__
			| SDL_WINDOW_ALLOW_HIGHDPI
#endif
	);
	if (m_window == nullptr) {
		LOG(ERROR) << "Failed to create SDL window: " << SDL_GetError();
		return false;
	}
	
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
	SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_ES | SDL_GL_CONTEXT_PROFILE_COMPATIBILITY
	);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	m_glContext = SDL_GL_CreateContext(m_window);
	if (m_glContext == nullptr) {
		LOG(ERROR) << "Failed to create OpenGL context: " << SDL_GetError();
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
		return false;
	}
	SDL_GL_SetSwapInterval(1);
	
	/* int width, height;
	SDL_GL_GetDrawableSize(m_window, &width, &height);
	handleResize(width, height); */
	return true;
}

std::string SDLGameEngine::prefix() {
	auto path = SDL_GetBasePath();
	std::string pathStr(path);
	SDL_free(path);
	return pathStr;
}

void SDLGameEngine::handleWindowEvent(const SDL_WindowEvent &event) {
	switch (event.event) {
		case SDL_WINDOWEVENT_RESIZED:
			handleResize(event.data1, event.data2);
			break;
	}
}

void SDLGameEngine::handleKeyDownEvent(const SDL_KeyboardEvent &event) {
	auto it = m_keyMap.find(event.keysym.sym);
	if (it != m_keyMap.end()) {
		keyDown(it->second);
	}
	
	switch (event.keysym.sym) {
		case SDLK_ESCAPE:
			SDL_SetRelativeMouseMode(SDL_FALSE);
			break;
		case SDLK_F11:
			m_isFullscreen = !m_isFullscreen;
			SDL_SetWindowFullscreen(m_window, m_isFullscreen);
			break;
#ifdef __EMSCRIPTEN__
		case SDLK_F5:
			emscripten_reloadPage();
			break;
#endif
	}
}

void SDLGameEngine::handleKeyUpEvent(const SDL_KeyboardEvent &event) {
	auto it = m_keyMap.find(event.keysym.sym);
	if (it != m_keyMap.end()) {
		keyUp(it->second);
	}
}

void SDLGameEngine::handleMouseMotionEvent(SDL_MouseMotionEvent &event) {
	if (event.which == SDL_TOUCH_MOUSEID) return;
	if (!viewportWidth() || !viewportHeight()) return;
	if (m_userInterfaceMouseLock) {
		userInterface().mouseDrag(glm::vec2(
				(float) event.x / (float) viewportWidth() * 2.0f - 1.0f,
				(float) -event.y / (float) viewportHeight() * 2.0f + 1.0f
		));
		return;
	}
	if (!SDL_GetRelativeMouseMode()) {
		userInterface().mouseMove(glm::vec2(
				(float) event.x / (float) viewportWidth() * 2.0f - 1.0f,
				(float) -event.y / (float) viewportHeight() * 2.0f + 1.0f
		));
		return;
	}
	updatePlayerDirection(
			(float) event.xrel / (float) viewportWidth(),
			(float) -event.yrel / (float) viewportHeight()
	);
}

void SDLGameEngine::handleMouseButtonDownEvent(SDL_MouseButtonEvent &event) {
	if (event.which == SDL_TOUCH_MOUSEID) return;
	if (!viewportWidth() || !viewportHeight()) return;
	if (userInterface().mouseDown(glm::vec2(
			(float) event.x / (float) viewportWidth() * 2.0f - 1.0f,
			(float) -event.y / (float) viewportHeight() * 2.0f + 1.0f
	))) {
		m_userInterfaceMouseLock = true;
		return;
	}
	if (!SDL_GetRelativeMouseMode()) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
		return;
	}
	switch (event.button) {
		case SDL_BUTTON_LEFT:
			keyDown(KeyCode::PRIMARY_CLICK);
			break;
		case SDL_BUTTON_RIGHT:
			keyDown(KeyCode::SECONDARY_CLICK);
			break;
	}
}

void SDLGameEngine::handleMouseButtonUpEvent(SDL_MouseButtonEvent &event) {
	if (event.which == SDL_TOUCH_MOUSEID) return;
	if (!viewportWidth() || !viewportHeight()) return;
	if (m_userInterfaceMouseLock) {
		m_userInterfaceMouseLock = false;
		userInterface().mouseUp(glm::vec2(
				(float) event.x / (float) viewportWidth() * 2.0f - 1.0f,
				(float) -event.y / (float) viewportHeight() * 2.0f + 1.0f
		));
		return;
	}
	if (!SDL_GetRelativeMouseMode()) {
		return;
	}
	switch (event.button) {
		case SDL_BUTTON_LEFT:
			keyUp(KeyCode::PRIMARY_CLICK);
			break;
		case SDL_BUTTON_RIGHT:
			keyUp(KeyCode::SECONDARY_CLICK);
			break;
	}
}

void SDLGameEngine::handleMouseWheelEvent(SDL_MouseWheelEvent &event) {
	mouseWheel(event.y);
}

void SDLGameEngine::handleTouchFingerDownEvent(SDL_TouchFingerEvent &event) {
	if (userInterface().touchStart(event.fingerId, glm::vec2(
			(float) event.x * 2.0f - 1.0f,
			(float) -event.y * 2.0f + 1.0f
	))) {
		m_userInterfaceFingerLocks.insert(event.fingerId);
		return;
	}
	if (!m_touchRotationStarted) {
		m_touchRotationStarted = true;
		m_rotationFingerId = event.fingerId;
	}
}

void SDLGameEngine::handleTouchFingerMotionEvent(SDL_TouchFingerEvent &event) {
	if (m_userInterfaceFingerLocks.count(event.fingerId)) {
		userInterface().touchMotion(event.fingerId, glm::vec2(
				(float) event.x * 2.0f - 1.0f,
				(float) -event.y * 2.0f + 1.0f
		));
		return;
	}
	if (m_touchRotationStarted && m_rotationFingerId == event.fingerId) {
		updatePlayerDirection(-event.dx, event.dy);
	}
}

void SDLGameEngine::handleTouchFingerUpEvent(SDL_TouchFingerEvent &event) {
	auto it = m_userInterfaceFingerLocks.find(event.fingerId);
	if (it != m_userInterfaceFingerLocks.end()) {
		m_userInterfaceFingerLocks.erase(it);
		userInterface().touchEnd(event.fingerId, glm::vec2(
				(float) event.x * 2.0f - 1.0f,
				(float) -event.y * 2.0f + 1.0f
		));
		return;
	}
	if (m_touchRotationStarted && m_rotationFingerId == event.fingerId) {
		m_touchRotationStarted = false;
	}
}

void SDLGameEngine::handleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				quit();
				break;
			case SDL_WINDOWEVENT:
				handleWindowEvent(event.window);
				break;
			case SDL_KEYDOWN:
				handleKeyDownEvent(event.key);
				break;
			case SDL_KEYUP:
				handleKeyUpEvent(event.key);
				break;
			case SDL_MOUSEMOTION:
				handleMouseMotionEvent(event.motion);
				break;
			case SDL_MOUSEBUTTONDOWN:
				handleMouseButtonDownEvent(event.button);
				break;
			case SDL_MOUSEBUTTONUP:
				handleMouseButtonUpEvent(event.button);
				break;
			case SDL_MOUSEWHEEL:
				handleMouseWheelEvent(event.wheel);
				break;
			case SDL_FINGERDOWN:
				handleTouchFingerDownEvent(event.tfinger);
				break;
			case SDL_FINGERMOTION:
				handleTouchFingerMotionEvent(event.tfinger);
				break;
			case SDL_FINGERUP:
				handleTouchFingerUpEvent(event.tfinger);
				break;
		}
	}
	render();
	SDL_GL_SwapWindow(m_window);
#ifdef __EMSCRIPTEN__
	if (!isRunning()) {
		emscripten_cancel_main_loop();
	}
#endif
}

int SDLGameEngine::run() {
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg([](void *arg) {
		((SDLGameEngine*) arg)->handleEvents();
	}, this, 0, 1);
#else
	while (isRunning()) {
		handleEvents();
	}
#endif
	return 0;
}
