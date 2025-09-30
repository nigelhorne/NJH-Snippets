#include <stdio.h>

static int
hex2int(const unsigned char *src)
{
	unsigned char ret;

	if((*src >= '0') && (*src <= '9'))
		ret = *src++ - '0';
	else {
		char c = tolower(*src);
		ret = c - 'a' + 10;
		src++;
	}

	ret <<= 4;

	if((*src >= '0') && (*src <= '9'))
		return ret | (*src - '0');
	return ret | (tolower(*src) - 'a' + 10);
}

main()
{
	printf("%d\n", hex2int("00"));
	printf("%d\n", hex2int("A0"));
	printf("%d\n", hex2int("a0"));
	printf("%d\n", hex2int("0a"));
	printf("%d\n", hex2int("0A"));
}
