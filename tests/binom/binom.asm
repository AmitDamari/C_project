# Initialize program
# Load n and k from memory
lw $a0, $zero, $imm1, $zero, 0x100, 0    # Load n into $a0
lw $a1, $zero, $imm1, $zero, 0x101, 0    # Load k into $a1
jal $ra, $zero, $zero, $imm1, binom, 0   # Call binom(n,k)
sw $zero, $imm1, $zero, $v0, 0x102, 0    # Store result in memory
halt $zero, $zero, $zero, $zero, 0, 0     # End program

# Function binom(n,k)
# $a0 = n, $a1 = k, returns result in $v0
binom:
    # Save registers on stack
    sub $sp, $sp, $imm1, $zero, 3, 0      # Allocate stack space
    sw $sp, $imm1, $zero, $ra, 0, 0       # Save return address
    sw $sp, $imm1, $zero, $a0, 1, 0       # Save n
    sw $sp, $imm1, $zero, $a1, 2, 0       # Save k

    # Check base cases
    # if (k == 0) return 1
    bne $zero, $a1, $zero, $imm1, not_zero, 0
    add $v0, $imm1, $zero, $zero, 1, 0    # Return 1
    beq $zero, $zero, $zero, $imm1, return, 0

not_zero:
    # if (n == k) return 1
    bne $zero, $a0, $a1, $imm1, recurse, 0
    add $v0, $imm1, $zero, $zero, 1, 0    # Return 1
    beq $zero, $zero, $zero, $imm1, return, 0

recurse:
    # First recursive call: binom(n-1, k-1)
    sub $a0, $a0, $imm1, $zero, 1, 0      # n-1
    sub $a1, $a1, $imm1, $zero, 1, 0      # k-1
    jal $t0, $zero, $zero, $imm1, binom, 0
    add $t1, $v0, $zero, $zero, 0, 0      # Save result in $t1

    # Second recursive call: binom(n-1, k)
    lw $a0, $sp, $imm1, $zero, 1, 0       # Restore n
    lw $a1, $sp, $imm1, $zero, 2, 0       # Restore k
    sub $a0, $a0, $imm1, $zero, 1, 0      # n-1
    jal $t0, $zero, $zero, $imm1, binom, 0

    # Add results
    add $v0, $v0, $t1, $zero, 0, 0        # $v0 = binom(n-1,k) + binom(n-1,k-1)

return:
    # Restore registers and return
    lw $ra, $sp, $imm1, $zero, 0, 0       # Restore return address
    add $sp, $sp, $imm1, $zero, 3, 0      # Deallocate stack space
    beq $zero, $zero, $zero, $ra, 0, 0    # Return

# Initialize memory with test values
.word 0x100 5    # n = 5
.word 0x101 2    # k = 2
.word 0x102 0    # Result will be stored here