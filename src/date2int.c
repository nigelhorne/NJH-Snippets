#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

/*
 * twobytes2char -
 *	e.g. twobytes2char("88") returns 88.
 *	In this context char means a number -128 -> 127
 * Assumes twobytes is right justified
 */
char
twobytes2char(twobytes)
register const char *twobytes;
{
	if(isdigit(twobytes[0]))
		return((char)(((twobytes[0] - '0') * 10) + (twobytes[1] - '0')));
	else
		return((char)((isdigit(twobytes[1])) ? (twobytes[1] - '0') : 0));
}

/*
 * char2twobytes -
 *	opposite of the above function
 */
void
char2twobytes(ch, twobytes)
char ch, *twobytes;
{
	ch %= 100; /* 130599 */
	twobytes[0] = (char)((ch < 10) ? ' ' : (ch / 10) + '0');
	twobytes[1] = (char)((ch % 10) + '0');
}

/*
 * Magic numbers for date/int conversion
 */
static	const	int	M0[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
}, M1[] = {
	0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
};

static	const	*errstring;

/*
 * today:
 *	Return today's date in "conventional" format
 */
const char *
today(ptr)
char *ptr;
{
	register struct tm *tm;
	time_t ltim;
	static char ret[9];

	if(ret[0] == '\0') {	/* cache */
		time(&ltim);
		tm = localtime(&ltim);
		sprintf(ret, "%02d/%02d/%02d", tm->tm_mday, tm->tm_mon+1, tm->tm_year % 100);
	}
	if(ptr)
		strcpy(ptr, ret);
	return(ret);
}

/*
 * date2int & int2date -
 *	Routines courtesy of computing, stash dates in ints, easier to
 * calculate date differences than the long method
 *
 * Dates aren't expected before 1970, so 01 will be taken to be 2001 etc.
 */
date2int(const char *date)
{
	register int y, m, d;

	d = twobytes2char(&date[0]);
	m = twobytes2char(&date[3]) - 1;
	y = twobytes2char(&date[6]) + 1900;

	if(y < 1970)
		y += 100;
	else
		puts("Warning 20th Century date assumed");

	if((y & 3) == 0)
		/* leap year */
		return(((y - 1973)>>2)*1461 + 1095 + M1[m] + d);
	else
		return(((y - 1973)>>2)*1461 + ((y - 1) & 3)*365 + M0[m] + d);
}

const char *
int2date(i)
int i;
{
	register int dd, y;
	register unsigned char m, d;
	static char date[9] = {
		' ', ' ', '/', ' ', ' ', '/', ' ', ' ', '\0'
	};

	if(i < 0)	/* illegal date */
		/* trigraphs nonsense */
		return("??/??/??");

	if((i % 1461) == 0) {
		/* last day of a leap year */
		y = 1973 + ((i/1461)<<2) - 1;
		m = 11;
		d = 31;
	} else {
		dd = i % 1461;
		y = 1973 + ((i/1461)<<2) + (dd - 1)/365;
		dd = ((dd - 1)%365) + 1;
		m = 12;
		if((y & 3) == 0) {
			/* leap year */
			while(dd <= M1[--m])
				;
			d = (unsigned char)(dd - M1[m]);
		} else {
			while(dd <= M0[--m])
				;
			d = (unsigned char)(dd - M0[m]);
		}
	}
	char2twobytes(d, &date[0]);
	char2twobytes(++m, &date[3]);
	char2twobytes((char)(y % 100), &date[6]);
	if(date[0] == ' ')
		date[0] = '0';
	if(date[3] == ' ')
		date[3] = '0';
	if(date[6] == ' ')
		date[6] = '0';
	return(date);
}

itoday()
{
	return(date2int(today(NULL)));
}

main()
{
	int foo = date2int("01/07/09");

	printf("%d\n", foo);

	puts(int2date(foo));
}
