using cp.core.console;

as namespace cp;

show_console(false);

print("[ ] Testing colors\n");
set_console_cursor_position(0, 0);
println("visible: ", is_console_visible());
set_console_font("Arial", 0, 24);

for(var k = 0; k < 16; k++) {
  for(var j = 0; j < 16; j++) {
    set_console_color(k, j);
    print('['+string(k)+","+string(j)+"] Testing colors\n");
  }
}

set_console_color(0, 7);
print("[0,7] Testing color default\n");
set_console_color(1, 7);
print("[1,7] Testing color default\n");
set_console_color(2, 7);
print("[2,7] Testing color default\n");
set_console_color(CL_BRIGHT_WHITE, CL_BLACK);
print("[2,7] Testing color default\n");

show_console(true);
