.global hash
.global addmod
.global fib
.global bit_xor
.p2align 2

.type hash, %function
.type addmod, %function
.type fib, %function
.type bit_xor, %function
	
.fnstart
// Through this code: r1 is used for the hash value
// r2 has the current char of the string 

hash:
	push {r5}
    mov r1, #0	// Length counter and loop itterator
	mov r5, r0	// Store input's string starting memory possition

hash_loop:
    ldrb r2, [r0], #1	// Load byte from the address of r0 into r2 and then increment r0 by 1 byte
    cmp r2, #0			// Compare r2 to 0 and set Zero flag
    beq hash_out		// If Zero flag is set, Branch to out
	add r1, r1, #1		// Loop advances to the next itteration (one more char exists)
    b hash_loop			// Branch to the start of the loop 

hash_out:
	ldr r0, =0x20001000 // r0 points to a place in memory where the correlation table will be stored 
	// Table Set
	mov r3, #5			// Digit: 0  |  Value: 5  
	str r3, [r0]	  	// Save value into the address of r0 and increment address by 1 byte (1st byte)
	mov r3, #12			// Digit: 1  |  Value: 12  
	str r3, [r0, #1]  	// Save value into the address of r0 and increment address by 1 byte (2nd byte) 
	mov r3, #7			// Digit: 2  |  Value: 7  
	str r3, [r0, #2]  	// Save value into the address of r0 and increment address by 1 byte (3rd byte)
	mov r3, #6			// Digit: 3  |  Value: 6  
	str r3, [r0, #3]  	// Save value into the address of r0 and increment address by 1 byte (4th byte)
	mov r3, #4			// Digit: 4  |  Value: 4  
	str r3, [r0, #4]  	// Save value into the address of r0 and increment address by 1 byte (5th byte)
	mov r3, #11			// Digit: 5  |  Value: 11  
	str r3, [r0, #5] 	// Save value into the address of r0 and increment address by 1 byte (6th byte)
	mov r3, #6			// Digit: 6  |  Value: 6  
	str r3, [r0, #6]  	// Save value into the address of r0 and increment address by 1 byte (7th byte) 
	mov r3, #3			// Digit: 7  |  Value: 3  
	str r3, [r0, #7]  	// Save value into the address of r0 and increment address by 1 byte (8th byte)
	mov r3, #10			// Digit: 8  |  Value: 10  
	str r3, [r0, #8]  	// Save value into the address of r0 and increment address by 1 byte (9th byte) 
	mov r3, #23			// Digit: 9  |  Value: 23  
	str r3, [r0, #9]  	// Save value into the address of r0 	
  
hash_addition:    
    ldrb r2, [r5], #1    
    cmp r2, #0            
    beq output            
	/*	
	/	The comparison here is done in an asceding
	/	format based on the order of digits in the ASCII table
	/	<48   : Skip digit
	/	<58   : Decimal number digit
	/	<65   : Skip digit
	/	<91   : Capital letter digit
	/	<97   : Skip digit
	/	<123  : Lower case letter digit
	/	>=123 : Skip digit
	*/
    cmp r2, #48
    blt hash_addition     
    cmp r2, #58           
    blt number_detected   
    cmp r2, #65           
    blt hash_addition     
    cmp r2, #91           
    blt cap_detected      
    cmp r2, #97           
    blt hash_addition     
    cmp r2, #123          
    blt low_detected      
    b hash_addition
	
// From now on, r1 will store the sumation of calculated hashes
//			and r3 will store the value to be added to the final sum (r1)
number_detected:		// Current digit is a number
    sub r3, r2, #48		// 48 is the equivalent of 0 in ASCII
						// -> r3 contains the detected number from 0 to 9
	ldrb r3, [r0, r3]	// Load the byte from the adjusted memory address into r3	
    add r1, r1, r3      // Add calculated value to r1
    b hash_addition     // Go to the next digit
    
cap_detected:       	// Current digit is a capital letter
    lsl r3, r2, #1      // Equivalent to x2 multiplication
    add r1, r1, r3		// Add calculated value to r1
    b hash_addition     // Go to the next digit
    
low_detected:			// Current digit is a lower case letter
    sub r3, r2, #97     // Subtract 97
    mul r3, r3, r3      // Calculate the square : (ASCII - 97)^2
    add r1, r1, r3      // Add calculated value to r1
    b hash_addition     // Go to the next digit
    
output:
    ldr r0, =0x20001100 // Load a specific address RAM to r0
    str r1, [r0]		// Store r1 into the content of r0
    mov r0, r1          // Move the hash value from r1 to r0 
    pop {r5}			// Restore r5 from the stack (cleanup stack)
	bx lr				// Return to the caller
.fnend


.fnstart
addmod:
	mov r2, #0			// r2 has the number 0 stored
    ldr r0, =0x20001100	// r0 points to 0x20001100 where the value of the hash is now stored
	ldrb r1, [r0]		// Load the value of the hash from memory to r1
	cmp r1, #10			// Compare r1 to 10
	bmi exit			// If Negative flag is set this means the hash value is <10 thus, Branch to exit
	mov r3, r1			// Store the initial value of r1 to r3
	mov r0, #10 		// r0 has the number 10
digits_addition:
    subs r1, r1, #10	// Subtract r1-10 and raise flags
    bge digits_addition // If r1>10 Branch to digits_addition
digit_found:
    add r1, r1, #10     // Add 10 to r1, now r1 contains the digit
    add r2, r2, r1      // Add this digit's value to the value
    udiv r3, r3, r0     // Divide r3 by 10 to remove the rightmost digit
    cmp r3, #0			// Compare the the new number to 0 	eg. If 9/10=0 , there are no more digits
    beq mod7			// If new number is zero -> Continue to next step : Modulo of 7
    mov r1, r3			// Else, move new number to r1
    b digits_addition   // Branch to digits_addition and pursue next digit
mod7:
	subs r2, r2, #7		// Subtract 7 from the single digit hash value
	bge mod7			// If r2 > 7 have not found hash value %7 thus, Branch to mod 7
	// mod7 is found:
	add r2, r2, #7 		// Add 7 to r2, now r1 contains hash value %7
	ldr r0, =0x20001110	// r0 points to 0x20001110 where the value of the hash %7 is stored
	str r2, [r0]		// Store hash value %7 in memory
	mov r0, r2			// Load r2 value to r0	
	bx lr				// Return to caller
exit:
	// Decision was made to return number 7 if hash returns < 10 
	ldr r0, =0x20001110	// r0 points to 0x20001110 where the value of the hash %7 is stored
	mov r2, #7			// Load number 7 to r2 to store on memory
	str r2, [r0]		// Store hash value %7 in memory
	mov r0, #7			// Load number 7 to r0 to return
	bx lr				// Return to caller
.fnend
    
	



.fnstart
	fib:
	push {r4, r5, lr}	// Save registers r4, r5 and lr
    cmp r0, #1			// Compare input n with 1
    ble fibout			// Branch to fibout if input is less-than or equal to 1
	// fib(n-1)
    sub r0, r0, #1		// Compute n-1
    mov r4, r0			// Save n-1 in r4
    bl fib				// Compute fib(n-1) and return to next insn
    mov r5, r0      	// Store fib(n-1) in r5
	// fib(n-2)
    sub r0, r4, #1		// Compute n-2 using preserved n-1 in r4
    bl fib				// Compute fib(n-2) and return to the next insn
	// fib(n)
    add r0, r5, r0		// Sum fib(n-1) + fib(n-2)

fibout:
	ldr	r1, =0x20001120
	str r0, [r1]
	pop {r4, r5, lr}	// Restore saved registers
    bx lr				// Return to caller
.fnend



.fnstart    

bit_xor:
	ldrb r2, [r0], #1	// Load byte from the address of r0 into r2 and the increment r0 by 1 byte    
bit_xor_mid:
	ldrb r1, [r0], #1
	cmp r1, #0			// Compare r1 to 0 and set Zero flag
    beq bit_out			// If Zero flag is set, Branch to bit_out
	EOR r2, r2, r1 		// r2 has the bit wise XOR of current traversed carachters
	b bit_xor_mid 		// Branch to bit_xor_mid
bit_out:
	ldr r0, =0x20001120	// r0 points to 0x20001120 where the bitwise xor will be stored
	str r2, [r0]		// Store bitwise xor in memory
	mov r0, r2			// Move the bit wise XOR to r0 to return
	bx lr				// Return to caller
.fnend
