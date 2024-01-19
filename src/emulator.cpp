#include "win32.h"

#include <cstdint>
#include <cstdlib>
#include <ctime>

#include <algorithm>

const uint16_t MEMORY_START_ADDRESS = 0x200;
const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;
const int VIDEO_MEMORY_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT;

struct Chip8 {
    uint8_t registers[16];
    uint16_t index;
    uint16_t stack[16];
    uint8_t memory[4096];
    uint32_t video[VIDEO_MEMORY_SIZE];
    uint16_t pc;
    uint16_t sp;
    uint16_t delay_timer;
    uint16_t sound_timer;
    bool keypad[0xF];

    uint8_t prev_key_press;
    uint8_t latest_key_press;

    bool video_updated;
    bool running;

    double frame_time;

    void cycle();
};

const uint8_t font[80] =
{
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void 
Chip8::cycle() 
{
    video_updated = false;

    uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
    uint16_t op_nible = (opcode & 0xF000) >> 12;

    pc += 2;

    uint8_t regX, regY;
    uint16_t val;

    switch (op_nible) 
    {
        case 0x0:
            switch (opcode) 
            {
                case 0x00E0: // clear the screen
                    std::memset(video, 0, sizeof(video));
                    video_updated = true;
                    break;
                case 0x00EE: // return from routine
                    pc = stack[--sp];
                    break;
                default: // call machine code routine, Not necessary for most ROMs
                    pc = opcode & 0x0FFF;
            }
            break;
        case 0x1: // jmp to address
            pc = opcode & 0x0FFF;
            break;
        case 0x2: // call subroutine
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;
        case 0x3: // Vx == NN skip instruction
            regX = (opcode & 0x0F00) >> 8;
            val = opcode & 0x00FF;

            if (registers[regX] == val)
            {
                pc += 2;
            }
            break;
        case 0x4: // Vx != NN skip instruction
            regX = (opcode & 0x0F00) >> 8;
            val = opcode & 0x00FF;

            if (registers[regX] != val)
            {
                pc += 2;
            }
            break;
        case 0x5: // Vx == Vy skip instruction
            regX = (opcode & 0x0F00) >> 8;
            regY = (opcode & 0x00F0) >> 4;

            if (registers[regX] == registers[regY])
            {
                pc += 2;
            }
            break;
        case 0x6: // Set Vx to NN
            regX = (opcode & 0x0F00) >> 8;
            val = opcode & 0x00FF;
            registers[regX] = val;
            break;
        case 0x7: // Add NN to Vx
            regX = (opcode & 0x0F00) >> 8;
            val = opcode & 0x00FF;
            registers[regX] += val;
            break;
        case 0x8: // Vx and Vy operations
            val = opcode & 0x000F;
            regX = (opcode & 0x0F00) >> 8;
            regY = (opcode & 0x00F0) >> 4;

            switch (val)
            {
                case 0x0: // Assign
                    registers[regX] = registers[regY];
                    break;
                case 0x1: // Bit OR
                    registers[regX] |= registers[regY];
                    break;
                case 0x2: // Bit AND
                    registers[regX] &= registers[regY];
                    break;
                case 0x3: // Bit XOR
                    registers[regX] ^= registers[regY];
                    break;
                case 0x4: // VX += VY - VF is set to 1 when there's an overflow, and to 0 when there is not
                    val = registers[regX] + registers[regY];

                    if (val >= 0xFF)
                    {
                        registers[0xF] = 1;
                    }
                    else
                    {
                        registers[0xF] = 0;
                    }

                    registers[regX] = val & 0xFF;
                    break;
                case 0x5: // VX -= Vy - VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
                    if (registers[regX] >= registers[regY])
                    {
                        registers[0xF] = 1;
                    }
                    else
                    {
                        registers[0xF] = 0;
                    }

                    registers[regX] -= registers[regY];
                    break;
                case 0x6: // Store the least significant bit of VX in VF and then shifts VX to the right by 1
                    registers[0xF] = registers[regX] & 0x1;
                    registers[regX] >>= 1;
                    break;
                case 0x7: // VX = VY - VX - VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX)
                    if (registers[regY] >= registers[regX])
                    {
                        registers[0xF] = 1;
                    }
                    else
                    {
                        registers[0xF] = 0;
                    }

                    registers[regX] = registers[regY] - registers[regX];
                    break;
                case 0xE: // Stores the most significant bit of VX in VF and then shifts VX to the left by 1
                    registers[0xF] = (registers[regX] & 0x80) >> 7;
                    registers[regX] <<= 1;
                    break;
            }
            break;
        case 0x9: // Vx != Vy skip instruction
            regX = (opcode & 0x0F00) >> 8;
            regY = (opcode & 0x00F0) >> 4;

            if (registers[regX] != registers[regY])
            {
                pc += 2;
            }
            break;
        case 0xA: // Set I to address NNN
            index = opcode & 0x0FFF;
            break;
        case 0xB: // jmp to V0 + NNN
            pc = registers[0] + (opcode & 0x0FFF);
            break;
        case 0xC: // Vx = rand() & NN
            regX = (opcode & 0x0F00) >> 8;
            val = opcode & 0x00FF;
            registers[regX] = (rand() % 255) & val;
            break;
        case 0xD: // Draw
        {
            regX = (opcode & 0x0F00) >> 8;
            regY = (opcode & 0x00F0) >> 4;
            uint8_t height = opcode & 0x000F;

            uint8_t pos_x = registers[regX] % SCREEN_WIDTH;
            uint8_t pos_y = registers[regY] % SCREEN_HEIGHT;

            registers[0xF] = 0;

            for (int i = 0; i < height; ++i)
            {
                uint8_t sprite = memory[index + i];

                for (int j = 0; j < 8; ++j)
                {
                    if (sprite & (0x80 >> j))
                    {
                        uint16_t pixel = pos_x + j + SCREEN_WIDTH * (pos_y + i);

                        if (video[pixel] == 0xFFFFFFFF)
                        {
                            registers[0xF] = 1;
                        }

                        video[pixel] ^= 0xFFFFFFFF;
                    }
                }
            }

            video_updated = true;
        } break;
        case 0xE:
            val = opcode & 0x00FF;
            regX = (opcode & 0x0F00) >> 8;

            switch (val)
            {
                case 0x9E: // if (key() == Vx) skip instruction
                    if (keypad[registers[regX]])
                    {
                        pc += 2;
                    }
                    break;
                case 0xA1: // if (key() != Vx) skip instruction
                    if (keypad[registers[regX]])
                    {
                        pc += 2;
                    }
                    break;
            }
            break;
        case 0xF:
            val = opcode & 0x00FF;
            regX = (opcode & 0x0F00) >> 8;

            switch(val)
            {
                case 0x07: // Set Vx to the value of the delay timer
                    registers[regX] = delay_timer;
                    break;
                case 0x0A: // Key is pressed and stored in Vx, this is a blocking operation
                    if (keypad[0x0])
                    {
                        registers[regX] = 0x0;
                    }
                    else if (keypad[0x1])
                    {
                        registers[regX] = 0x1;
                    }
                    else if (keypad[0x2])
                    {
                        registers[regX] = 0x2;
                    }
                    else if (keypad[0x3])
                    {
                        registers[regX] = 0x3;
                    }
                    else if (keypad[0x4])
                    {
                        registers[regX] = 0x4;
                    }
                    else if (keypad[0x5])
                    {
                        registers[regX] = 0x5;
                    }
                    else if (keypad[0x6])
                    {
                        registers[regX] = 0x6;
                    }
                    else if (keypad[0x7])
                    {
                        registers[regX] = 0x7;
                    }
                    else if (keypad[0x8])
                    {
                        registers[regX] = 0x8;
                    }
                    else if (keypad[0x9])
                    {
                        registers[regX] = 0x9;
                    }
                    else if (keypad[0xa])
                    {
                        registers[regX] = 0xa;
                    }
                    else if (keypad[0xb])
                    {
                        registers[regX] = 0xb;
                    }
                    else if (keypad[0xc])
                    {
                        registers[regX] = 0xc;
                    }
                    else if (keypad[0xd])
                    {
                        registers[regX] = 0xd;
                    }
                    else if (keypad[0xe])
                    {
                        registers[regX] = 0xe;
                    }
                    else if (keypad[0xf])
                    {
                        registers[regX] = 0xf;
                    }
                    else
                    {
                        pc -= 2;
                    }
                    break;
                case 0x15: // Set delay timer to Vx
                    delay_timer = registers[regX];
                    break;
                case 0x18: // Set sound timer to Vx
                    sound_timer = registers[regX];
                    break;
                case 0x1E: // Add Vx to Index
                    index += registers[regX];
                    break;
                case 0x29: // Set Index to the location of the sprite character in Vx
                    index = 5 * registers[regX]; // font start address is zero therefore can ignore adding it
                    break;
                case 0x33: // Store binary-coded-decimal representation of Vx with the hundreds digit in memory at location in Index
                    val = registers[regX];
                    
                    memory[index + 2] = val % 10;
                    val /= 10;
                    
                    memory[index + 1] = val % 10;
                    val /= 10;

                    memory[index] = val % 10;
                    break;
                case 0x55: // Store V0 to Vx (inclusive) in memory starting at Index
                    for (int i = 0; i <= registers[regX]; ++i)
                    {
                        memory[index + i] = registers[i];
                    }
                    break;
                case 0x65: // Store values from 0 to X from memory in registers V0 - Vx
                    for (int i = 0; i <= registers[regX]; ++i)
                    {
                        registers[i] = memory[index + i];
                    }
                    break;
            }
            break;
        default:
            message_box("Error", "Unknown opcode");
            running = false;
    }

    if (delay_timer > 0)
    {
        --delay_timer;
    }

    if (sound_timer > 0)
    {
        --sound_timer;
    }
}

const int RESOLUTION_UPSCALE = 15;

bool 
init_application(int argc, char **argv, void **app, int *width, int *height, char **window_title) 
{
    Chip8 *emulator = reinterpret_cast<Chip8*>(malloc(sizeof(Chip8)));

    std::memset(emulator->registers, 0, sizeof(emulator->registers));
    emulator->index = 0;
    std::memset(emulator->stack, 0, sizeof(emulator->stack));
    std::memset(emulator->memory, 0, sizeof(emulator->memory));
    std::memset(emulator->video, 0, sizeof(emulator->video));
    std::memset(emulator->keypad, false, sizeof(emulator->keypad));
    emulator->pc = MEMORY_START_ADDRESS;
    emulator->delay_timer = 0;
    emulator->sound_timer = 0;
    emulator->prev_key_press = 0;
    emulator->latest_key_press = 0;
    emulator->video_updated = false;
    emulator->running = true;

    *app = emulator;

    srand(time(NULL));

    // wiki says between 0x0000 and 0x01FF is a common font storage location
    std::copy(font, font + sizeof(font), emulator->memory);

    *width = SCREEN_WIDTH * RESOLUTION_UPSCALE;
    *height = SCREEN_HEIGHT * RESOLUTION_UPSCALE;
    *window_title = "Chip-8 Emulator";

    if (argc < 2) 
    {
        return false;
    }

    uint64_t file_size;
    uint8_t *data = read_file(argv[1], &file_size);
    
    if (!data)
    {
        return false;
    }
    
    std::copy(data, data + file_size, emulator->memory + MEMORY_START_ADDRESS);

    return true;
}

const double desired_frame_time = (1 / 60) * 1000;

bool 
update_application(void *app, double frame_time) 
{
    Chip8 *emulator = reinterpret_cast<Chip8*>(app);

    if (emulator->frame_time >= 16)
    {
        emulator->frame_time = 0;
        emulator->cycle();
    }
    else
    {
        emulator->frame_time += frame_time;
    }
    
    return emulator->running;
}

void 
handle_input(void *app, Input_events &input_events) 
{
    Chip8 *emulator = reinterpret_cast<Chip8*>(app);

    if (input_events.event[Input_events::CODES::ONE])
    {
        emulator->keypad[0x00] = input_events.event[Input_events::CODES::ONE] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::TWO])
    {
        emulator->keypad[0x01] = input_events.event[Input_events::CODES::TWO] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::THREE])
    {
        emulator->keypad[0x02] = input_events.event[Input_events::CODES::THREE] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::FOUR])
    {
        emulator->keypad[0x03] = input_events.event[Input_events::CODES::FOUR] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::Q])
    {
        emulator->keypad[0x04] = input_events.event[Input_events::CODES::Q] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::W])
    {
        emulator->keypad[0x05] = input_events.event[Input_events::CODES::W] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::E])
    {
        emulator->keypad[0x06] = input_events.event[Input_events::CODES::E] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::R])
    {
        emulator->keypad[0x07] = input_events.event[Input_events::CODES::R] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::A])
    {
        emulator->keypad[0x08] = input_events.event[Input_events::CODES::A] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::S])
    {
        emulator->keypad[0x09] = input_events.event[Input_events::CODES::S] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::D])
    {
        emulator->keypad[0x0A] = input_events.event[Input_events::CODES::D] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::F])
    {
        emulator->keypad[0x0B] = input_events.event[Input_events::CODES::F] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::Z])
    {
        emulator->keypad[0x0C] = input_events.event[Input_events::CODES::Z] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::X])
    {
        emulator->keypad[0x0D] = input_events.event[Input_events::CODES::X] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::C])
    {
        emulator->keypad[0x0E] = input_events.event[Input_events::CODES::C] & Input_events::STATE::DOWN;
    }
    else if (input_events.event[Input_events::CODES::V])
    {
        emulator->keypad[0x0F] = input_events.event[Input_events::CODES::V] & Input_events::STATE::DOWN;
    }

    if (input_events.event[Input_events::CODES::ESC] & Input_events::STATE::UP)
    {
        emulator->running = false;
    }

    std::memset(input_events.event, 0, sizeof(input_events.event));
}

bool 
render_application(void *app, uint32_t *pixels, int width, int height) 
{
     
    Chip8 *emulator = reinterpret_cast<Chip8*>(app);
   
    if (emulator->video_updated) 
    {
        std::memset(pixels, 0, width * height * sizeof(uint32_t));
        
        for (int i = 0; i < SCREEN_HEIGHT; ++i)
        {
            for (int j = 0; j < SCREEN_WIDTH; ++j)
            {
                uint32_t pixel_value = emulator->video[j + SCREEN_WIDTH * i];

                if (pixel_value)
                {
                    int adjusted_j = j * RESOLUTION_UPSCALE;

                    for (int k = 0; k < RESOLUTION_UPSCALE; ++k)
                    {
                        int adjusted_i = height - 1 - (i * RESOLUTION_UPSCALE) - k;
                        int index = adjusted_j + width * adjusted_i;
                        std::memset(pixels + index , 0x00FFFFFF, sizeof(uint32_t) * RESOLUTION_UPSCALE);
                    }
                }
            }
        }
    }

    return true;
}