#include <stdio.h>

extern int hash(const char * string);
extern int addmod();
extern int fib(int num);
extern char bit_xor(const char* string);

int main() {
    char* string = "A39b";
	
    printf("Hash value is: %d\n" , hash(string));

		int num = addmod();
	
		printf("Hash value mod 7 is: %d\n", num);
		
		printf("The fibonacci sequence of %d is: %d\n", num, fib(num));

		printf("Bitwise XOR of characters is: %u", bit_xor(string));

    return 0;
}
