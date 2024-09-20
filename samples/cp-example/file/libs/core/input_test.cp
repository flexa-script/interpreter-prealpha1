using cp.core.input;

as namespace cp;

fun test_key_press() {
    println("Press 'A' to continue...");
    while (not is_key_pressed(KEY_A)) {
        // Waiting for 'A' key press
    }
    println("Key 'A' pressed!\n");
}

fun test_mouse_position() {
    println("Move the mouse to change its position...\n");
    var pos = get_mouse_position();
    while (not is_key_pressed(KEY_ENTER)) {
        var new_pos = get_mouse_position();
        if (new_pos.x != pos.x or new_pos.y != pos.y) {
            println("Mouse moved to position: ");
            println(new_pos.x, ", ", new_pos.y, "\n");
            pos = new_pos;
        }
    }
}

fun test_mouse_click() {
    println("Click the left mouse button to continue...");
    while (not is_mouse_button_pressed(MOUSE_LEFT_BUTTON)) {
        // Waiting for mouse click
    }
    println("Mouse left button clicked!\n");
}

// fun test_mouse_wheel_scroll() {
//     println("Scroll the mouse wheel...");
//     var initial_scroll = get_mouse_wheel_scroll();
//     while (true) {
//         var current_scroll = get_mouse_wheel_scroll();
//         if (current_scroll != initial_scroll) {
//             println("Mouse wheel scrolled: " + (current_scroll > initial_scroll ? "Up" : "Down") + "\n");
//             initial_scroll = current_scroll;
//         }
//     }
// }

fun run_all_tests() {
    test_key_press();
    test_mouse_position();
    test_mouse_click();
    // test_mouse_wheel_scroll();
}

run_all_tests();
