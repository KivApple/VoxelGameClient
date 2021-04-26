#include <gtest/gtest.h>

extern "C" void __cxa_pure_virtual() {
	fprintf(stderr, "pure virtual method called\n");
	abort();
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
