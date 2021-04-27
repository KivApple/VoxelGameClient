#include <easylogging++.h>
#include <gtest/gtest.h>

INITIALIZE_EASYLOGGINGPP

extern "C" void __cxa_pure_virtual() {
	fprintf(stderr, "pure virtual method called\n");
	abort();
}

int main(int argc, char **argv) {
	START_EASYLOGGINGPP(argc, argv);
	{
		el::Configurations conf;
		conf.setGlobally(
				el::ConfigurationType::Format,
				"%datetime{%Y-%M-%d %H:%m:%s.%g} [%level] [%logger] %msg"
		);
		el::Loggers::setDefaultConfigurations(conf, true);
	}
	
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
