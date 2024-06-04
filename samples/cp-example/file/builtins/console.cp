print("[ ] Testing colors\n");
//set_console_font("Arial", 0, 24);

for(var k = 0; k < 255; k++) {
    set_console_color(k);
    print('['+string(k)+"] Testing colors\n");
    //set_console_cursor_position(0, 0);
}

set_console_color(7);
print("[7] Testing color default\n");
