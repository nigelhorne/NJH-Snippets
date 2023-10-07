#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory.h>
#include <string.h>
#ifdef	M_I386
#include <prototypes.h>
#include <sys/tape.h>
#endif
#ifdef	sun
#include <sys/mtio.h>
#endif

union header {
	char buffer[1024];
	struct cpio {
		short	c_magic;
		short	c_dev;
		ushort	c_ino;
		ushort	c_mode;
		ushort	c_uid;
		ushort	c_gid;
		short	c_nlink;
		short	c_rdev;
		short	c_mtime[2];
		short	c_namesize;
		short	c_filesize[2];
		char	c_name[1];
	} cpio;
	struct {
		char	t_name[100],
			t_mode[8],
			t_uid[8],
			t_gid[8],
			t_size[12],
			t_mtime[12],
			t_chksum[8],
			t_linkflag,
			t_linkname[100],
			t_extno[4],
			t_extotal[4],
			t_efsize[12];
	} tar;
};

#define	CPIO	0
#define	ASCII_CPIO	1
#define	TAR	2

#ifdef	M_I386
int pascal	cmp(const char *filename, register FILE *ftape, off_t filelen, off_t tapelen);
int pascal	ignoreit(const char *filename, register FILE *ftape, off_t bytesleft);
int pascal	match(register const char *pattern, register const char *string);
#endif

/*#define	MAXFILES	100*/
#define	MAXFILES	0

#ifdef	M_I386
main(int argc, char **argv)
#else
main(argc, argv)
char **argv;
#endif
{
	register int fd, i, type, c, istape;
	register FILE *ftape;
	register char *ptr;
	int checksum;
	off_t offset, longfile;
	time_t longtime;
	union header header;
	struct stat statb;
#ifdef	M_I386
	struct tape_status tape;
#endif
#ifdef	sun
	struct mtget mtget;
#endif

#if	MAXFILES > 0
	fputs("Unregistered version - please register this copy\n", stderr);
	fputs("njh@smsltd.demon.co.uk\n", stderr);
#endif

	if(argc != 2) {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
		return(1);
	}
	fd = open(argv[1], O_RDONLY);
	if(fd < 0) {
		perror(argv[1]);
		return(2);
	}
#ifdef	M_I386
	if(ioctl(fd, MT_RESET, &tape) < 0)
		istape = 0;
	else {
		istape = 1;
		if(ioctl(fd, MT_STATUS, &tape) < 0) {
			perror(argv[1]);
			return(3);
		}
		if(!tape.bot)
			ioctl(fd, MT_REWIND, &tape);
		close(fd);
		fd = open(argv[1], O_RDONLY);
	}
#endif
#ifdef	sun
	if(ioctl(fd, MTREW, &mtget) < 0)
		istape = 0;
	else
		istape = 1;
#endif
	if(read(fd, (char *)&header, 1024) != 1024) {
		perror(argv[1]);
		return(4);
	}
	if(header.cpio.c_magic == 070707) {
		puts("cpio format not supported yet - use tar or ascii cpio");
		return(5);
		/*type = CPIO;*/
	} else if(memcmp(header.buffer, "070707", 6) == 0)
		type = ASCII_CPIO;
	else {
		sscanf(header.tar.t_chksum, "%o", &checksum);
		i = 0;
		for(ptr = header.tar.t_chksum; ptr < &header.tar.t_chksum[sizeof(header.tar.t_chksum)]; ptr++)
			*ptr = ' ';
		for(ptr = header.buffer; ptr < &header.buffer[512]; ptr++)
			i += *ptr;
		if(i == checksum)
			type = TAR;
		else {
			puts("unknown backup format");
			return(5);
		}
	}
	close(fd);
	if(istape) {
		fd = open(argv[1], O_RDONLY);
#ifdef	sun
		if(ioctl(fd, MTREW, &mtget) < 0) {
			perror(argv[1]);
			return(6);
		}
#endif
#ifdef	M_I386
		if(ioctl(fd, MT_RESET, &tape) < 0) {
			perror(argv[1]);
			return(6);
		}
		ioctl(fd, MT_REWIND, &tape);
#endif
		close(fd);
	}
	ftape = fopen(argv[1], "r");
	switch(type) {
		case ASCII_CPIO:
			ptr = header.cpio.c_name;
			while(fscanf(ftape, "%6o%6o%6o%6o%6o%6o%6o%6o%11lo%6o%11lo",
			    &header.cpio.c_magic, &header.cpio.c_dev,
			    &header.cpio.c_ino, &header.cpio.c_mode,
			    &header.cpio.c_uid, &header.cpio.c_gid,
			    &header.cpio.c_nlink, &header.cpio.c_rdev,
			    &longtime, &header.cpio.c_namesize,
			    &longfile) == 11) {
				if(header.cpio.c_magic != 070707) {
					puts("bad magic number");
					return(7);
				}
				if(fgets(ptr, header.cpio.c_namesize+1, ftape) == NULL) {
					perror(argv[1]);
					return(8);
				}
				if(strcmp(ptr, "TRAILER!!!") == 0)
					break;
				/*sprintf(filename, "%.*s\n", header.cpio.c_namesize + 1, header.cpio.c_name);*/
				/*if(stat(ptr, &statb) < 0) {*/
				if(lstat(ptr, &statb) < 0) {
					perror(ptr);
					if(ignoreit(ptr, ftape, longfile))
						continue;
					return(9);
				}
				printf("%s\r", ptr);
				fflush(stdout);
				if((mode_t)header.cpio.c_mode != statb.st_mode) {
					printf("%s: tape mode o%o file mode o%o\n", ptr, (int)header.cpio.c_mode, statb.st_mode);
					if(ignoreit(ptr, ftape, longfile))
						continue;
					return(10);
				}
				if((uid_t)header.cpio.c_uid != statb.st_uid)
					printf("%s: tape owner %d file owner %d (warning)\n", ptr, header.cpio.c_uid, statb.st_uid);
				if((gid_t)header.cpio.c_gid != statb.st_gid)
					printf("%s: tape group %d file group %d (warning)\n", ptr, header.cpio.c_gid, statb.st_gid);
				if(header.cpio.c_nlink != statb.st_nlink)
					printf("%s: tape link count %d file link count %lu (warning)\n", ptr, header.cpio.c_nlink, statb.st_nlink);
				if((statb.st_mode&S_IFMT) == S_IFREG)
					if(!cmp(ptr, ftape, statb.st_size, longfile)) {
						fprintf(stderr, "tapechk failed on %s\n", ptr);
						return(13);
					}
				printf("%-*c\r", header.cpio.c_namesize + 2, ' ');
			}
			break;
		case TAR:
			ptr = "";
			for(;;) {
				printf("%-*c\r", (int)strlen(ptr) + 2, ' ');
				offset = ftell(ftape);
				if(offset & 511L) {
					if(istape) {
						offset = 512L - (offset & 511L);
						while(--offset >= 0)
							c = getc(ftape);
					} else
						fseek(ftape, (offset & ~511L) + 512L, 0);
				}
				if(fread(header.buffer, 512, 1, ftape) != 1)
					break;
				if(header.tar.t_name[0] == '\0')
					break;
				sscanf(header.tar.t_chksum, "%o", &checksum);
				i = 0;
				for(ptr = header.tar.t_chksum; ptr < &header.tar.t_chksum[sizeof(header.tar.t_chksum)]; ptr++)
					*ptr = ' ';
				for(ptr = header.buffer; ptr < &header.buffer[512]; ptr++)
					i += *ptr;
				if(i != checksum) {
					puts("Directory checksum error");
					return(9);
				}
				ptr = header.tar.t_name;
				sscanf(header.tar.t_size, "%lo", &offset);
				/*if(stat(ptr, &statb) < 0) {*/
				if(lstat(ptr, &statb) < 0) {
					perror(ptr);
					if(ignoreit(ptr, ftape, offset))
						continue;
					return(10);
				}
				printf("%s\r", ptr);
				fflush(stdout);
				sscanf(header.tar.t_mode, "%o", &checksum);
				if((checksum&~S_IFMT) != (statb.st_mode&~S_IFMT)) {
					printf("%s: tape mode o%o file mode o%o\n", ptr, (checksum&~S_IFMT), (statb.st_mode&~S_IFMT));
					return(11);
				}
				sscanf(header.tar.t_uid, "%o", &checksum);
				if((uid_t)checksum != statb.st_uid)
					printf("%s: tape owner %d file owner %d (warning)\n", ptr, checksum, statb.st_uid);
				sscanf(header.tar.t_gid, "%o", &checksum);
				if((gid_t)checksum != statb.st_gid)
					printf("%s: tape group %d file group %d (warning)\n", ptr, checksum, statb.st_gid);
				checksum = 0;
				sscanf(header.tar.t_extno, "%o", &checksum);
				if(checksum) {
					fprintf(stderr, "%s: tar extents (%d) not supported\n", ptr, checksum);
					return(12);
				}
				/* TODO: sort this out */
#if	0
				if(header.tar.t_linkflag) {
					header.tar.t_linkflag -= '0';
					if(header.tar.t_linkflag) {
						if(header.tar.t_linkflag != (char)statb.st_nlink)
							printf("%s: tape link count %d file link count %d (warning)\n", ptr, (int)header.tar.t_linkflag, statb.st_nlink);
						continue;
					}
				}
#endif
				if((statb.st_mode&S_IFMT) == S_IFREG)
					if(!cmp(ptr, ftape, statb.st_size, offset)) {
						fprintf(stderr, "tapechk failed on %s\n", ptr);
						return(15);
					}
			}
	}
	fclose(ftape);
	return(0);
}

#ifdef	M_I386
int pascal
cmp(const char *filename, register FILE *ftape, off_t filelen, off_t tapelen)
#else
cmp(filename, ftape, filelen, tapelen)
char *filename;
FILE *ftape;
off_t filelen, tapelen;
#endif
{
	register FILE *fin;
	register off_t offset = (off_t)0;
	register int c, ret;
#if	MAXFILES > 0
	static int maxfiles;

	if(++maxfiles == MAXFILES) {
		fprintf(stderr, "Unregistered version: you have reached the limit of %d files\n", MAXFILES);
		exit(1);
	}
#endif

	if(filelen != tapelen) {
		printf("%s: tape %ld bytes file %ld bytes", filename, tapelen, filelen);
		return(ignoreit(filename, ftape, tapelen));
	}
	fin = fopen(filename, "r");
	if(fin == NULL) {
		perror(filename);
		return(ignoreit(filename, ftape, tapelen));
	}
	ret = 1;
	while((c = getc(fin)) != EOF) {
		offset++;
		if(c != getc(ftape)) {
			printf("%s: differs at byte %ld", filename, offset);
			ret = ignoreit(filename, ftape, tapelen - offset);
			break;
		}
	}
	fclose(fin);
	return(ret);
}

#ifdef	M_I386
int pascal
ignoreit(const char *filename, register FILE *ftape, off_t bytesleft)
#else
ignoreit(filename, ftape, bytesleft)
char *filename;
FILE *ftape;
off_t bytesleft;
#endif
{
	static FILE *ignore;
	char line[81];

	if(ignore == (FILE *)NULL) {
		ignore = fopen("/usr/local/lib/tapechk", "r");
		if(ignore == (FILE *)NULL) {
			ignore = fopen("/usr/lib/tapechk", "r");
			if(ignore == (FILE *)NULL) {
				ignore = fopen("/lib/tapechk", "r");
				if(ignore == (FILE *)NULL)
					ignore = fopen("/etc/tapechk", "r");
			}
		}
	}
	if(ignore) {
		rewind(ignore);
		while(fgets(line, sizeof(line) - 1, ignore)) {
			line[strlen(line) - 1] = '\0';	/* remove \\n */
			if(match(line, filename))
				break;
		}
		if(feof(ignore)) {
			putchar('\n');
			return(0);
		}
		puts(" (ignored)");
		while((--bytesleft >= 0) && (getc(ftape) != EOF))
			;
		return(1);
	} else {
		putchar('\n');
		return(0);
	}
}

/*
 * match -
 *	Tarted up by NJH.
 *
 *	Author:		Gary S. Moss
 *	U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground
 *	Maryland 21005-5066
 *	(301)278-6647 or AV-283-6647
 *
 *	if string matches pattern, return 1, else return 0
 *	special characters:
 *		*	Matches any string including the null string.
 *		?	Matches any single character.
 *		[...]	Matches any one of the characters enclosed.
 *		[!..]	Matches an character NOT enclosed.
 *		-	May be used inside brackets to specify range
 *			(i.e. str[1-58] matches str1, str2, ... str5, str8)
 *		\	Escapes special characters.
 */
#ifdef	M_I386
int pascal
match(register const char *pattern, register const char *string)
#else
match(pattern, string)
register char *pattern, *string;
#endif
{
	do {
		switch(pattern[0]) {
		case '*': /* Match any string including null string. */
			if(pattern[1] == '\0' || string[0] == '\0')
				return(1);
			while(string[0] != '\0') {
				if(match(&pattern[1], string))
					return(1);
				++string;
			}
			return(0);
		case '?': /* Match any character. */
			break;
		case '[': /* Match one of the characters in brackets
				unless first is a '!', then match
				any character not inside brackets. */
			{
				register char *rgtBracket;
				register int negation;

				++pattern; /* Skip over left bracket. */
				/* Find matching right bracket. */
				if((rgtBracket = strchr(pattern, ']')) == NULL) {
					puts(" unmatched '['");
					return(0);
				}
				/* Check for negation operator. */
				if(pattern[0] == '!') {
					++pattern;
					negation = 1;
				} else
					negation = 0;
				/* Traverse pattern inside brackets. */
				for(; pattern < rgtBracket && pattern[0] != string[0]; ++pattern) {
					if(pattern[0] == '-' && pattern[-1] != '\\') {
						if(pattern[-1] <= string[0] && pattern[-1] != '[' && pattern[1] >= string[0] && pattern[1] != ']')
							break;
					}
				}
				if(pattern == rgtBracket) {
					if(!negation)
						return(0);
				} else {
					if(negation)
						return(0);
				}
				pattern = rgtBracket; /* Skip to right bracket. */
				break;
			}
		case '\\': /* Escape special character.	*/
			++pattern;
			/* WARNING: falls through to default case. */
		default: /* Compare characters. */
			if(pattern[0] != string[0])
				return(0);
		}
		++pattern;
		++string;
	} while(pattern[0] != '\0' && string[0] != '\0');
	return((pattern[0] == '\0' || pattern[0] == '*') && string[0] == '\0');
}
