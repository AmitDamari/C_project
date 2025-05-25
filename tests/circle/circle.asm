# Initialize stack pointer to 0x800
sll $sp, $imm1, $imm2, $zero, 1, 11      # Set stack pointer to 0x800

# Load circle parameters
lw $s0, $imm1, $zero, $zero, 256, 0      # Load radius from memory address 0x100
add $s1, $zero, $imm1, $zero, 0, 0       # Initialize Y coordinate to 0

column:
    # Y coordinate loop (0 to 255)
    beq $zero, $s1, $imm1, $imm2, 256, exit1 # Exit if Y >= 256
    add $s2, $zero, $zero, $zero, 0, 0       # Initialize X coordinate to 0

row:
    # X coordinate loop (0 to 255)
    beq $zero, $s2, $imm1, $imm2, 256, exit2 # Exit if X >= 256
    
    # Calculate distance from center (127,127)
    sub $a0, $s1, $imm1, $zero, 127, 0       # dy = Y - center_y
    sub $a1, $s2, $imm1, $zero, 127, 0       # dx = X - center_x
    
    # Calculate (dx^2 + dy^2) for circle equation
    mac $a0, $a0, $a0, $zero, 0, 0           # dy^2
    mac $a1, $a1, $a1, $zero, 0, 0           # dx^2
    add $t0, $a0, $a1, $zero, 0, 0           # distance^2 = dx^2 + dy^2
    
    # Compare with radius^2
    mac $a1, $s0, $s0, $zero, 0, 0           # radius^2
    bgt $zero, $t0, $a1, $imm2, 0, continue  # Skip if point is outside circle

    # Draw pixel if inside circle
    add $a2, $s1, $zero, $zero, 0, 0         # Set Y coordinate
    add $a0, $s2, $zero, $zero, 0, 0         # Set X coordinate
    jal $ra, $zero, $zero, $imm2, 0, put     # Call pixel drawing subroutine

continue:
    add $s2, $s2, $imm1, $zero, 1, 0         # Increment X
    beq $zero, $zero, $zero, $imm2, 0, row   # Continue row loop

exit2:
    add $s1, $s1, $imm1, $zero, 1, 0         # Increment Y
    beq $zero, $zero, $zero, $imm2, 0, column # Continue column loop

exit1:
    halt $zero, $zero, $zero, $zero, 0, 0     # End program

put:
    # Draw pixel subroutine
    sll $t0, $a2, $imm1, $zero, 8, 0         # Calculate pixel offset (Y * 256)
    add $t1, $t0, $a0, $zero, 0, 0           # Add X offset
    out $zero, $imm1, $zero, $t1, 20, 0      # Set pixel address
    out $zero, $imm1, $zero, $imm2, 21, 255  # Set pixel color (white)
    out $zero, $imm1, $zero, $imm2, 22, 1    # Write pixel
    beq $zero, $zero, $zero, $ra, 0, 0       # Return

.word 0x100 100                               # Circle radius value