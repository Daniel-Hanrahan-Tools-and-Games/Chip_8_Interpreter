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
    int raw_char = -1; static int key_hold_timer = 0;

    #if defined(_WIN32) || defined(_WIN64)
        if (_kbhit()) { raw_char = _getch(); } /* Win32 non-blocking character snapshot check loop routine */
    #else
        struct termios oldt, newt; tcgetattr(STDIN_FILENO, &oldt); newt = oldt; newt.c_lflag &= ~(ICANON | ECHO); tcsetattr(STDIN_FILENO, TCSANOW, &newt); /* Disable system canonical buffers */
        int oldf = fcntl(STDIN_FILENO, F_GETFL, 0); fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); /* Append execution tracking attributes */
        char ch; if (read(STDIN_FILENO, &ch, 1) > 0) { raw_char = ch; }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); fcntl(STDIN_FILENO, F_SETFL, oldf); /* Restore terminal default parameters safely */
    #endif

    if (raw_char != -1) {
        memset(cpu->keypad, 0, sizeof(cpu->keypad)); key_hold_timer = 350; /* Set persistence latch block to hold button states visible */
        switch (raw_char) { /* Map standard QWERTY rows to targeted vintage 0x0-0xF hex input array indexing structures */
            case '1': cpu->keypad[0x1] = 1; break; case '2': cpu->keypad[0x2] = 1; break; case '3': cpu->keypad[0x3] = 1; break; case '4': cpu->keypad[0xC] = 1; break;
            case 'q': case 'Q': cpu->keypad[0x4] = 1; break; case 'w': case 'W': cpu->keypad[0x5] = 1; break; case 'e': case 'E': cpu->keypad[0x6] = 1; break; case 'r': case 'R': cpu->keypad[0xD] = 1; break;
            case 'a': case 'A': cpu->keypad[0x7] = 1; break; case 's': case 'S': cpu->keypad[0x8] = 1; break; case 'd': case 'D': cpu->keypad[0x9] = 1; break; case 'f': case 'F': cpu->keypad[0xE] = 1; break;
            case 'z': case 'Z': cpu->keypad[0xA] = 1; break; case 'x': case 'X': cpu->keypad[0x0] = 1; break; case 'c': case 'C': cpu->keypad[0xB] = 1; break; case 'v': case 'V': cpu->keypad[0xF] = 1; break;
            default: key_hold_timer = 0; break;
        }
    } else {
        if (key_hold_timer > 0) { key_hold_timer--; if (key_hold_timer == 0) { memset(cpu->keypad, 0, sizeof(cpu->keypad)); } } /* Latch window elapsed: drop status array registers back to 0 */
    }
}

/* Stream the linearized internal display memory buffer out to console rows using native ANSI characters */
void render_to_terminal(const struct Chip8* cpu) {
    printf("\033[H"); /* Overwrite character rows tracking directly from absolute home coordinates to avoid visual panel flashes */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) { printf(cpu->display[y * SCREEN_WIDTH + x] ? "█" : " "); }
        printf("\n");
    }
}

/* Application execution entry boundary orchestrating hardware initialization and clock cadence loops */
int main(int argc, char** argv) {
    if (argc < 2) { printf("Usage: %s <rom_file>\n", argv[0]); return 1; } /* Enforce file bounding checks */
    struct Chip8 cpu; init_chip8(&cpu); /* Allocate architecture structures directly onto thread execution stacks */
    if (!load_rom(&cpu, argv[1])) { printf("Failed to load ROM file: %s\n", argv[1]); return 1; }
    printf("\033[2J"); unsigned int loops = 0; /* Clear console display cache once before entering processing loops */

    while (1) {
        update_keypad_input(&cpu); emulate_cycle(&cpu); /* Update active input configurations and track opcode instructions */
        memcpy(cpu.previous_keypad, cpu.keypad, 16);    /* Capture current input snapshot map for transition checks next frame */
        loops++;
        if (loops % 10 == 0) {
            render_to_terminal(&cpu);
            if (cpu.delay_timer > 0) cpu.delay_timer--;
            if (cpu.sound_timer > 0) cpu.sound_timer--;
        }
        for (volatile int delay = 0; delay < 120000; delay++); /* Standard execution pacing loop throttle anchor block */
    }
    return 0;
}

