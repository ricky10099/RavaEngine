#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/Log.h"
#include "GameScenes/ExampleScene/ExampleScene.h"

int main() {
	Rava::Engine engine{};
	engine.LoadScene(std::make_unique<ExampleScene>());

	try {
		engine.Run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}