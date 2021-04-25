#include <cstdio>
#include <csignal>
#include "GameServerEngine.h"
#include "net/WebSocketServerTransport.h"

static GameServerEngine *engineInstance = nullptr;

static void sigIntHandler(int) {
	if (engineInstance) {
		fprintf(stderr, "Shutdown initiated\n");
		engineInstance->shutdown();
		engineInstance = nullptr;
	} else {
		exit(0);
	}
}

static void setupSigIntHandler() {
	signal(SIGINT, sigIntHandler);
}

int main(int argc, char *argv[]) {
	GameServerEngine engine;
	engineInstance = &engine;
	setupSigIntHandler();
	engine.addTransport(std::make_unique<WebSocketServerTransport>(9002));
	auto retVal = engine.run();
	engineInstance = nullptr;
	return retVal;
}
