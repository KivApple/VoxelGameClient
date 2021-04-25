#include "SDLGameEngine.h"

int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	
	SDLGameEngine engine;
	return engine.init() ? engine.run() : 1;
}
