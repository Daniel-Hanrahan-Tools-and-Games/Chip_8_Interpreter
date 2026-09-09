# Chip_8_Interpreter
A Chip8 interpreter I made myself.

I have tested the interpreter myself except for beep because no sound driver is connected to it and it works perfectly.

How to run software in Chip_8_Interpereter:
<ol>
<li>Drag interpreter executable into terminal</li>
<li>Drag a .ch8 file into terminal</li>
<li>Press Enter</li>
</ol>

To compile interpreter you need to cd into the folder you want your executable in, in the terminal.

How to compile interpreter with gcc:
copy this into your terminal: gcc -O2 what/directory/its/in/Chip_8_Interpreter.c -o Chip_8_Interpreter

<a href="https://github.com/Daniel-Hanrahan-Tools-and-Games/chip8-test-suite">Run tests in order of chip8-test-suite from here except beep because the code is not connected to any sound driver and scrolling because that was never part of original chip8 spec.</a>

<a href="https://github.com/Daniel-Hanrahan-Tools-and-Games/extensions">spec for chip 8 extensions if anyone wants to use features of chip 8 extensions</a>

Controls, only for keyboard so far:
Original CHIP-8 Keypad              Your Modern QWERTY Mapping
┌───┬───┬───┬───┐                   ┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ C │                   │ 1 │ 2 │ 3 │ 4 │
├───┼───┼───┼───┤                   ├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ D │       ────►       │ Q │ W │ E │ R │
├───┼───┼───┼───┤                   ├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ E │                   │ A │ S │ D │ F │
├───┼───┼───┼───┤                   ├───┼───┼───┼───┤
│ A │ 0 │ B │ F │                   │ Z │ X │ C │ V │
└───┴───┴───┴───┘                   └───┴───┴───┴───┘
