#include <csignal>
#include <exception>
#include "net/WebSocketServerTransport.h"
#include "GameServerEngine.h"

INITIALIZE_EASYLOGGINGPP

static GameServerEngine *engineInstance = nullptr;

static void sigIntHandler(int) {
	if (engineInstance) {
		LOG(INFO) << "Shutdown initiated";
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
	START_EASYLOGGINGPP(argc, argv);
	{
		el::Configurations conf;
		conf.setGlobally(
				el::ConfigurationType::Format,
				"%datetime{%Y-%M-%d %H:%m:%s.%g} [%level] [%logger] [%thread] %msg"
		);
		el::Loggers::setDefaultConfigurations(conf, true);
	}
#ifdef ELPP_FEATURE_CRASH_LOG
	std::set_terminate([]() {
		LOG(ERROR) << "std::terminate() called";
		el::Helpers::logCrashReason(SIGABRT, true);
		el::Helpers::crashAbort(SIGABRT);
	});
#endif
	
	GameServerEngine engine;
	engineInstance = &engine;
	setupSigIntHandler();
	engine.addTransport(std::make_unique<WebSocketServerTransport>(9002));
	auto retVal = engine.run();
	engineInstance = nullptr;
	return retVal;
}
