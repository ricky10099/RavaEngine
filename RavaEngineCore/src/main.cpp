#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/Log.h"

int main() {
	Rava::Engine engine{};

	try {
		engine.Run();
	} catch (const std::exception& e) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}