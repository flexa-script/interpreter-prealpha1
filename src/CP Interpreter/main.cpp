//#include "util.hpp"
//#include "cprepl.hpp"
//#include "cpinterpreter.hpp"
////#include "logging.hpp"
//#include "watch.h"
#include "graphics.hpp"

class MyEngine : public SimpleGraphicsEngine {
protected:
	void OnInitialize() override {
		// Custom initialization
	}

	void OnUpdate() override {
		// Custom update logic
	}

	void OnDraw() override {
		ClearScreen(RGB(0, 0, 0));  // Clear with black
		DrawPixel(100, 100, RGB(255, 0, 0));  // Draw red pixel
	}
};

int main(int argc, const char* argv[]) {

	MyEngine engine;

	if (engine.Initialize(L"My Simple Graphics Engine", 800, 600)) {
		engine.Run();
	}

	//SimpleGraphicsEngine graph = SimpleGraphicsEngine();
	//graph.Initialize(L"My Simple Graphics Engine", 800, 600);
	//graph.ClearScreen(RGB(0, 0, 0));  // Clear with black
	//graph.DrawPixel(100, 100, RGB(255, 0, 0));  // Draw red pixel

	return 0;

	//auto log = axe::Logger(axe::DEBUG, ".\\data.log");

	//for (int i = 0; i < argc; ++i) {
	//	log.debug(argv[i]);
	//}

	//auto sw = ChronoStopwatch();
	//sw.start();

	int result = 0;

	//if (argc == 1) {
	//	return CPRepl::execute();
	//}

	//auto interpreter = CPInterpreter();
	//result = interpreter.execute(argc, argv);

	//sw.stop();
	//std::cout << std::endl << "execution time: " << sw.get_elapsed_formatted() << std::endl;
	//system("pause");
	return result;
}
