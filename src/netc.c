#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define	PORT	514	/* TODO: /etc/services lookup for shell */

static	void	*check_host(void *h);

static const char *hosts[] = {
	"localhost",
	"gateway",
	NULL,
};

#define	NHOSTS	((sizeof(hosts) / sizeof(hosts[0])) - 1)

int
main(int argc, const char **argv)
{
#if	0
	switch(fork()) {
		case -1:
			perror("fork");
			return errno;
		case 0:
			break;
		default:
			return 0;
	}
#endif

	for(;;) {
		const char **host;
		pthread_t tids[NHOSTS];
		int i = 0;

		for(host = hosts; *host; host++) {
#if	0
			int sock;
			in_addr_t ip = inet_addr(*host);
			struct sockaddr_in server;

			printf("Trying %s\n", *host);
			if(ip == INADDR_NONE) {
				const struct hostent *h = gethostbyname(*host);

				if(h == NULL) {
					fprintf(stderr, "%s: Unknown host %s\n",
						argv[0], *host);
					return 1;
				}

				memcpy((char *)&ip, h->h_addr, sizeof(ip));
			}
			memset((char *)&server, 0, sizeof(struct sockaddr_in));
			server.sin_family = AF_INET;
			server.sin_port = (in_port_t)htons(PORT);

			server.sin_addr.s_addr = ip;

			if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				perror(*host);
				return errno;
			}
			if(connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
				/*perror(*host);
				close(sock);
				return errno;*/
			}
			close(sock);
#endif
			pthread_create(&tids[i++], NULL, check_host, (void *)*host);
		}
		
		sleep(1);
	}
	return 0;
}

static void *
check_host(void *h)
{
	const char *host = (const char *)h;

	puts(host);

	return h;
}
