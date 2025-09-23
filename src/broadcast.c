#include <netinet/in.h>
#include <net/if.h>

static void
broadcast(const char *mess)
{
	int on, sock;
	struct sockaddr_in s;
	struct ifreq ifr = { 0 };

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(sock < 0) {
		perror("socket");
		return;
	}

	on = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (int *)&on, sizeof(on)) < 0) {
		perror("setsockopt");
		close(sock);
		return;
	}
	strcpy(ifr.ifr_name, "eth1");
	if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
		perror("setsockopt");
		close(sock);
		return;
	}

	memset(&s, '\0', sizeof(struct sockaddr_in));
	s.sin_family = AF_INET;
	s.sin_port = (in_port_t)htons(3310);
	s.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	if(sendto(sock, mess, strlen(mess), 0, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) < 0)
		perror("sendto");

	close(sock);
}

main()
{
	broadcast("foo");
}
