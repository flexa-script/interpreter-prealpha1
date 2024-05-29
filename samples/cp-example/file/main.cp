using cp.core.graphics;
// using cp.std.random;
// using cp.std.math;

var window: cp::Window = cp::Window{};

cp::initialize(window, "CP Graphics", 800, 450);

while(not cp::is_quit(window)) {
    cp::clear_screen(window, cp::Color{r=0,g=0,b=0});

	cp::draw_pixel(window, 100, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 101, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 102, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 103, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 104, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 105, 50, cp::Color{r=255,g=0,b=0});
	
	cp::draw_pixel(window, 105, 51, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 105, 52, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 105, 53, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 105, 54, cp::Color{r=255,g=0,b=0});

	cp::draw_pixel(window, 100, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 101, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 102, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 103, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 104, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 105, 55, cp::Color{r=255,g=0,b=0});
	
	cp::draw_pixel(window, 100, 51, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 100, 52, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 100, 53, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(window, 100, 54, cp::Color{r=255,g=0,b=0});

	cp::update(window);
}

cp::destroy(window);

print("quit");
