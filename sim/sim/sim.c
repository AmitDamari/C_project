/*****************************************************************
/*****************************************************************
 * SIMP Processor Simulator
 *
 * Implements complete SIMP architecture including:
 * - Instruction and data memory
 * - I/O devices and interrupts
 * - Disk operations
 * - Monitor and LED outputs
 *****************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

 /************************* Constants *************************/
#define MEMORY_SIZE 4096
#define DISK_SIZE 16384      // 128 sectors * 128 words per sector
#define MONITOR_SIZE 256
#define MAX_LINE_LENGTH 500
#define DISK_BUSY_CYCLES 1024

/************************* Data Structures *************************/
typedef struct {
    // CPU registers and state
    uint32_t registers[16];      // R0-R15
    uint32_t pc;                 // Program counter
    uint64_t imem[MEMORY_SIZE];  // Instruction memory
    uint32_t dmem[MEMORY_SIZE];  // Data memory
    uint32_t disk[DISK_SIZE];    // Disk storage

    // Interrupt registers
    uint32_t irq0enable;
    uint32_t irq1enable;
    uint32_t irq2enable;
    uint32_t irq0status;
    uint32_t irq1status;
    uint32_t irq2status;
    uint32_t irqhandler;
    uint32_t irqreturn;
    int in_interrupt;

    // Timer registers
    uint32_t timerenable;
    uint32_t timercurrent;
    uint32_t timermax;

    // Disk registers
    uint32_t diskcmd;
    uint32_t disksector;
    uint32_t diskbuffer;
    uint32_t diskstatus;
    uint32_t disk_busy_cycles;

    // Monitor registers
    uint32_t monitoraddr;
    uint32_t monitordata;
    uint32_t monitorcmd;
    uint8_t monitor_buffer[MONITOR_SIZE][MONITOR_SIZE];

    // I/O registers
    uint32_t leds;
    uint32_t display7seg;

    // Simulation state
    uint32_t cycle_counter;
    int halt;
} Processor;

typedef struct {
    uint32_t opcode;
    uint32_t rd;
    uint32_t rs;
    uint32_t rt;
    uint32_t rm;
    uint32_t immediate1;
    uint32_t immediate2;
} Instruction;

/************************* Function Prototypes *************************/
// Initialization
void init_processor(Processor* proc);
int load_memory32(const char* filename, uint32_t* memory, int size, int word_size);
int load_memory64(const char* filename, uint64_t* memory, int size, int word_size);
int load_irq2_timing(const char* filename, uint32_t* timing, int* count);

// Instruction handling
Instruction decode_instruction(uint64_t word);
void execute_instruction(Processor* proc, Instruction inst);

// I/O operations
void handle_io_read(Processor* proc, uint32_t address, uint32_t* value);
void handle_io_write(Processor* proc, uint32_t address, uint32_t value);
void update_devices(Processor* proc);

// Interrupt handling
void check_interrupts(Processor* proc);
void handle_timer(Processor* proc);
void handle_disk(Processor* proc);
void check_irq2(Processor* proc, uint32_t* irq2_timing, int irq2_count);

// Output generation
void write_trace(FILE* f, Processor* proc, uint64_t inst);
void write_hwregtrace(FILE* f, uint32_t cycle, const char* name, const char* action, uint32_t value);
void write_regout(FILE* f, Processor* proc);
void write_dmemout(FILE* f, Processor* proc);
void write_diskout(FILE* f, Processor* proc);
void write_monitor(FILE* f_txt, FILE* f_yuv, Processor* proc);
void update_led_display(FILE* fled, FILE* fdisplay, Processor* proc, uint32_t prev_leds, uint32_t prev_display);

// Simulation
void simulate(Processor* proc, char* argv[]);

/************************* Global Variables *************************/
const char* io_register_names[] = {
    "irq0enable",   // 0
    "irq1enable",   // 1
    "irq2enable",   // 2
    "irq0status",   // 3
    "irq1status",   // 4
    "irq2status",   // 5
    "irqhandler",   // 6
    "irqreturn",    // 7
    "clks",         // 8
    "leds",         // 9
    "display7seg",  // 10
    "timerenable",  // 11
    "timercurrent", // 12
    "timermax",     // 13
    "diskcmd",      // 14
    "disksector",   // 15
    "diskbuffer",   // 16
    "diskstatus",   // 17
    "reserved0",    // 18
    "reserved1",    // 19
    "monitoraddr",  // 20
    "monitordata",  // 21
    "monitorcmd"    // 22
};
/************************* Initialization Functions *************************/
void init_processor(Processor* proc) {
    memset(proc, 0, sizeof(Processor));
    proc->registers[0] = 0;  // $zero always 0
    proc->pc = 0;
    proc->halt = 0;
    proc->cycle_counter = 0;
    proc->in_interrupt = 0;
}

int load_memory32(const char* filename, uint32_t* memory, int size, int word_size) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    char line[MAX_LINE_LENGTH];
    int addr = 0;

    // Read each line
    while (addr < size && fgets(line, sizeof(line), f)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        // Convert hex string to integer
        uint32_t value;
        value = strtoull(line, NULL, 16);
        memory[addr] = value;
        addr++;
    }

    // Fill rest with zeros
    while (addr < size) {
        memory[addr++] = 0;
    }

    fclose(f);
    return 1;
}

int load_memory64(const char* filename, uint64_t* memory, int size, int word_size) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    char line[MAX_LINE_LENGTH];
    int addr = 0;

    // Read each line
    while (addr < size && fgets(line, sizeof(line), f)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        // Convert hex string to integer
        uint64_t value;
        value = strtoull(line, NULL, 16);
        memory[addr] = value;
        addr++;
    }

    // Fill rest with zeros
    while (addr < size) {
        memory[addr++] = 0;
    }

    fclose(f);
    return 1;
}
int load_irq2_timing(const char* filename, uint32_t* timing, int* count) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    *count = 0;
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), f) && *count < MEMORY_SIZE) {
        if (strlen(line) > 1) {
            timing[(*count)++] = atoi(line);
        }
    }

    fclose(f);
    return 1;
}

/************************* Instruction Handling *************************/
Instruction decode_instruction(uint64_t word) {
    Instruction inst;

    // Extract fields according to instruction format
    inst.opcode = (word >> 40) & 0x3F;     // Bits 40-47
    inst.rd = (word >> 36) & 0xF;          // Bits 36-39
    inst.rs = (word >> 32) & 0xF;          // Bits 32-35
    inst.rt = (word >> 28) & 0xF;          // Bits 28-31
    inst.rm = (word >> 24) & 0xF;          // Bits 24-27
    inst.immediate1 = (word >> 12) & 0xFFF; // Bits 23-12
    inst.immediate2 = word & 0xFFF;        // Bits 11-0

    // Sign extend immediate if needed
    if (inst.immediate1 & 0x800) {
        inst.immediate1 |= 0xFFFFF000;
    }
    if (inst.immediate2 & 0x800) {
        inst.immediate2 |= 0xFFFFF000;
    }


    return inst;
}

void execute_instruction(Processor* proc, Instruction inst) {
    uint32_t* regs = proc->registers;
    uint32_t temp;  // For temporary calculations
    int pc_modified = 0; // Flag to check if pc was modified

    // Update special registers
    regs[1] = inst.immediate1;  // $imm1
    regs[2] = inst.immediate2;  // $imm2

    switch (inst.opcode) {
    case 0:  // add
        regs[inst.rd] = regs[inst.rs] + regs[inst.rt] + regs[inst.rm];
        break;

    case 1:  // sub
        regs[inst.rd] = regs[inst.rs] - regs[inst.rt] - regs[inst.rm];
        break;

    case 2:  // mac
        regs[inst.rd] = regs[inst.rs] * regs[inst.rt] + regs[inst.rm];
        break;

    case 3:  // and
        regs[inst.rd] = regs[inst.rs] & regs[inst.rt] & regs[inst.rm];
        break;

    case 4:  // or
        regs[inst.rd] = regs[inst.rs] | regs[inst.rt] | regs[inst.rm];
        break;

    case 5:  // xor
        regs[inst.rd] = regs[inst.rs] ^ regs[inst.rt] ^ regs[inst.rm];
        break;

    case 6:  // sll
        regs[inst.rd] = regs[inst.rs] << regs[inst.rt];
        break;

    case 7:  // sra
        regs[inst.rd] = (int32_t)regs[inst.rs] >> regs[inst.rt];
        break;

    case 8:  // srl
        regs[inst.rd] = regs[inst.rs] >> regs[inst.rt];
        break;

    case 9:  // beq
        if (regs[inst.rs] == regs[inst.rt]) {
            proc->pc = regs[inst.rm];
            pc_modified = 1;
        }
        break;

    case 10:  // bne
        if (regs[inst.rs] != regs[inst.rt]) {
            proc->pc = regs[inst.rm];
            pc_modified = 1;
        }
        break;

    case 11:  // blt
        if ((int32_t)regs[inst.rs] < (int32_t)regs[inst.rt]) {
            proc->pc = regs[inst.rm];
            pc_modified = 1;
        }
        break;

    case 12:  // bgt
        if ((int32_t)regs[inst.rs] > (int32_t)regs[inst.rt]) {
            proc->pc = regs[inst.rm];
            pc_modified = 1;
        }
        break;

    case 13:  // ble
        if ((int32_t)regs[inst.rs] <= (int32_t)regs[inst.rt]) {
            proc->pc = regs[inst.rm];
            pc_modified = 1;
        }
        break;

    case 14:  // bge
        if ((int32_t)regs[inst.rs] >= (int32_t)regs[inst.rt]) {
            proc->pc = regs[inst.rm];
            pc_modified = 1;
        }
        break;

    case 15:  // jal
        regs[inst.rd] = proc->pc + 1;
        proc->pc = regs[inst.rm];
        pc_modified = 1;
        break;

    case 16:  // lw
        temp = regs[inst.rs] + regs[inst.rt];
        if (temp < MEMORY_SIZE) {
            regs[inst.rd] = proc->dmem[temp] + regs[inst.rm];
        }
        break;

    case 17:  // sw
        temp = regs[inst.rs] + regs[inst.rt];
        if (temp < MEMORY_SIZE) {
            proc->dmem[temp] = regs[inst.rd] + regs[inst.rm];
        }
        break;

    case 18:  // reti
        proc->pc = proc->irqreturn;
        proc->in_interrupt = 0;
        pc_modified = 1;
        break;

    case 19:  // in
        handle_io_read(proc, regs[inst.rs] + regs[inst.rt], &regs[inst.rd]);
        break;

    case 20:  // out
        handle_io_write(proc, regs[inst.rs] + regs[inst.rt], regs[inst.rm]);
        break;

    case 21:  // halt
        proc->halt = 1;
        break;

    default:
        break;
    }

    // Ensure $zero stays 0
    regs[0] = 0;

    // If PC wasn't modified, increment it
    if (!pc_modified && !proc->halt) {
        proc->pc++;
    }
}
/************************* I/O and Device Management *************************/
void handle_io_read(Processor* proc, uint32_t address, uint32_t* value) {
    switch (address) {
    case 0: *value = proc->irq0enable; break;
    case 1: *value = proc->irq1enable; break;
    case 2: *value = proc->irq2enable; break;
    case 3: *value = proc->irq0status; break;
    case 4: *value = proc->irq1status; break;
    case 5: *value = proc->irq2status; break;
    case 6: *value = proc->irqhandler; break;
    case 7: *value = proc->irqreturn; break;
    case 8: *value = proc->cycle_counter; break;
    case 9: *value = proc->leds; break;
    case 10: *value = proc->display7seg; break;
    case 11: *value = proc->timerenable; break;
    case 12: *value = proc->timercurrent; break;
    case 13: *value = proc->timermax; break;
    case 14: *value = proc->diskcmd; break;
    case 15: *value = proc->disksector; break;
    case 16: *value = proc->diskbuffer; break;
    case 17: *value = proc->diskstatus; break;
    case 20: *value = proc->monitoraddr; break;    // Corrected mapping
    case 21: *value = proc->monitordata; break;
    case 22: *value = proc->monitorcmd; break;
    default: *value = 0; break;
    }
}

void handle_io_write(Processor* proc, uint32_t address, uint32_t value) {
    switch (address) {
    case 0: proc->irq0enable = value & 1; break;
    case 1: proc->irq1enable = value & 1; break;
    case 2: proc->irq2enable = value & 1; break;
    case 3: proc->irq0status = value & 1; break;
    case 4: proc->irq1status = value & 1; break;
    case 5: proc->irq2status = value & 1; break;
    case 6: proc->irqhandler = value; break;
    case 7: proc->irqreturn = value; break;
    case 9: proc->leds = value; break;
    case 10: proc->display7seg = value; break;
    case 11: proc->timerenable = value & 1; break;
    case 12: proc->timercurrent = value; break;
    case 13: proc->timermax = value; break;
    case 14:
        proc->diskcmd = value;
        if (value == 1 || value == 2) {  // Read or Write command
            proc->diskstatus = 1;  // Set busy
            proc->disk_busy_cycles = 0;
        }
        break;
    case 15: proc->disksector = value; break;
    case 16: proc->diskbuffer = value; break;
    case 20: proc->monitoraddr = value; break;
    case 21: proc->monitordata = value & 0xFF; break;
    case 22:
        if (value == 1) {  // Write pixel command
            uint32_t x = proc->monitoraddr % MONITOR_SIZE;
            uint32_t y = proc->monitoraddr / MONITOR_SIZE;
            if (x < MONITOR_SIZE && y < MONITOR_SIZE) {
                proc->monitor_buffer[y][x] = proc->monitordata;
            }
        }
        break;
    }
}

/************************* Interrupt Handling *************************/
void check_interrupts(Processor* proc) {
    if (!proc->in_interrupt) {
        uint32_t irq = (proc->irq0enable & proc->irq0status) |
            (proc->irq1enable & proc->irq1status) |
            (proc->irq2enable & proc->irq2status);

        if (irq) {
            proc->irqreturn = proc->pc;
            proc->pc = proc->irqhandler;
            proc->in_interrupt = 1;
        }
    }
}

void handle_timer(Processor* proc) {
    if (proc->timerenable) {
        proc->timercurrent++;
        if (proc->timercurrent >= proc->timermax) {
            proc->irq0status = 1;
            proc->timercurrent = 0;
        }
    }
}

void handle_disk(Processor* proc) {
    if (proc->diskstatus) {  // If disk is busy
        proc->disk_busy_cycles++;
        if (proc->disk_busy_cycles >= DISK_BUSY_CYCLES) {
            // Perform disk operation
            if (proc->diskcmd == 1) {  // Read
                for (int i = 0; i < 128; i++) {  // 128 words per sector
                    proc->dmem[proc->diskbuffer + i] =
                        proc->disk[proc->disksector * 128 + i];
                }
            }
            else if (proc->diskcmd == 2) {  // Write
                for (int i = 0; i < 128; i++) {
                    proc->disk[proc->disksector * 128 + i] =
                        proc->dmem[proc->diskbuffer + i];
                }
            }

            proc->diskstatus = 0;  // Set disk ready
            proc->diskcmd = 0;     // Clear command
            proc->irq1status = 1;  // Set disk interrupt
            proc->disk_busy_cycles = 0;
        }
    }
}

void check_irq2(Processor* proc, uint32_t* irq2_timing, int irq2_count) {
    for (int i = 0; i < irq2_count; i++) {
        if (proc->cycle_counter == irq2_timing[i]) {
            proc->irq2status = 1;
            break;
        }
    }
}

/************************* Device Updates *************************/
void update_devices(Processor* proc) {
    handle_timer(proc);
    handle_disk(proc);
    check_interrupts(proc);
}
/************************* File Output Functions *************************/
void write_trace(FILE* f, Processor* proc, uint64_t inst) {
    // Format: PC INST R0-R15
    fprintf(f, "%03X %012llX", proc->pc, inst & 0xFFFFFFFFFFFF);  // Ensure 12 hex digits
    for (int i = 0; i < 16; i++) {
        fprintf(f, " %08X", proc->registers[i]);
    }
    fprintf(f, "\n");
}

void write_hwregtrace(FILE* f, uint32_t cycle, const char* name,
    const char* action, uint32_t value) {
    fprintf(f, "%d %s %s %08X\n", cycle, action, name, value);
}

void write_regout(FILE* f, Processor* proc) {
    // Write registers R3-R15 (skip R0-R2)
    for (int i = 3; i < 16; i++) {
        fprintf(f, "%08X\n", proc->registers[i]);
    }
}

void write_dmemout(FILE* f, Processor* proc) {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        fprintf(f, "%08X\n", proc->dmem[i]);
    }
}

void write_diskout(FILE* f, Processor* proc) {
    for (int i = 0; i < DISK_SIZE; i++) {
        fprintf(f, "%08X\n", proc->disk[i]);
    }
}

void write_monitor(FILE* f_txt, FILE* f_yuv, Processor* proc) {
    // Write text format (monitor.txt)
    for (int y = 0; y < MONITOR_SIZE; y++) {
        for (int x = 0; x < MONITOR_SIZE; x++) {
            fprintf(f_txt, "%02X\n", proc->monitor_buffer[y][x]);
        }
    }

    // Write binary YUV format (monitor.yuv)
    for (int y = 0; y < MONITOR_SIZE; y++) {
        for (int x = 0; x < MONITOR_SIZE; x++) {
            fputc(proc->monitor_buffer[y][x], f_yuv);

        }
    }
    for (int i = 0; i < 256 * 256 * 2; i++) {
        fputc(128, f_yuv);
    }
}

/************************* Main Simulation Loop *************************/
void simulate(Processor* proc, char* argv[]) {
    // Open all input files
    FILE* imemin = fopen(argv[1], "r");
    FILE* dmemin = fopen(argv[2], "r");
    FILE* diskin = fopen(argv[3], "r");
    FILE* irq2in = fopen(argv[4], "r");

    // Open all output files
    FILE* dmemout = fopen(argv[5], "w");
    FILE* regout = fopen(argv[6], "w");
    FILE* trace = fopen(argv[7], "w");
    FILE* hwregtrace = fopen(argv[8], "w");
    FILE* cycles = fopen(argv[9], "w");
    FILE* leds = fopen(argv[10], "w");
    FILE* display7seg = fopen(argv[11], "w");
    FILE* diskout = fopen(argv[12], "w");
    FILE* monitor_txt = fopen(argv[13], "w");
    FILE* monitor_yuv = fopen(argv[14], "wb");  // Binary mode

    if (!imemin || !dmemin || !diskin || !irq2in ||
        !dmemout || !regout || !trace || !hwregtrace ||
        !cycles || !leds || !display7seg || !diskout ||
        !monitor_txt || !monitor_yuv) {
        fprintf(stderr, "Error: Cannot open one or more files\n");
        exit(1);
    }

    // Load initial states
    load_memory64(argv[1], proc->imem, MEMORY_SIZE, 12);  // Instructions
    load_memory32(argv[2], proc->dmem, MEMORY_SIZE, 8);   // Data
    load_memory32(argv[3], proc->disk, DISK_SIZE, 8);     // Disk

    // Load IRQ2 timing
    uint32_t irq2_timing[MEMORY_SIZE];
    int irq2_count = 0;
    load_irq2_timing(argv[4], irq2_timing, &irq2_count);

    // Previous state for tracking changes
    uint32_t prev_leds = 0;
    uint32_t prev_display = 0;

    // Main simulation loop
    while (!proc->halt) {
        // Update devices
        update_devices(proc);
        check_irq2(proc, irq2_timing, irq2_count);

        // Fetch instruction
        uint64_t inst = proc->imem[proc->pc];

        // Decode instruction
        Instruction decoded_inst = decode_instruction(inst);


        // **Update $imm1 and $imm2 before writing trace**
        proc->registers[1] = decoded_inst.immediate1;  // $imm1
        proc->registers[2] = decoded_inst.immediate2;  // $imm2

        // Write trace before execution
        write_trace(trace, proc, inst);

        // Execute instruction
        execute_instruction(proc, decoded_inst);

        // Handle IO operations tracing
        if (decoded_inst.opcode == 19) {  // in
            uint32_t addr = proc->registers[decoded_inst.rs] +
                proc->registers[decoded_inst.rt];
            write_hwregtrace(hwregtrace, proc->cycle_counter,
                io_register_names[addr], "READ",
                proc->registers[decoded_inst.rd]);
        }
        else if (decoded_inst.opcode == 20) {  // out
            uint32_t addr = proc->registers[decoded_inst.rs] +
                proc->registers[decoded_inst.rt];
            uint32_t value = proc->registers[decoded_inst.rm];
            write_hwregtrace(hwregtrace, proc->cycle_counter, io_register_names[addr],
                "WRITE", value);
        }

        // Update LED and display files if changed
        if (proc->leds != prev_leds) {
            fprintf(leds, "%u %08X\n", proc->cycle_counter, proc->leds);
            prev_leds = proc->leds;
        }
        if (proc->display7seg != prev_display) {
            fprintf(display7seg, "%u %08X\n", proc->cycle_counter,
                proc->display7seg);
            prev_display = proc->display7seg;
        }

        // Increment cycle counter
        proc->cycle_counter++;
    }

    // Write final states
    write_dmemout(dmemout, proc);
    write_regout(regout, proc);
    write_diskout(diskout, proc);
    write_monitor(monitor_txt, monitor_yuv, proc);
    fprintf(cycles, "%u", proc->cycle_counter);

    // Close all files
    fclose(imemin);
    fclose(dmemin);
    fclose(diskin);
    fclose(irq2in);
    fclose(dmemout);
    fclose(regout);
    fclose(trace);
    fclose(hwregtrace);
    fclose(cycles);
    fclose(leds);
    fclose(display7seg);
    fclose(diskout);
    fclose(monitor_txt);
    fclose(monitor_yuv);
}
/************************* Main Function *************************/
int main(int argc, char* argv[]) {
    if (argc != 15) {  // Program name + 14 arguments
        fprintf(stderr, "Usage: %s imemin.txt dmemin.txt diskin.txt irq2in.txt "
            "dmemout.txt regout.txt trace.txt hwregtrace.txt cycles.txt "
            "leds.txt display7seg.txt diskout.txt monitor.txt monitor.yuv\n",
            argv[0]);
        return 1;
    }

    // Initialize processor
    Processor proc;
    init_processor(&proc);

    // Run simulation
    simulate(&proc, argv);
    printf("Simulator completed successfully!\n");

    return 0;
}