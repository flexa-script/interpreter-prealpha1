using cp.core.graphics;
using cp.std.random;
using cp.std.math;

cp::initialize("CP Graphics", 800, 450);

while(!cp::is_quit()) {
    clear_screen(Color{r=0,g=0,b=0});

	draw_pixel(100, 50, Color{r=255,g=0,b=0});
	draw_pixel(101, 50, Color{r=255,g=0,b=0});
	draw_pixel(102, 50, Color{r=255,g=0,b=0});
	draw_pixel(103, 50, Color{r=255,g=0,b=0});
	draw_pixel(104, 50, Color{r=255,g=0,b=0});
	draw_pixel(105, 50, Color{r=255,g=0,b=0});
	
	draw_pixel(105, 51, Color{r=255,g=0,b=0});
	draw_pixel(105, 52, Color{r=255,g=0,b=0});
	draw_pixel(105, 53, Color{r=255,g=0,b=0});
	draw_pixel(105, 54, Color{r=255,g=0,b=0});

	draw_pixel(100, 55, Color{r=255,g=0,b=0});
	draw_pixel(101, 55, Color{r=255,g=0,b=0});
	draw_pixel(102, 55, Color{r=255,g=0,b=0});
	draw_pixel(103, 55, Color{r=255,g=0,b=0});
	draw_pixel(104, 55, Color{r=255,g=0,b=0});
	draw_pixel(105, 55, Color{r=255,g=0,b=0});
	
	draw_pixel(99, 51, Color{r=255,g=0,b=0});
	draw_pixel(99, 52, Color{r=255,g=0,b=0});
	draw_pixel(99, 53, Color{r=255,g=0,b=0});
	draw_pixel(99, 54, Color{r=255,g=0,b=0});

	update();
}
