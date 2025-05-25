/*****************************************************************
 * Assembler for SIMP Processor
 * Complete implementation with two-pass assembly
 * Handles 6-operand instruction format with register immediates
 * Citation: Architecture based on provided SIMP processor specification
 *****************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

 /************************* Constants *************************/
#define MAX_LINE_LENGTH 500
#define MAX_LABEL_LENGTH 50
#define MEMORY_SIZE 4096
#define MAX_IMMEDIATE 2048     // 11-bit immediate value limit

/************************* Data Structures *************************/
typedef struct Label {
    char name[MAX_LABEL_LENGTH];
    int address;
    struct Label* next;
} Label;

/************************* Function Prototypes *************************/
Label* create_label(const char* name, int address);
Label* add_label(Label* head, const char* name, int address);
int find_label(Label* head, const char* name);
Label* first_pass(FILE* input);
void second_pass(FILE* input, FILE* imemin, FILE* dmemin, Label* labels);
int get_register_number(const char* reg);
int get_opcode_number(const char* opcode);
void cleanup_labels(Label* head);
void trim(char* str);
int is_number(const char* str);
int parse_immediate(const char* imm, Label* labels, int bit_size);
int parse_immediate_signed(const char* imm, Label* labels, int bit_size);

/************************* Utility Functions *************************/
void trim(char* str) {
    char* start = str;
    char* end;

    while (isspace((unsigned char)*start)) start++;

    if (*start == 0) {
        *str = 0;
        return;
    }

    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;

    memmove(str, start, end - start + 2);
}

int is_number(const char* str) {
    if (!str || !*str) return 0;

    if (*str == '-') str++;
    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
        str += 2;
        while (*str) {
            if (!isxdigit((unsigned char)*str)) return 0;
            str++;
        }
        return 1;
    }
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

int parse_immediate(const char* imm, Label* labels, int bit_size) {
    if (!imm || !*imm) return 0;

    long value = 0;

    // Check if it's a register
    if (imm[0] == '$') {
        int reg_num = get_register_number(imm);
        if (reg_num == -1) {
            fprintf(stderr, "Error: Invalid register name %s\n", imm);
            exit(1);
        }
        return reg_num;
    }

    // Check if it's a number
    if (is_number(imm)) {
        value = strtol(imm, NULL, 0);
        if (value < 0) {
            // Handle negative values using two's complement
            value = (1 << bit_size) + value;
        }
    }
    else {
        // Must be a label
        int label_addr = find_label(labels, imm);
        if (label_addr == -1) {
            fprintf(stderr, "Error: Undefined label %s\n", imm);
            exit(1);
        }
        value = label_addr;
    }

    // Mask the immediate to the correct size
    value &= (1 << bit_size) - 1;

    return value;
}

int parse_immediate_signed(const char* imm, Label* labels, int bit_size) {
    if (!imm || !*imm) return 0;

    long value = 0;

    // Check if it's a register
    if (imm[0] == '$') {
        int reg_num = get_register_number(imm);
        if (reg_num == -1) {
            fprintf(stderr, "Error: Invalid register name %s\n", imm);
            exit(1);
        }
        return reg_num;
    }

    // Check if it's a number
    if (is_number(imm)) {
        value = strtol(imm, NULL, 0);
        // No need to adjust negative values, as they will be represented correctly in two's complement
    }
    else {
        // Must be a label
        int label_addr = find_label(labels, imm);
        if (label_addr == -1) {
            fprintf(stderr, "Error: Undefined label %s\n", imm);
            exit(1);
        }
        value = label_addr;
    }

    // Mask the immediate to the correct size (assuming two's complement)
    value = ((value + (1 << bit_size)) % (1 << bit_size));

    return value;
}

/************************* Label Management Functions *************************/
Label* create_label(const char* name, int address) {
    Label* new_label = (Label*)malloc(sizeof(Label));
    if (!new_label) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(1);
    }

    strncpy(new_label->name, name, MAX_LABEL_LENGTH - 1);
    new_label->name[MAX_LABEL_LENGTH - 1] = '\0';
    new_label->address = address;
    new_label->next = NULL;
    return new_label;
}

Label* add_label(Label* head, const char* name, int address) {
    Label* new_label = create_label(name, address);
    new_label->next = head;
    return new_label;
}

int find_label(Label* head, const char* name) {
    while (head) {
        if (strcmp(head->name, name) == 0) {
            return head->address;
        }
        head = head->next;
    }
    return -1;
}
/************************* Cleanup Functions *************************/
void cleanup_labels(Label* head) {
    while (head) {
        Label* temp = head;
        head = head->next;
        free(temp);
    }
}
/************************* Instruction Processing Functions *************************/
int get_register_number(const char* reg) {
    if (!reg || !*reg) return -1;

    static const char* registers[] = {
        "$zero", "$imm1", "$imm2", "$v0",
        "$a0", "$a1", "$a2", "$t0",
        "$t1", "$t2", "$s0", "$s1",
        "$s2", "$gp", "$sp", "$ra"
    };

    for (int i = 0; i < 16; i++) {
        if (strcmp(reg, registers[i]) == 0) {
            return i;
        }
    }

    // Handle registers like "$0", "$1", etc.
    if (reg[0] == '$' && is_number(reg + 1)) {
        int reg_num = atoi(reg + 1);
        if (reg_num >= 0 && reg_num <= 15) {
            return reg_num;
        }
    }

    return -1;
}

int get_opcode_number(const char* opcode) {
    if (!opcode || !*opcode) return -1;

    static const struct {
        const char* name;
        int number;
    } opcodes[] = {
        {"add", 0x00},      // 0
        {"sub", 0x01},      // 1
        {"mac", 0x02},      // 2
        {"and", 0x03},      // 3
        {"or",  0x04},      // 4
        {"xor", 0x05},      // 5
        {"sll", 0x06},      // 6
        {"sra", 0x07},      // 7
        {"srl", 0x08},      // 8
        {"beq", 0x09},      // 9
        {"bne", 0x0A},      // 10
        {"blt", 0x0B},      // 11
        {"bgt", 0x0C},      // 12
        {"ble", 0x0D},      // 13
        {"bge", 0x0E},      // 14
        {"jal", 0x0F},      // 15
        {"lw",  0x10},      // 16
        {"sw",  0x11},      // 17
        {"reti",0x12},      // 18
        {"in",  0x13},      // 19
        {"out", 0x14},      // 20
        {"halt",0x15}       // 21
    };

    for (int i = 0; i < sizeof(opcodes) / sizeof(opcodes[0]); i++) {
        if (strcmp(opcode, opcodes[i].name) == 0) {
            return opcodes[i].number;
        }
    }
    return -1;
}
/************************* First Pass Implementation *************************/
Label* first_pass(FILE* input) {
    char line[MAX_LINE_LENGTH];
    int current_address = 0;
    Label* label_list = NULL;

    rewind(input);

    while (fgets(line, MAX_LINE_LENGTH, input)) {
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';  // Remove comments

        trim(line);
        if (strlen(line) == 0) continue;  // Skip empty lines

        // Handle .word directive
        if (strstr(line, ".word") != NULL) {
            continue; // Do not increment current_address for .word directives
        }

        // Check for labels
        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char label_name[MAX_LABEL_LENGTH];
            strncpy(label_name, line, MAX_LABEL_LENGTH - 1);
            trim(label_name);
            label_list = add_label(label_list, label_name, current_address);

            // Check if there's an instruction after the label
            char* instruction = colon + 1;
            trim(instruction);
            if (strlen(instruction) == 0) continue;
            strcpy(line, instruction); // Process the instruction after label
        }

        // Increment current address only if it's an instruction
        if (strlen(line) > 0 && strstr(line, ".word") == NULL) {
            current_address++;
        }
    }

    return label_list;
}

/************************* Second Pass Implementation *************************/
void second_pass(FILE* input, FILE* imemin, FILE* dmemin, Label* labels) {
    char line[MAX_LINE_LENGTH];
    int current_address = 0;
    int dmem[MEMORY_SIZE] = { 0 };
    int max_dmem_address = 64;

    rewind(input);

    while (fgets(line, MAX_LINE_LENGTH, input)) {
        char original_line[MAX_LINE_LENGTH];
        strcpy(original_line, line);

        // Remove comments
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        trim(line);
        if (strlen(line) == 0) continue;

        // Handle .word directive
        if (strstr(line, ".word") != NULL) {
            char* token = strtok(line, " \t");  // Get .word
            token = strtok(NULL, " \t");        // Get address
            int word_address = strtol(token, NULL, 0);
            token = strtok(NULL, " \t");        // Get value

            if (token) {
                trim(token);
                int value = strtol(token, NULL, 0);
                dmem[word_address] = value;
                if (word_address > max_dmem_address) {
                    max_dmem_address = word_address;
                }
            }
            continue;
        }

        // Skip label definitions
        char* colon = strchr(line, ':');
        if (colon) {
            char* instruction = colon + 1;
            trim(instruction);
            if (strlen(instruction) == 0) continue;
            strcpy(line, instruction);
        }

        // Initialize default operands
        char opcode[6] = "", rd[6] = "$zero", rs[6] = "$zero", rt[6] = "$zero", rm[6] = "$zero";
        char imm1[50] = "0", imm2[50] = "0";

        // Parse instruction
        char* token = strtok(line, " \t,");

        if (token) {
            strncpy(opcode, token, 5);
            opcode[5] = '\0';
            token = strtok(NULL, " \t,");

            if (token) {  // rd
                strncpy(rd, token, 5);
                rd[5] = '\0';
                token = strtok(NULL, " \t,");

                if (token) {  // rs
                    strncpy(rs, token, 5);
                    rs[5] = '\0';
                    token = strtok(NULL, " \t,");

                    if (token) {  // rt
                        strncpy(rt, token, 5);
                        rt[5] = '\0';
                        token = strtok(NULL, " \t,");

                        if (token) {  // rm
                            strncpy(rm, token, 5);
                            rm[5] = '\0';
                            token = strtok(NULL, " \t,");

                            if (token) {  // imm1
                                strncpy(imm1, token, 49);
                                imm1[49] = '\0';
                                token = strtok(NULL, " \t,");

                                if (token) {  // imm2
                                    strncpy(imm2, token, 49);
                                    imm2[49] = '\0';
                                }
                            }
                        }
                    }
                }
            }
        }

        // Process instruction
        if (strlen(opcode) > 0) {
            int opcode_num = get_opcode_number(opcode);
            if (opcode_num == -1) {
                fprintf(stderr, "Error: Invalid opcode %s\n", opcode);
                exit(1);
            }

            int rd_num = get_register_number(rd);
            int rs_num = get_register_number(rs);
            int rt_num = get_register_number(rt);
            int rm_num = get_register_number(rm);

            // Parse immediates
            int imm1_value = 0;
            int imm2_value = 0;

            // Handle imm1
            if (imm1[0] == '$') {  // If it's a register
                imm1_value = get_register_number(imm1);
                if (imm1_value == -1) {
                    fprintf(stderr, "Error: Invalid register %s\n", imm1);
                    exit(1);
                }
            }
            else if (is_number(imm1)) {  // If it's a number
                imm1_value = strtol(imm1, NULL, 0);
                if (imm1_value < 0) {
                    imm1_value = (1 << 12) + imm1_value;
                }
            }
            else {  // Must be a label
                imm1_value = find_label(labels, imm1);
                if (imm1_value == -1) {
                    fprintf(stderr, "Error: Undefined label %s\n", imm1);
                    exit(1);
                }
            }

            // Handle imm2
            if (imm2[0] == '$') {  // If it's a register
                imm2_value = get_register_number(imm2);
                if (imm2_value == -1) {
                    fprintf(stderr, "Error: Invalid register %s\n", imm2);
                    exit(1);
                }
            }
            else if (is_number(imm2)) {  // If it's a number
                imm2_value = strtol(imm2, NULL, 0);
                if (imm2_value < 0) {
                    imm2_value = (1 << 12) + imm2_value;
                }
            }
            else {  // Must be a label
                imm2_value = find_label(labels, imm2);
                if (imm2_value == -1) {
                    fprintf(stderr, "Error: Undefined label %s\n", imm2);
                    exit(1);
                }
            }

            // Ensure values are within their bit ranges
            opcode_num &= 0xFF;    // 8 bits
            rd_num &= 0xF;         // 4 bits
            rs_num &= 0xF;         // 4 bits
            rt_num &= 0xF;         // 4 bits
            rm_num &= 0xF;         // 4 bits
            imm1_value &= 0xFFF;   // 12 bits
            imm2_value &= 0xFFF;   // 12 bits

            // Format and write the instruction
            fprintf(imemin, "%02X%01X%01X%01X%01X%03X%03X\n",
                opcode_num,
                rd_num,
                rs_num,
                rt_num,
                rm_num,
                imm1_value,
                imm2_value);

            current_address++;
        }
    }

    // Write data memory contents
    for (int i = 0; i <= max_dmem_address; i++) {
        fprintf(dmemin, "%08X\n", dmem[i]);
    }
}

/************************* Main Function *************************/
int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input.asm> <imemin.txt> <dmemin.txt>\n", argv[0]);
        fprintf(stderr, "Example: assembler program.asm imemin.txt dmemin.txt\n");
        return 1;
    }

    // Open input assembly file
    FILE* input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file %s\n", argv[1]);
        return 1;
    }

    // Open output files
    FILE* imemin = fopen(argv[2], "w");
    if (!imemin) {
        fprintf(stderr, "Error: Cannot open output file %s\n", argv[2]);
        fclose(input);
        return 1;
    }

    FILE* dmemin = fopen(argv[3], "w");
    if (!dmemin) {
        fprintf(stderr, "Error: Cannot open output file %s\n", argv[3]);
        fclose(input);
        fclose(imemin);
        return 1;
    }

    // First pass - collect labels
    Label* labels = first_pass(input);
    if (!labels && ferror(input)) {
        fprintf(stderr, "Error: First pass failed\n");
        fclose(input);
        fclose(imemin);
        fclose(dmemin);
        return 1;
    }

    // Second pass - generate machine code
    second_pass(input, imemin, dmemin, labels);

    // Cleanup
    fclose(input);
    fclose(imemin);
    fclose(dmemin);

    printf("Assembly completed successfully!\n");
    return 0;
}