#include <stdio.h>
main()
{
	printf("X%.5sX\n", "hello, world");
	printf("X%.*sX\n", 5, "hello, world");
	printf("X%-5sX\n", "hello, world");
	printf("X%-5.5sX\n", "hello, world");
	printf("X%7.5sX\n", "hello, world");
	printf("X%5.7sX\n", "hello, world");
	printf("X%-5.7sX\n", "hello, world");
	printf("X%20.5sX\n", "hello, world");
	printf("X%-20.5sX\n", "hello, world");
	printf("X%5.20sX\n", "hello, world");
	printf("X%-20sX\n", "hello, world");
	putchar('\n');
	printf("X%.2fX\n", 15.0);
	printf("X%9.2fX\n", 15.0);
	printf("X%-.2fX\n", 15.0);
	printf("X%-9.2fX\n", 15.0);
	printf("X%02xX\n", 9);
	printf("X%02XX\n", 10);
	printf("X%02xX\n", 10);
}
