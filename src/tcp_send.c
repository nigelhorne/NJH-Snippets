#include <stdio.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int
main()
{
	int s, c, val;
	struct sockaddr_in sin;
	struct hostent *h;
	struct timeval timeout;
	char buf[80];

	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0) {
		perror("socket");
		return errno;
	}

	h = gethostbyname("localhost");
	if(h == NULL) {
		puts("localhost?!?");
		return 1;
	}

	memset(&sin, '\0', sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(25);
	memcpy((char *)&sin.sin_addr.s_addr, (char *)h->h_addr, h->h_length);

	if(connect(s, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0) {
		perror("connect");
		close(s);
		return errno;
	}

	while((c = getchar()) != EOF) {
		send(s, &c, 1, 0);
		putchar(c);

		if(c == '\n')
			for(;;) {
				fd_set rfds;

				FD_ZERO(&rfds);
				FD_SET(s, &rfds);
				timeout.tv_sec = 0;
				timeout.tv_usec = 1;
				val = select(s + 1, &rfds, NULL, NULL, &timeout);
				if(val > 0)
					write(1, buf, recv(s, buf, 80, 0));
				else
					break;
			}
	}

	shutdown(s, 1);

	while((val = recv(s, buf, 80, 0)) > 0)
		write(1, buf, val);

	return (close(s) < 0);
}
