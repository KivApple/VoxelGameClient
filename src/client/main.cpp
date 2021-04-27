#include "SDLGameEngine.h"
#include "net/WebSocketClientTransport.h"
#include <easylogging++.h>

INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[]) {
	START_EASYLOGGINGPP(argc, argv);
	{
		el::Configurations conf;
		conf.setGlobally(
				el::ConfigurationType::Format,
				"%datetime{%Y-%M-%d %H:%m:%s.%g} [%level] [%logger] %msg"
		);
		el::Loggers::setDefaultConfigurations(conf, true);
	}
	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s ws://localhost:9002/\n", argv[0]);
		return 0;
	}
	
	SDLGameEngine engine;
	if (!engine.init()) {
		return 1;
	}
	engine.setTransport(std::make_unique<WebSocketClientTransport>(engine, argv[1]));
	return engine.run();
}
