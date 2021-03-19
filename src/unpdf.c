/*
 * PDFextractor in C
 * Copyright (C) 2005 Nigel Horne
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <zlib.h>

static	void	unpdf(const char *filename);
static	const	char	*cli_pmemstr(const char *haystack, size_t hs, const char *needle, size_t ns);

#ifndef	MIN
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

int
main(int argc, char **argv)
{
	if(argc < 2) {
		puts("Arg count");
		return 1;
	}
	while(*++argv)
		unpdf(*argv);

	return 0;
}

static void
unpdf(const char *filename)
{
	struct stat statb;
	off_t size;
	int fd;
	char *buf;
	const char *p, *q;

	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		perror(filename);
		return;
	}
	if(fstat(fd, &statb) < 0) {
		perror(filename);
		close(fd);
		return;
	}
	size = (size_t)statb.st_size;

	if(size == 0) {
		close(fd);
		return;
	}
	p = buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if(buf == MAP_FAILED) {
		perror(filename);
		close(fd);
		return;
	}
	while((q = cli_pmemstr(p, size, "obj", 3)) != NULL) {
		int length, flatedecode;
		const char *s, *t;
		const char *u, *obj;
		size_t objlen;
		static int attachment = 0;
		char fname[3];
		int fout;

		size -= (q - p) + 3;
		obj = p = &q[3];
		q = cli_pmemstr(p, size, "endobj", 6);
		if(q == NULL) {
			puts("No matching endobj");
			break;
		}
		size -= (q - p) + 6;
		p = &q[6];
		objlen = (size_t)(q - obj);

		t = cli_pmemstr(obj, objlen, "stream\n", 7);
		if(t == NULL)
			continue;

		length = flatedecode = 0;
		for(s = obj; s < t; s++) {
			if(*s == '/') {
				if(strncmp(++s, "Length ", 7) == 0) {
					s += 7;
					length = atoi(s);
					while(isdigit(*s))
						s++;
					printf("length %d\n", length);
				} else if((strncmp(s, "FlateDecode ", 12) == 0) ||
					  (strncmp(s, "FlateDecode\n", 12) == 0)) {
					flatedecode = 1;
					s += 12;
				}
			}
		}
		t += 7;
		u = cli_pmemstr(t, objlen - 7, "endstream\n", 10);
		if(u == NULL) {
			puts("No endstream");
			break;
		}
		if(++attachment == 100) {
			puts("fname overflow");
			break;
		}
		sprintf(fname, "%d", attachment);
		fout = open(fname, O_CREAT|O_TRUNC|O_WRONLY, 0600);
		if(fout < 0) {
			perror(fname);
			break;
		}
		if(flatedecode) {
			z_stream stream;
			size_t len = (size_t)(u - t);
			unsigned char output[BUFSIZ];
			size_t buflen;

			stream.zalloc = (alloc_func)Z_NULL;
			stream.zfree = (free_func)Z_NULL;
			stream.opaque = (void *)NULL;
			stream.next_in = (unsigned char *)t;
			buflen = stream.avail_in = len;

			if(inflateInit(&stream) != Z_OK) {
				puts("inflateInit failed");
				break;
			}
			stream.next_out = output;
			stream.avail_out = sizeof(output);
			do
				if(stream.avail_out == 0) {
					write(fout, output, BUFSIZ);
					stream.next_out = output;
					stream.avail_out = BUFSIZ;
				}
			while(inflate(&stream, Z_NO_FLUSH) == Z_OK);

			write(fout, output, sizeof(output) - stream.avail_out);
			inflateEnd(&stream);
		} else
			write(fout, t, (size_t)(u - t));

		close(fout);
	}
	munmap(buf, size);
	close(fd);
}

static const char *
cli_pmemstr(const char *haystack, size_t hs, const char *needle, size_t ns)
{
	const char *pt, *hay;
	size_t n;

	if(haystack == needle)
		return haystack;

	if(hs < ns)
		return NULL;

	if(memcmp(haystack, needle, ns) == 0)
		return haystack;

	pt = hay = haystack;
	n = hs;

	while((pt = memchr(hay, needle[0], n)) != NULL) {
		n -= (int) pt - (int) hay;
		if(n < ns)
			break;

		if(memcmp(pt, needle, ns) == 0)
			return pt;

		if(hay == pt) {
			n--;
			hay++;
		} else
			hay = pt;
	}

	return NULL;
}
