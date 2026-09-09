#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

/* Platform Detection for Zero-Dependency Unbuffered Console Input */
#if defined(_WIN32) || defined(_WIN64)
    #include <conio.h>
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <termios.h>
#endif

struct Chip8 {
    uint8_t  memory[4096];
    uint8_t  V[16];
    uint16_t I;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t  sp;
    uint8_t  delay_timer;
    uint8_t  sound_timer;
    uint8_t  display[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint8_t  keypad[16];
    uint8_t  previous_keypad[16]; /* Used for precise key-release tracking */
};

const uint8_t FONT_SET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
    0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
    0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
    0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
    0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
    0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
    0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
    0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
    0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
    0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
    0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
    0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
    0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
    0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
    0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
    0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

void init_chip8(struct Chip8* cpu) {
    memset(cpu, 0, sizeof(struct Chip8));
    cpu->pc = 0x200;
    memcpy(&cpu->memory[0x050], FONT_SET, sizeof(FONT_SET));
}

int load_rom(struct Chip8* cpu, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;
    size_t bytes_read = fread(&cpu->memory[0x200], 1, sizeof(cpu->memory) - 0x200, file);
    fclose(file);
    return (bytes_read > 0);
}
void emulate_cycle(struct Chip8* cpu) {
    uint16_t opcode = (cpu->memory[cpu->pc] << 8) | cpu->memory[cpu->pc + 1];
    cpu->pc += 2;

    uint16_t nnn = opcode & 0x0FFF;
    uint8_t  kk  = opcode & 0x00FF;
    uint8_t  x   = (opcode & 0x0F00) >> 8;
    uint8_t  y   = (opcode & 0x00F0) >> 4;
    uint8_t  n   = opcode & 0x000F;

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                memset(cpu->display, 0, sizeof(cpu->display));
            } else if (opcode == 0x00EE) {
                cpu->sp--;
                cpu->pc = cpu->stack[cpu->sp];
            }
            break;

        case 0x1000: cpu->pc = nnn; break;
        case 0x2000: cpu->stack[cpu->sp] = cpu->pc; cpu->sp++; cpu->pc = nnn; break;
        case 0x3000: if (cpu->V[x] == kk) cpu->pc += 2; break;
        case 0x4000: if (cpu->V[x] != kk) cpu->pc += 2; break;
        case 0x5000: if (cpu->V[x] == cpu->V[y]) cpu->pc += 2; break;
        case 0x6000: cpu->V[x] = kk; break;
        case 0x7000: cpu->V[x] += kk; break;

        case 0x8000:
            switch (n) {
                case 0x0: cpu->V[x] = cpu->V[y]; break;
                case 0x1: cpu->V[x] |= cpu->V[y]; cpu->V[0xF] = 0; break;
                case 0x2: cpu->V[x] &= cpu->V[y]; cpu->V[0xF] = 0; break;
                case 0x3: cpu->V[x] ^= cpu->V[y]; cpu->V[0xF] = 0; break;
                case 0x4: {
                    uint16_t sum = (uint16_t)cpu->V[x] + (uint16_t)cpu->V[y];
                    cpu->V[x] = (uint8_t)(sum & 0xFF);
                    cpu->V[0xF] = (sum > 255) ? 1 : 0;
                    break;
                }
                case 0x5: {
                    uint8_t not_borrow = (cpu->V[x] >= cpu->V[y]) ? 1 : 0;
                    cpu->V[x] = cpu->V[x] - cpu->V[y];
                    cpu->V[0xF] = not_borrow;
                    break;
                }
                case 0x6: {
                    cpu->V[x] = cpu->V[y];
                    uint8_t lsb = cpu->V[x] & 0x01;
                    cpu->V[x] >>= 1;
                    cpu->V[0xF] = lsb;
                    break;
                }
                case 0x7: {
                    uint8_t not_borrow = (cpu->V[y] >= cpu->V[x]) ? 1 : 0;
                    cpu->V[x] = cpu->V[y] - cpu->V[x];
                    cpu->V[0xF] = not_borrow;
                    break;
                }
                case 0xE: {
                    cpu->V[x] = cpu->V[y];
                    uint8_t msb = (cpu->V[x] & 0x80) >> 7;
                    cpu->V[x] <<= 1;
                    cpu->V[0xF] = msb;
                    break;
                }
            }
            break;

        case 0x9000: if (cpu->V[x] != cpu->V[y]) cpu->pc += 2; break;
        case 0xA000: cpu->I = nnn; break;
        case 0xB000: cpu->pc = nnn + cpu->V[0]; break;

        case 0xC000: {
            static unsigned int lfsr = 0xACE1u;
            lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
            cpu->V[x] = (uint8_t)(lfsr & 0xFF) & kk;
            break;
        }

        case 0xD000:
            cpu->V[0xF] = 0;
            for (int row = 0; row < n; row++) {
                uint8_t sprite_byte = cpu->memory[cpu->I + row];
                for (int col = 0; col < 8; col++) {
                    if ((sprite_byte & (0x80 >> col)) != 0) {
                        int tx = (cpu->V[x] + col) % SCREEN_WIDTH;
                        int ty = (cpu->V[y] + row) % SCREEN_HEIGHT;
                        int index = tx + (ty * SCREEN_WIDTH);
                        if (cpu->display[index] == 1) cpu->V[0xF] = 1;
                        cpu->display[index] ^= 1;
                    }
                }
            }
            break;

        case 0xE000:
            if (kk == 0x9E) {
                if (cpu->keypad[cpu->V[x]]) cpu->pc += 2;
            } else if (kk == 0xA1) {
                if (!cpu->keypad[cpu->V[x]]) cpu->pc += 2;
            }
            break;

        case 0xF000:
            switch (kk) {
                case 0x07: cpu->V[x] = cpu->delay_timer; break;
                case 0x15: cpu->delay_timer = cpu->V[x]; break;
                case 0x18: cpu->sound_timer = cpu->V[x]; break;
                case 0x1E: cpu->I += cpu->V[x]; break;
                case 0x29: cpu->I = 0x050 + (cpu->V[x] * 5); break;
                case 0x33:
                    cpu->memory[cpu->I]     = cpu->V[x] / 100;
                    cpu->memory[cpu->I + 1] = (cpu->V[x] / 10) % 10;
                    cpu->memory[cpu->I + 2] = cpu->V[x] % 10;
                    break;
                case 0x55: for (int i = 0; i <= x; i++) cpu->memory[cpu->I + i] = cpu->V[i]; break;
                case 0x65: for (int i = 0; i <= x; i++) cpu->V[i] = cpu->memory[cpu->I + i]; break;
                case 0x0A: {
                    int key_pressed = 0;
                    for (int i = 0; i < 16; i++) {
                        /* Fixed to watch for exact key-release transitions to satisfy test 3 */
                        if (cpu->previous_keypad[i] == 1 && cpu->keypad[i] == 0) {
                            cpu->V[x] = i;
                            key_pressed = 1;
                            break;
                        }
                    }
                    if (!key_pressed) cpu->pc -= 2;
                    break;
                }
            }
            break;
    }
}
void update_keypad_input(struct Chip8* cpu) {
    int raw_char = -1;
    static int key_hold_timer = 0;

    #if defined(_WIN32) || defined(_WIN64)
        if (_kbhit()) { raw_char = _getch(); }
    #else
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0) { raw_char = ch; }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
    #endif

    if (raw_char != -1) {
        memset(cpu->keypad, 0, sizeof(cpu->keypad));
        key_hold_timer = 350; /* Latch holds the button active across standard cycles */

        switch (raw_char) {
            case '1': cpu->keypad[0x1] = 1; break; case '2': cpu->keypad[0x2] = 1; break;
            case '3': cpu->keypad[0x3] = 1; break; case '4': cpu->keypad[0xC] = 1; break;
            case 'q': case 'Q': cpu->keypad[0x4] = 1; break; case 'w': case 'W': cpu->keypad[0x5] = 1; break;
            case 'e': case 'E': cpu->keypad[0x6] = 1; break; case 'r': case 'R': cpu->keypad[0xD] = 1; break;
            case 'a': case 'A': cpu->keypad[0x7] = 1; break; case 's': case 'S': cpu->keypad[0x8] = 1; break;
            case 'd': case 'D': cpu->keypad[0x9] = 1; break; case 'f': case 'F': cpu->keypad[0xE] = 1; break;
            case 'z': case 'Z': cpu->keypad[0xA] = 1; break; case 'x': case 'X': cpu->keypad[0x0] = 1; break;
            case 'c': case 'C': cpu->keypad[0xB] = 1; break; case 'v': case 'V': cpu->keypad[0xF] = 1; break;
            default: key_hold_timer = 0; break;
        }
    } else {
        if (key_hold_timer > 0) {
            key_hold_timer--;
            if (key_hold_timer == 0) {
                memset(cpu->keypad, 0, sizeof(cpu->keypad));
            }
        }
    }
}

void render_to_terminal(const struct Chip8* cpu) {
    printf("\033[H");
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            printf(cpu->display[y * SCREEN_WIDTH + x] ? "█" : " ");
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <rom_file>\n", argv[0]);
        return 1;
    }

    struct Chip8 cpu;
    init_chip8(&cpu);

    if (!load_rom(&cpu, argv[1])) {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return 1;
    }

    printf("\033[2J");
    unsigned int loops = 0;

    while (1) {
        update_keypad_input(&cpu);
        emulate_cycle(&cpu);
        
        /* Fixed latch tracking cache sync hook directly in the main execution ring */
        memcpy(cpu.previous_keypad, cpu.keypad, sizeof(cpu.keypad));
        
        loops++;
        if (loops % 10 == 0) {
            render_to_terminal(&cpu);
            if (cpu.delay_timer > 0) cpu.delay_timer--;
            if (cpu.sound_timer > 0) cpu.sound_timer--;
        }
        for (volatile int delay = 0; delay < 120000; delay++);
    }
    return 0;
}

