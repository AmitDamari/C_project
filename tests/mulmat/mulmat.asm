# Matrix Multiplication Program (4x4 matrices)
# Input:  Matrix A (0x100-0x10F), Matrix B (0x110-0x11F)
# Output: Matrix C (0x120-0x12F) = A × B

# Initialize Matrix A (4x4)
.word 0x100 1    # Row 0: [1 2 3 4]
.word 0x101 2
.word 0x102 3
.word 0x103 4
.word 0x104 1    # Row 1: [1 1 1 1]
.word 0x105 1
.word 0x106 1
.word 0x107 1
.word 0x108 1    # Row 2: [1 1 1 1]
.word 0x109 1
.word 0x10A 1
.word 0x10B 1
.word 0x10C 1    # Row 3: [1 1 1 1]
.word 0x10D 1
.word 0x10E 1
.word 0x10F 1

# Initialize Matrix B (4x4)
.word 0x110 1    # Row 0: [1 2 3 4]
.word 0x111 2
.word 0x112 3
.word 0x113 4
.word 0x114 1    # Row 1: [1 1 1 1]
.word 0x115 1
.word 0x116 1
.word 0x117 1
.word 0x118 1    # Row 2: [1 1 1 1]
.word 0x119 1
.word 0x11A 1
.word 0x11B 1
.word 0x11C 1    # Row 3: [1 1 1 1]
.word 0x11D 1
.word 0x11E 1
.word 0x11F 1

# Main Program
# Initialize base addresses for matrices
add $s0, $zero, $imm1, $zero, 0x100, 0  # $s0 = Base address of Matrix A
add $s1, $zero, $imm1, $zero, 0x110, 0  # $s1 = Base address of Matrix B
add $s2, $zero, $imm1, $zero, 0x120, 0  # $s2 = Base address of Result Matrix C

# Outer loop - Iterate through rows of A
add $t0, $zero, $zero, $zero, 0, 0      # $t0 = row index i = 0
loop_i:
    beq $zero, $imm2, $t0, $imm1, end, 4  # Exit if i == 4

    # Middle loop - Iterate through columns of B
    add $t1, $zero, $zero, $zero, 0, 0    # $t1 = column index j = 0
    loop_j:
        beq $zero, $imm2, $t1, $imm1, next_i, 4  # Move to next row if j == 4

        # Initialize dot product accumulator
        add $v0, $zero, $zero, $zero, 0, 0    # $v0 = sum = 0
        add $t2, $zero, $zero, $zero, 0, 0    # $t2 = k = 0

        # Inner loop - Calculate dot product
        loop_k:
            beq $zero, $imm2, $t2, $imm1, store, 4  # Store result if k == 4

            # Calculate array offsets and load values
            mac $a0, $t0, $imm1, $t2, 4, 0    # $a0 = 4*i + k (offset for A[i][k])
            lw $a0, $s0, $a0, $zero, 0, 0     # $a0 = A[i][k]

            mac $a1, $t2, $imm1, $t1, 4, 0    # $a1 = 4*k + j (offset for B[k][j])
            lw $a1, $s1, $a1, $zero, 0, 0     # $a1 = B[k][j]

            # Multiply and accumulate: sum += A[i][k] * B[k][j]
            mac $v0, $a0, $a1, $v0, 0, 0

            # Increment k
            add $t2, $t2, $imm1, $zero, 1, 0
            beq $zero, $zero, $zero, $imm1, loop_k, 0

        store:
            # Calculate offset and store result in C[i][j]
            mac $a0, $t0, $imm1, $t1, 4, 0    # $a0 = 4*i + j (offset for C[i][j])
            sw $zero, $a0, $s2, $v0, 0, 0     # C[i][j] = sum

            # Increment j
            add $t1, $t1, $imm1, $zero, 1, 0
            beq $zero, $zero, $zero, $imm1, loop_j, 0

    next_i:
        # Increment i
        add $t0, $t0, $imm1, $zero, 1, 0
        beq $zero, $zero, $zero, $imm1, loop_i, 0

end:
    halt $zero, $zero, $zero, $zero, 0, 0