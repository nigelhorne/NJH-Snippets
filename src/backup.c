#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <unistd.h>

#define	SEVENDAYS	((time_t)(7 * 24 * 3600))

static	void	copyfile(const char *path, const char *targetdir);

int
main(int argc, char **argv)
{
	size_t n;
	time_t now;
	char *line;

	if(argc != 2) {
		fputs("Arg count\n", stderr);
		return 1;
	}

	line = NULL;
	n = 0;

	time(&now);

	while(getline(&line, &n, stdin) >= 0) {
		char *ptr;
		struct stat in, out;
		char buf[NAME_MAX + 1];

		if((ptr = strchr(line, '\n')) != NULL)
			*ptr = '\0';

		if(stat(line, &in) < 0) {
			perror(line);
			continue;
		}

		if((now - in.st_mtime) > SEVENDAYS)
			continue;

		snprintf(buf, sizeof(buf) - 1, "%s/%s", argv[1], line);

		if(stat(buf, &out) >= 0) {
			if(S_ISDIR(in.st_mode))
				continue;
			if(out.st_mtime >= in.st_mtime)
				continue;
		} else if(S_ISDIR(in.st_mode)) {
			/*printf("making directory %s\n", buf);*/
			if((mkdir(buf, in.st_mode) < 0) && (errno != EEXIST)) {
				perror(buf);
				continue;
			}
			if(chown(buf, in.st_uid, in.st_gid) < 0)
				perror(buf);
			continue;
		}
		if(S_ISREG(in.st_mode))
			copyfile(line, buf);
	}

	if(line)
		free(line);

	return 0;
}

static void
copyfile(const char *in, const char *out)
{
	FILE *fin, *fout;
	int c;

	if((fin = fopen(in, "r")) == NULL) {
		perror(in);
		return;
	}

	if((fout = fopen(out, "w")) == NULL) {
		perror(out);
		fclose(fin);
		return;
	}

	/*printf("copying %s to %s\n", in, out);*/

	while((c = getc(fin)) != EOF)
		putc(c, fout);

	fclose(fin);
	fclose(fout);
}
