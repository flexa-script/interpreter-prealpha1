using cp.core.graphics;
// using cp.std.random;
// using cp.std.math;

cp::initialize("CP Graphics", 800, 450);

while(not cp::is_quit()) {
    cp::clear_screen(cp::Color{r=0,g=0,b=0});

	cp::draw_pixel(100, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(101, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(102, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(103, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(104, 50, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(105, 50, cp::Color{r=255,g=0,b=0});
	
	cp::draw_pixel(105, 51, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(105, 52, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(105, 53, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(105, 54, cp::Color{r=255,g=0,b=0});

	cp::draw_pixel(100, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(101, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(102, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(103, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(104, 55, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(105, 55, cp::Color{r=255,g=0,b=0});
	
	cp::draw_pixel(99, 51, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(99, 52, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(99, 53, cp::Color{r=255,g=0,b=0});
	cp::draw_pixel(99, 54, cp::Color{r=255,g=0,b=0});

	cp::update();
}

print("quit");
