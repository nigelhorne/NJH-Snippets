/*
 * Example program for the zlib library
 *	cc zlib.c -lz
 *
 * Author Nigel Horne <njh@nigelhorne.com>
 */
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main()
{
	/*const char foo[] = "jLSlMbEYnBAzXOfZJHBV+COBjvVdDSOmpvacu25IkUsgIAIHXirU9iJLwM4LLnHBxiqWgktyRrZI";*/
	const char foo[] = "AAEAAAADAAAAAAAAADIwMDM6MDQ6MzAgMTc6MzE6MzMAMjAwMzowNDozMCAxNzozMTozMwAAAAAA";
	char *bar;
	char *foo2;
	uLongf f, g;

	f = compressBound(sizeof(foo));

	printf("%d->%d", strlen(foo), f);

	bar = malloc(f);

	switch(compress2(bar, &f, foo, sizeof(foo), 9)) {
		case Z_OK:
			break;
		case Z_MEM_ERROR:
			return 1;
		case Z_BUF_ERROR:
			return 2;
		case Z_DATA_ERROR:
			return 3;
	}

	printf("->%d\n", f);

	if(f > sizeof(foo))
		puts("Hasn't made it smaller");

	g = sizeof(foo) + 1;
	
	foo2 = malloc(g);

	switch(uncompress(foo2, &g, bar, f)) {
		case Z_OK:
			break;
		case Z_MEM_ERROR:
			return 1;
		case Z_BUF_ERROR:
			return 2;
		case Z_DATA_ERROR:
			return 3;
	}

	printf("%s (%lu)\n", foo2, g);
}
