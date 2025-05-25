# Disk Test Program (disktest.asm)
# Moves contents of sectors 0-7 forward by one sector
# Each sector is 512 bytes (128 words)
# Uses a buffer at address 0x100 for temporary storage

add $s0, $zero, $imm1, $zero, 0x100, 0       # Initialize buffer address to 0x100
add $s1, $zero, $imm1, $zero, 7, 0           # Initialize sector counter to 7 (start from last sector)

main_loop:
out $zero, $zero, $imm1, $s1, 15, 0          # Set source sector number (register 15: disksector)
out $zero, $zero, $imm1, $s0, 16, 0          # Set buffer address (register 16: diskbuffer)

add $t0, $zero, $imm1, $zero, 1, 0           # Prepare read command (1)
out $zero, $zero, $imm1, $t0, 14, 0          # Issue read command (register 14: diskcmd)

add $t0, $zero, $imm1, $zero, 1, 0           # Enable disk interrupt
out $zero, $zero, $imm1, $t0, 1, 0           # Set irq1enable

wait_read:
in $t0, $zero, $imm1, $zero, 17, 0           # Read disk status (register 17: diskstatus)
bne $zero, $t0, $zero, $imm1, wait_read, 0   # If disk busy (status != 0), keep waiting

add $t0, $zero, $zero, $zero, 0, 0           # Prepare to clear interrupt
out $zero, $zero, $imm1, $t0, 4, 0           # Clear irq1status

add $t1, $s1, $imm1, $zero, 1, 0             # Calculate destination sector (current + 1)
out $zero, $zero, $imm1, $t1, 15, 0          # Set destination sector number

out $zero, $zero, $imm1, $s0, 16, 0          # Set buffer address again

add $t0, $zero, $imm1, $zero, 2, 0           # Prepare write command (2)
out $zero, $zero, $imm1, $t0, 14, 0          # Issue write command

wait_write:
in $t0, $zero, $imm1, $zero, 17, 0           # Read disk status
bne $zero, $t0, $zero, $imm1, wait_write, 0  # If disk busy, keep waiting

add $t0, $zero, $zero, $zero, 0, 0           # Prepare to clear interrupt
out $zero, $zero, $imm1, $t0, 4, 0           # Clear irq1status

sub $s1, $s1, $imm1, $zero, 1, 0             # Decrement sector counter

bge $zero, $s1, $zero, $imm1, main_loop, 0   # If sector >= 0, continue loop

add $t0, $zero, $zero, $zero, 0, 0           # Prepare to disable interrupt
out $zero, $zero, $imm1, $t0, 1, 0           # Disable disk interrupt (irq1enable = 0)

halt $zero, $zero, $zero, $zero, 0, 0         # End program