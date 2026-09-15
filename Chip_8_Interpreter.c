#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h> /* Standard library providing guaranteed, non-varying integer bit-widths */

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

/* Platform Detection for Zero-Dependency Unbuffered Console Input */
#if defined(_WIN32) || defined(_WIN64)
    #include <conio.h>     /* Windows native header for _kbhit() and _getch() */
#else
    #include <unistd.h>    /* POSIX system boundary standard descriptors */
    #include <fcntl.h>     /* POSIX file descriptor manipulation libraries */
    #include <termios.h>   /* POSIX terminal control attributes */
#endif

/* Central Hardware Architecture Map */
struct Chip8 {
    uint8_t  memory[4096];                          /* 4KB flat RAM memory bank space */
    uint8_t  V[16];                                 /* 16 general-purpose 8-bit registers (V0-VF) */
    uint16_t I;                                     /* 16-bit Index Register mapping target memory addresses */
    uint16_t pc;                                    /* 16-bit Program Counter register */
    uint16_t stack[16];                             /* Subroutine execution return destination address stack */
    uint8_t  sp;                                    /* 8-bit Stack Pointer offset tracker */
    uint8_t  delay_timer;                           /* 8-bit Delay register decremented at 60Hz intervals */
    uint8_t  sound_timer;                           /* 8-bit Sound register activating audio beeps when > 0 */
    uint8_t  display[SCREEN_WIDTH * SCREEN_HEIGHT]; /* 1D logical monochrome frame-buffer tracker array */
    uint8_t  keypad[16];                            /* Current live input array state mapping context indicators */
    uint8_t  previous_keypad[16];                   /* Prior snapshot frame cache used for release tracking transitions */

    /* UNIVERSAL AUDIO DRIVER INTERFACE HOOKS */
    void (*start_beep)(void); /* Points to the driver's audio activation code */
    void (*stop_beep)(void);  /* Points to the driver's audio silence code */
};

/* Built-in 5-byte font set mapping pixel layouts for hexadecimal characters 0-F */
const uint8_t FONT_SET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, /* character '0' */   0x20, 0x60, 0x20, 0x20, 0x70, /* character '1' */
    0xF0, 0x10, 0xF0, 0x80, 0xF0, /* character '2' */   0xF0, 0x10, 0xF0, 0x10, 0xF0, /* character '3' */
    0x90, 0x90, 0xF0, 0x10, 0x10, /* character '4' */   0xF0, 0x80, 0xF0, 0x10, 0xF0, /* character '5' */
    0xF0, 0x80, 0xF0, 0x90, 0xF0, /* character '6' */   0xF0, 0x10, 0x20, 0x40, 0x40, /* character '7' */
    0xF0, 0x90, 0xF0, 0x90, 0xF0, /* character '8' */   0xF0, 0x90, 0xF0, 0x10, 0xF0, /* character '9' */
    0xF0, 0x90, 0xF0, 0x90, 0x90, /* character 'A' */   0xE0, 0x90, 0xE0, 0x90, 0xE0, /* character 'B' */
    0xF0, 0x80, 0x80, 0x80, 0xF0, /* character 'C' */   0xE0, 0x90, 0x90, 0x90, 0xE0, /* character 'D' */
    0xF0, 0x80, 0xF0, 0x80, 0xF0, /* character 'E' */   0xF0, 0x80, 0xF0, 0x80, 0x80  /* character 'F' */
};

/* Clear variables, initialize tracking parameters, and stage memory caches */
void init_chip8(struct Chip8* cpu) {
    memset(cpu, 0, sizeof(struct Chip8));       /* Zero out all arrays and system status elements */
    cpu->pc = 0x200;                            /* Staged program execution starts at offset address 0x200 */
    memcpy(&cpu->memory[0x050], FONT_SET, 80);  /* Place built-in system fonts into safe, unused legacy region 0x050 */
}

/* Portable binary disk file reader mapping flat data to virtual RAM targets */
int load_rom(struct Chip8* cpu, const char* filename) {
    FILE* file = fopen(filename, "rb");          /* Open the targeted program file in read-binary mode */
    if (!file) return 0;                        /* Return false safely if the path was completely unreadable */
    size_t bytes_read = fread(&cpu->memory[0x200], 1, sizeof(cpu->memory) - 0x200, file); /* Stream data bytes */
    fclose(file);                               /* Drop the system file descriptor */
    return (bytes_read > 0);                    /* Return true if binary data populated execution RAM banks */
}
/* Core state processor tracking high-frequency opcode execution parameters */
void emulate_cycle(struct Chip8* cpu) {
    /* Fetch Phase: Combine two consecutive bytes from memory into a single 16-bit word */
    uint16_t opcode = (cpu->memory[cpu->pc] << 8) | cpu->memory[cpu->pc + 1];
    cpu->pc += 2; /* Advance execution program counter tracking index boundaries */

    /* Decode Phase: Isolate instruction variables using bitwise filters */
    uint16_t nnn = opcode & 0x0FFF;         /* Extracts absolute address values (lowest 12 bits) */
    uint8_t  kk  = opcode & 0x00FF;         /* Extracts immediate byte numeric constant values (lowest 8 bits) */
    uint8_t  x   = (opcode & 0x0F00) >> 8;   /* Extracts register identifier location index offsets */
    uint8_t  y   = (opcode & 0x00F0) >> 4;   /* Extracts register identifier location index offsets */
    uint8_t  n   = opcode & 0x000F;         /* Extracts sprite drawing element height boundaries */

    /* Execute Phase: Route actions based on high-order instruction identifiers */
    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) { /* CLS: Clear visual buffer */
                memset(cpu->display, 0, sizeof(cpu->display));
            } else if (opcode == 0x00EE) { /* RET: Return from Subroutine */
                cpu->sp--;                  /* Retract stack pointer depth allocation indicator */
                cpu->pc = cpu->stack[cpu->sp]; /* Assign execution focus back to the parent cached destination */
            }
            break;

        case 0x1000: cpu->pc = nnn; break; /* JP addr: Jump to destination memory offset address NNN */
        case 0x2000: cpu->stack[cpu->sp] = cpu->pc; cpu->sp++; cpu->pc = nnn; break; /* CALL addr: Branch into subroutine */
        case 0x3000: if (cpu->V[x] == kk) cpu->pc += 2; break; /* SE Vx, byte: Skip next if register Vx matches literal KK */
        case 0x4000: if (cpu->V[x] != kk) cpu->pc += 2; break; /* SNE Vx, byte: Skip next if register Vx differs from literal KK */
        case 0x5000: if (cpu->V[x] == cpu->V[y]) cpu->pc += 2; break; /* SE Vx, Vy: Skip next if register Vx matches register Vy */
        case 0x6000: cpu->V[x] = kk; break; /* LD Vx, byte: Assign immediate variable content value KK into register Vx */
        case 0x7000: cpu->V[x] += kk; break; /* ADD Vx, byte: Increment register Vx by literal numeric element KK */

        case 0x8000: /* Arithmetic Logic Unit Core Switch */
            switch (n) {
                case 0x0: cpu->V[x] = cpu->V[y]; break; /* LD Vx, Vy: Assign content of register Vy into register Vx */
                case 0x1: cpu->V[x] |= cpu->V[y]; cpu->V[0xF] = 0; break; /* OR Vx, Vy: Bitwise OR calculation. Wipes VF to 0 (VIP Quirk) */
                case 0x2: cpu->V[x] &= cpu->V[y]; cpu->V[0xF] = 0; break; /* AND Vx, Vy: Bitwise AND calculation. Wipes VF to 0 (VIP Quirk) */
                case 0x3: cpu->V[x] ^= cpu->V[y]; cpu->V[0xF] = 0; break; /* XOR Vx, Vy: Bitwise XOR calculation. Wipes VF to 0 (VIP Quirk) */
                case 0x4: { /* ADD Vx, Vy: Combine registers tracking arithmetic overflow carry limitations */
                    uint16_t sum = (uint16_t)cpu->V[x] + (uint16_t)cpu->V[y];
                    cpu->V[x] = (uint8_t)(sum & 0xFF); cpu->V[0xF] = (sum > 255) ? 1 : 0;
                    break;
                }
                case 0x5: { /* SUB Vx, Vy: Subtraction logic capturing borrow evaluation boundaries */
                    uint8_t not_borrow = (cpu->V[x] >= cpu->V[y]) ? 1 : 0;
                    cpu->V[x] = cpu->V[x] - cpu->V[y]; cpu->V[0xF] = not_borrow;
                    break;
                }
                case 0x6: { /* SHR Vx: COSMAC VIP compliant shift right bit method */
                    cpu->V[x] = cpu->V[y]; /* Historical quirk: copy Vy into Vx first before applying manipulation */
                    uint8_t lsb = cpu->V[x] & 0x01; cpu->V[x] >>= 1; cpu->V[0xF] = lsb;
                    break;
                }
                case 0x7: { /* SUBN Vx, Vy: Reverse subtraction sequence */
                    uint8_t not_borrow = (cpu->V[y] >= cpu->V[x]) ? 1 : 0;
                    cpu->V[x] = cpu->V[y] - cpu->V[x]; cpu->V[0xF] = not_borrow;
                    break;
                }
                case 0xE: { /* SHL Vx: COSMAC VIP compliant shift left bit method */
                    cpu->V[x] = cpu->V[y]; /* Historical quirk: copy Vy into Vx first before applying manipulation */
                    uint8_t msb = (cpu->V[x] & 0x80) >> 7; cpu->V[x] <<= 1; cpu->V[0xF] = msb;
                    break;
                }
            }
            break;
        case 0x9000: if (cpu->V[x] != cpu->V[y]) cpu->pc += 2; break; /* SNE Vx, Vy: Skip next if register Vx differs from register Vy */
        case 0xA000: cpu->I = nnn; break; /* LD I, addr: Load absolute address data pointer value NNN onto index register I */
        case 0xB000: cpu->pc = nnn + cpu->V[0]; break; /* JP V0, addr: Divert program counter route to address pointer calculation NNN + V0 */

        case 0xC000: { /* RND Vx, byte: Pseudo-random masking routine applying a fast LFSR sequence generator */
            static unsigned int lfsr = 0xACE1u;
            lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
            cpu->V[x] = (uint8_t)(lfsr & 0xFF) & kk;
            break;
        }

        case 0xD000: /* DRW Vx, Vy, nibble: Coordinate graphics XOR blitter drawing logic loop */
            cpu->V[0xF] = 0; /* Reset baseline pixel collision monitoring flag back to zero status */
            for (int row = 0; row < n; row++) {
                uint8_t sprite_byte = cpu->memory[cpu->I + row]; /* Isolate target sprite configuration byte */
                for (int col = 0; col < 8; col++) {
                    if ((sprite_byte & (0x80 >> col)) != 0) {
                        int tx = (cpu->V[x] + col) % SCREEN_WIDTH;  /* Compute horizontal mapping wrapped boundary offsets */
                        int ty = (cpu->V[y] + row) % SCREEN_HEIGHT; /* Compute vertical mapping wrapped boundary offsets */
                        int index = tx + (ty * SCREEN_WIDTH);       /* Linear vector data location lookups */
                        if (cpu->display[index] == 1) cpu->V[0xF] = 1; /* Collision discovered: mark carry flag register setting to 1 */
                        cpu->display[index] ^= 1; /* Invert matching visual buffer frame-buffer block pixel using an exclusive OR */
                    }
                }
            }
            break;

        case 0xE000:
            if (kk == 0x9E) { if (cpu->keypad[cpu->V[x]]) cpu->pc += 2; } /* SKP Vx: Skip if keypad selection index register targets an active down event */
            else if (kk == 0xA1) { if (!cpu->keypad[cpu->V[x]]) cpu->pc += 2; } /* SKNP Vx: Skip if keypad selection index register targets a released status */
            break;

        case 0xF000:
            switch (kk) {
                case 0x07: cpu->V[x] = cpu->delay_timer; break; /* LD Vx, DT: Read out current internal software tracking delay value onto Vx */
                case 0x15: cpu->delay_timer = cpu->V[x]; break; /* LD DT, Vx: Synchronize hardware pacing delay timer parameters with Vx element contents */
                case 0x18: cpu->sound_timer = cpu->V[x]; break; /* LD ST, Vx: Synchronize hardware buzzer alert sound timer parameters with Vx element contents */
                case 0x1E: cpu->I += cpu->V[x]; break; /* ADD I, Vx: Advance structural address pointer I by register offset size value Vx */
                case 0x29: cpu->I = 0x050 + (cpu->V[x] * 5); break; /* LD F, Vx: Pin Index I path directly onto standard alphanumeric font tracking coordinates */
                case 0x33: /* LD B, Vx: Binary-Coded Decimal translation sequence math */
                    cpu->memory[cpu->I]     = cpu->V[x] / 100;
                    cpu->memory[cpu->I + 1] = (cpu->V[x] / 10) % 10;
                    cpu->memory[cpu->I + 2] = cpu->V[x] % 10;
                    break;
                case 0x55: for (int i = 0; i <= x; i++) cpu->memory[cpu->I + i] = cpu->V[i]; break; /* FX55: Snapshot registers V0-Vx out to memory arrays */
                case 0x65: for (int i = 0; i <= x; i++) cpu->V[i] = cpu->memory[cpu->I + i]; break; /* FX65: Populate registers V0-Vx from memory arrays */
                case 0x0A: { /* FX0A: Hardware press-and-release block trap hook */
                    int key_pressed = 0;
                    for (int i = 0; i < 16; i++) {
                        /* Test 3 Pass Verification: Intercept the exact boundary instant where an input falls from 1 (pressed) to 0 (released) */
                        if (cpu->previous_keypad[i] == 1 && cpu->keypad[i] == 0) {
                            cpu->V[x] = i; key_pressed = 1; break;
                        }
                    }
                    if (!key_pressed) cpu->pc -= 2; /* Freeze execution processing cadence here if release conditions have not met requirements */
                    break;
                }
            }
            break;
    }
}
/* Unbuffered, non-blocking platform keyboard parsing driver module tracking event persistence windows */
void update_keypad_input(struct Chip8* cpu) {
    int raw_char = -1; 
    static int key_hold_timer = 0;
    static int current_active_key = -1; // Track which key is currently held down

    #if defined(_WIN32) || defined(_WIN64)
        if (_kbhit()) { raw_char = _getch(); } /* Win32 non-blocking character snapshot check loop routine */
    #else
        struct termios oldt, newt; tcgetattr(STDIN_FILENO, &oldt); newt = oldt; newt.c_lflag &= ~(ICANON | ECHO); tcsetattr(STDIN_FILENO, TCSANOW, &newt); /* Disable system canonical buffers */
        int oldf = fcntl(STDIN_FILENO, F_GETFL, 0); fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); /* Append execution tracking attributes */
        char ch; if (read(STDIN_FILENO, &ch, 1) > 0) { raw_char = ch; }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); fcntl(STDIN_FILENO, F_SETFL, oldf); /* Restore terminal default parameters safely */
    #endif

    if (raw_char != -1) {
        memset(cpu->keypad, 0, sizeof(cpu->keypad)); 
        key_hold_timer = 3; /* Short persistence latch (e.g., 3 frames) to smooth over OS key-repeat gaps */
        
        int mapped_key = -1;
        switch (raw_char) { /* Map standard QWERTY rows to targeted vintage 0x0-0xF hex input array indexing structures */
            case '1': mapped_key = 0x1; break; case '2': mapped_key = 0x2; break; case '3': mapped_key = 0x3; break; case '4': mapped_key = 0xC; break;
            case 'q': case 'Q': mapped_key = 0x4; break; case 'w': case 'W': mapped_key = 0x5; break; case 'e': case 'E': mapped_key = 0x6; break; case 'r': case 'R': mapped_key = 0xD; break;
            case 'a': case 'A': mapped_key = 0x7; break; case 's': case 'S': mapped_key = 0x8; break; case 'd': case 'D': mapped_key = 0x9; break; case 'f': case 'F': mapped_key = 0xE; break;
            case 'z': case 'Z': mapped_key = 0xA; break; case 'x': case 'X': mapped_key = 0x0; break; case 'c': case 'C': mapped_key = 0xB; break; case 'v': case 'V': mapped_key = 0xF; break;
            default: mapped_key = -1; break;
        }

        if (mapped_key != -1) {
            current_active_key = mapped_key;
            cpu->keypad[current_active_key] = 1;
        }
    } else {
        if (key_hold_timer > 0) { 
            key_hold_timer--; 
            if (current_active_key != -1) {
                cpu->keypad[current_active_key] = 1; // Hold it active for a couple of frames
            }
            if (key_hold_timer == 0) { 
                memset(cpu->keypad, 0, sizeof(cpu->keypad)); 
                current_active_key = -1;
            } 
        } else {
            memset(cpu->keypad, 0, sizeof(cpu->keypad));
            current_active_key = -1;
        }
    }
}

/* Stream the linearized internal display memory buffer out to console rows using native ANSI characters */
void render_to_terminal(const struct Chip8 *cpu) {
    // Snap cursor back to the top-left instead of clearing the screen
    printf("\033[H"); 

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            // Print your pixels (e.g., '█' or ' ')
            putchar(cpu->display[y * 64 + x] ? '#' : ' ');
        }
        putchar('\n');
    }
    fflush(stdout); // Force the buffer to print immediately
}

/* 
 * ZERO-DEPENDENCY AUDIO DRIVER FALLBACKS (Terminal Alert Hook)
 * These stub functions handle the actual hardware sound production.
 */
/**
 * 🔊 FALLBACK AUDIO DRIVER (NATIVE OS OR HARDWARE HOOK)
 * This function handles the physical buzzer simulation when the CHIP-8 sound timer is active.
 * 
 * 🛠️ FOR OTHER OPERATING SYSTEMS / DRIVERS / BARE METAL:
 * If compiling on other platforms or architectures, modify the command inside:
 * 
 * - Linux Mint/Ubuntu (Default): system("paplay /path/to/sound.oga &");
 * - Windows (CMD/PowerShell):     Beep(440, 50); // Requires <windows.h>
 * - macOS (Apple Terminal):       system("afplay /System/Library/Sounds/Ping.aiff &");
 * - Open-Source SDL2 (Universal): SDL_PauseAudioDevice(audio_device, 0);
 * 
 * - Bare-Metal Embedded MCU (No OS):
 *   To flash this directly to a microchip with a real hardware buzzer attached,
 *   replace the command with a direct register write to flip a physical GPIO pin:
 * 
 *   GPIO_PIN_OUTPUT_REG |= (1 << BUZZER_PIN_OFFSET); // Pull hardware pin HIGH
 */
void fallback_beep_on(void) { 
    /* Stream a built-in system sound file directly into headphones/speakers in the background */
    system("paplay /usr/share/sounds/freedesktop/stereo/complete.oga &"); 
}

void fallback_beep_off(void) { 
    /* Audio file finishes naturally or plays a short pulse */ 
}


/* Application execution entry boundary orchestrating hardware initialization and clock cadence loops */
int main(int argc, char** argv) {
    if (argc < 2) { printf("Usage: %s <rom_file>\n", argv[0]); return 1; } /* Enforce file bounding checks */
    
    struct Chip8 cpu; 
    init_chip8(&cpu); /* Allocate architecture structures directly onto thread execution stacks */
    
    /* 
     * 🔌 HARDWARE AUDIO DRIVER INTERFACE HOOKS
     * By default, this engine utilizes Example A (The Zero-Dependency Terminal Alert Bell).
     * 
     * 💡 HOW TO CHANGE THE SOUND DRIVER:
     * To swap this out for any open-source or native framework (like SDL2, ALSA, or a 
     * bare-metal hardware pin), write your new turn-on and turn-off functions elsewhere,
     * and remap these two pointers during initialization.
     * 
     * Example for open-source SDL2 audio:
     *   cpu.start_beep = my_sdl_audio_unpause_function;
     *   cpu.stop_beep  = my_sdl_audio_pause_function;
     * 
     * Example for a Bare-Metal microcontroller hardware buzzer:
     *   cpu.start_beep = my_embedded_gpio_pin_high_function;
     *   cpu.stop_beep  = my_embedded_gpio_pin_low_function;
     */
    cpu.start_beep = fallback_beep_on;
    cpu.stop_beep  = fallback_beep_off;

    if (!load_rom(&cpu, argv[1])) { printf("Failed to load ROM file: %s\n", argv[1]); return 1; }
    printf("\033[2J"); /* Clear console display cache once before entering processing loops */

    /* Setup standard C time tracking */
    // 50000000, 20 FPS (50 milliseconds in nanoseconds)
    // 16666666, 60 FPS (16.66 milliseconds in nanoseconds) STANDARD
    struct timespec target_frame_time = {0, 16666666}; 

    int sound_was_active = 0; // Track state changes to prevent spamming audio hooks

    while (1) {
        update_keypad_input(&cpu); 

        /* Execute 30 instructions per frame to maintain ~600 IPS at 20 FPS */
        for (int i = 0; i < 30; i++) {
            emulate_cycle(&cpu); /* Update active input configurations and track opcode instructions */
        }
        
        memcpy(cpu.previous_keypad, cpu.keypad, 16);    /* Capture current input snapshot map for transition checks next frame */
        
        render_to_terminal(&cpu);
        
        if (cpu.delay_timer > 0) cpu.delay_timer--;
        
        /* Universal Audio Driver Interface Execution Loop */
        if (cpu.sound_timer > 0) {
            if (!sound_was_active) {
                if (cpu.start_beep != NULL) { cpu.start_beep(); }
                sound_was_active = 1;
            }
            cpu.sound_timer--;
        } else {
            if (sound_was_active) {
                if (cpu.stop_beep != NULL) { cpu.stop_beep(); }
                sound_was_active = 0;
            }
        }
        
        /* Sleep for 50ms to lock the game strictly to 20 FPS */
        struct timespec remaining;
        thrd_sleep(&target_frame_time, &remaining);
    }
    return 0;
}
