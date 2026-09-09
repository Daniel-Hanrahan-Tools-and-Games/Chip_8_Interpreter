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

<a href="https://github.com/Daniel-Hanrahan-Tools-and-Games/chip8-book?tab=License-1-ov-file">proof chip8_spec_info.pdf is in the public domain/equvalent</a>

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


<a href="https://github.com/Daniel-Hanrahan-Tools-and-Games/Chip_8_Interpreter">Repository Page</a>

<a href="https://daniel-hanrahan-tools-and-games.github.io/">Home Page</a>




This only affects the things in this repository that has not been noted with a different license in names and documentation files.

CC BY-SA 4.0 and GNU GPL v3.0 Conditional Exceptions to use MPL 2.0 and CC BY-SA 4.0 or CC BY 4.0

If the following condition is met, the licensing rules for both content covered by GNU GPL v3.0 and content not covered by GNU GPL v3.0 are modified as described below:

Condition:

The developer is distributing, porting, or integrating the software with platforms or environments that impose requirements incompatible with GPL-3.0, including but not limited to:
- proprietary or non-redistributable SDKs
- confidential hardware or platform documentation
- legally required confidentiality obligations preventing full GPL redistribution
- safety-regulated or certified systems where full GPL redistribution cannot be satisfied

Effect on licensing:

- Content covered by GNU GPL v3.0: May instead be used under the Mozilla Public License 2.0.

- Content not covered by GNU GPL v3.0 (e.g., assets): Normally may be used under CC BY-SA 4.0. If ShareAlike requirements of CC BY-SA 4.0 prevent lawful distribution under the MPL alternative, developers may instead use CC BY 4.0 **solely to the extent necessary** to enable such distribution.

These exceptions apply **only when the condition above is met**.





CC BY-SA 4.0 and GNU GPL v3.0 Conditional Exceptions to use PolyForm Noncommercial and CC BY-NC 4.0

The PolyForm Noncommercial License (and Creative Commons
Attribution-NonCommercial 4.0 International for non-code
content) may be used as an alternative only when the combined
work is subject to binding legal, contractual, or platform-
imposed restrictions that prohibit commercial use.

Such restrictions may arise from third-party licenses,
distribution platforms, or other enforceable legal terms that
make commercial use of the combined work not legally permitted.

Content covered by the primary license (e.g., source code or
other covered material) remains governed by that license.

Content not covered by the primary license (e.g., assets,
documentation, or other non-code materials) is governed by
CC BY-NC 4.0, unless otherwise stated.

This alternative applies only to the extent necessary to
comply with such restrictions.




CC BY-SA 4.0 and GNU GPL v3.0 Conditional Exceptions to use PolyForm Strict and CC BY-NC-ND 4.0

The PolyForm Strict License may be used as an alternative
license only when the combined work is subject to binding
legal, contractual, or platform-imposed restrictions that
require both non-commercial use and prohibit the creation of
derivative works as part of the distribution terms.

Such restrictions may arise from third-party licenses,
distribution platforms, or other enforceable legal terms that
impose both non-commercial and no-derivatives requirements on
the combined work.

Content covered by the primary license (e.g., source code or
other covered material) remains governed by that license.

Content not covered by the primary license (e.g., assets,
documentation, or other non-code materials) is governed by
Creative Commons Attribution-NonCommercial-NoDerivatives
4.0 International (CC BY-NC-ND 4.0), unless otherwise stated.

This alternative applies only to the extent necessary to
comply with such restrictions.





Contributors needed.
