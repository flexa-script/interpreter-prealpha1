#ifndef CPCGRAPHICS_HPP
#define CPCGRAPHICS_HPP

#include "vendor/graphics.hpp"

class Graphics {
	Graphics() {
		axe::Graphics engine;

		if (engine.initialize(L"Graphics", 800, 600)) {
			while (!engine.is_quit()) {
				engine.clear_screen(RGB(0, 0, 0));

				engine.draw_pixel(100, 100, RGB(255, 0, 0));
				engine.draw_line(110, 110, 210, 210, RGB(255, 0, 0));

				engine.draw_rect(120, 50, 100, 100, RGB(0, 255, 0));
				engine.fill_rect(600, 350, 100, 100, RGB(0, 255, 0));

				engine.draw_circle(400, 350, 100, RGB(0, 0, 255));
				engine.fill_circle(600, 100, 100, RGB(0, 0, 255));

				engine.update();
			}
		}
	}

};

#endif // CPCGRAPHICS_HPP
