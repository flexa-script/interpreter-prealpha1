using cp.core.graphics;
// using cp.std.random;
// using cp.std.math;

var window1: cp::Window = cp::Window{};
var window2: cp::Window = cp::Window{};

cp::initialize(window1, "CP Graphics 1", 800, 450);
cp::initialize(window2, "CP Graphics 2", 400, 225);

while (not cp::is_quit(window1) or not cp::is_quit(window2)) {
	if (not cp::is_quit(window1)) {
		cp::clear_screen(window1, cp::Color{r=0,g=0,b=0});

		cp::draw_pixel(window1, 100, 50, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 101, 50, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 102, 50, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 103, 50, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 104, 50, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 105, 50, cp::Color{r=255,g=0,b=0});
		
		cp::draw_pixel(window1, 105, 51, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 105, 52, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 105, 53, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 105, 54, cp::Color{r=255,g=0,b=0});

		cp::draw_pixel(window1, 100, 55, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 101, 55, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 102, 55, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 103, 55, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 104, 55, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 105, 55, cp::Color{r=255,g=0,b=0});
		
		cp::draw_pixel(window1, 100, 51, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 100, 52, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 100, 53, cp::Color{r=255,g=0,b=0});
		cp::draw_pixel(window1, 100, 54, cp::Color{r=255,g=0,b=0});

		cp::draw_line(window1, 90, 40, 600, 80, cp::Color{r=255,g=0,b=0});

		cp::draw_rect(window1, 500, 200, 100, 80, cp::Color{r=255,g=0,b=0});
		cp::fill_rect(window1, 200, 200, 100, 80, cp::Color{r=255,g=0,b=0});

		cp::update(window1);
	}
	else {
		cp::destroy(window1);
	}
	if (not cp::is_quit(window2)) {
		cp::clear_screen(window2, cp::Color{r=0,g=0,b=0});
		cp::draw_pixel(window2, 100, 50, cp::Color{r=255,g=0,b=0});
		cp::update(window2);
	}
	else {
		cp::destroy(window2);
	}
}

cp::destroy(window1);
cp::destroy(window2);

print("quit");
