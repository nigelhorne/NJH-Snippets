/*
 * cc $CFLAGS `curl-config --cflags --libs` curlsample.c
 */
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static	void	getURL(const char *url, const char *dir, const char *filename);

int
main()
{
	getURL("http://192.168.1.1", "/tmp", "out");
	getURL("http://192.168.1.1/about.htm", "/tmp", "about");

	return 0;
}

static void
getURL(const char *url, const char *dir, const char *filename)
{
	char *fout;
	static int initialised = 0;
	static CURL *curl;
	FILE *fp;

	if(!initialised) {
		if(curl_global_init(CURL_GLOBAL_NOTHING) != 0) {
			puts("curl_global_init");
			return;
		}
		/* easy isn't the word I'd use... */
		curl = curl_easy_init();
		if(curl == NULL) {
			puts("curl_easy_init");
			curl_global_cleanup();
			return;
		}
		(void)curl_easy_setopt(curl, CURLOPT_USERAGENT, "www.clamav.net");

		initialised = 1;
	}
	if(curl_easy_setopt(curl, CURLOPT_URL, url) != 0) {
		puts(url);
		return;
	}
	fout = (char *)malloc(strlen(dir) + strlen(filename) + 2);

	if(fout == NULL)
		return;

	sprintf(fout, "%s/%s", dir, filename);

	fp = fopen(fout, "w");
	if(fp == NULL) {
		perror(fout);
		free(fout);
		return;
	}
	free(fout);

	if(curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp) != 0) {
		puts("curl_easy_setopt");
		return;
	}

	if(curl_easy_perform(curl)) {
		puts(url);
		return;
	}
}
