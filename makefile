debug:
	gcc -Og -g -Wall -Wextra -pedantic -fsanitize=leak,address,undefined main.c -o uefi-disk -luuid -lz
