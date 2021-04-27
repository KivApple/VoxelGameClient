#include "SDLGameEngine.h"
#include "net/WebSocketClientTransport.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s ws://localhost:9002/\n", argv[0]);
		return 0;
	}
	
	SDLGameEngine engine;
	engine.setTransport(std::make_unique<WebSocketClientTransport>(engine, argv[1]));
	return engine.init() ? engine.run() : 1;
}
