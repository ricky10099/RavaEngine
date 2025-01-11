#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/Log.h"
#include "GameScenes/ExampleScene/ExampleScene.h"

int main() {
	Rava::Engine engine{};
	engine.LoadScene(std::make_shared<ExampleScene>("Example scene"));

	try {
		engine.Run();
	} catch (const std::exception& e) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}