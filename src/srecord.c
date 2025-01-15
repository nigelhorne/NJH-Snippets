/*
 * Read input file(s), convert them to S-records and use either TCP or UDP to
 * send to a specified machine and port; e.g.
 *	./a.out tcp localhost 3000 /etc/passwd
 * or, send them directly over a serial port (38400 8N1); e.g.
 *	./a.out serial /dev/ttyS0 /etc/passwd
 *
 * Author Nigel Horne: njh@bandsman.co.uk
 *
 * Protocol can be tcp or udp
 * Hosts can be IPv4 addresses or machine names
 * Ports can be service names or port numbers
 * If no files are given, or if the filename is '-', use standard input
 * More than one input file can be specified
 *
 * As a quick confidence test, on machineA run:
 *	nc -l -p 9000 | srec_cat -Output -Binary
 * on machineB run:
 *	./a.out tcp machineB 9000 /etc/passwd
 * You should then see machineA's password file appear on machineB
 *
 * The EDW100 device must be set to 38400 8N1
 *
 * This program could be changed to listen for data on the connection as
 *	well, using select
 *
 * As a confidence test that the EDW100 is working, run
 *	telnet 169.254.100.100 9000
 * and, while the telnet is still running, do 
 *	 ./a.out serial /dev/ttyS0 /etc/passwd
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>

#define	IN_REC_LENGTH	32	/* 32 limits the output to 72 characters per line */
#define	WAIT_TIME_MS	200	/*
				 * ensure that the EDW100's buffer doesn't fill
				 * this is the number of mS to stop between
				 * sending Srecords via Ethernet to be sent
				 * out on the serial port
				 *
				 * 10mS is a reasonable value for 38.4. Perhaps
				 *	it can be smaller with faster rates
				 */


static	int	serial(int argc, const char **argv);
static	int	out_stdout(int argc, const char **argv);
static	int	ip(int argc, const char **argv);
static	int	encodeFile(const char *fileName, int sockout, const struct sockaddr_in *sin);
static	void	encodeDescriptor(FILE *fin, int sockout, const struct sockaddr_in *sin);
static	void	toSrec(const unsigned char *in, uint16_t address, char *out);
static	char	encodeNibble(uint8_t n);

int
main(int argc, const char **argv)
{
	if(argc < 2) {
		fprintf(stderr, "Usage: %s TCP|UDP [hostname] port [filenames...]\n", argv[0]);
		fprintf(stderr, "Usage: %s serial device [filenames...]\n", argv[0]);
		fprintf(stderr, "Usage: %s stdout [filenames...]\n", argv[0]);
		return 1;
	}

	/* Select operation mode based on the first argument */
	if(strcasecmp(argv[1], "serial") == 0)
		return serial(argc, argv);
	if(strcasecmp(argv[1], "stdout") == 0)
		return out_stdout(argc, argv);

	return ip(argc, argv);
}

/*
 * Handles serial communication: Configures the serial port and sends S-records.
 */
static int
serial(int argc, const char **argv)
{
	int fd;
	struct termios termios;

	if(argc < 3) {
		fprintf(stderr, "Usage: %s serial port [filenames...]\n", argv[0]);
		return 1;
	}

	/* Open the serial port for writing */
	fd = open(argv[2], O_WRONLY);
	if(fd < 0) {
		perror(argv[2]);
		return errno;
	}

	/* Ensure the file descriptor refers to a terminal device */
	if(!isatty(fd)) {
		close(fd);
		return EINVAL;
	}

	/* Configure the serial port settings */
	if((tcgetattr(fd, &termios) < 0) ||
	   (cfsetispeed(&termios, B38400) < 0) ||
	   (cfsetospeed(&termios, B38400) < 0)) {
		perror(argv[2]);
		close(fd);
		return errno;
	}

	cfmakeraw(&termios);	/* Set raw mode */
	termios.c_cflag &= ~(CSTOPB|PARENB);	/* 1 stop bit, no parity */
	termios.c_cflag = (termios.c_cflag & ~CSIZE) | CS8;	/* 8 bit */
	termios.c_cflag |= CREAD;	/* Enable receiver */
	termios.c_iflag = IGNBRK;	/* Ignore break condition */
	termios.c_lflag = termios.c_oflag = 0;	/* Disable input/output processing */

	if(tcsetattr(fd, TCSANOW, &termios) < 0) {
		perror(argv[2]);
		close(fd);
		return errno;
	}

	argv++;
	argv++;
	argc--;

	/* No S0 (header) record is sent */

	if(--argc == 1) {
		encodeDescriptor(stdin, fileno(stdout), NULL);
		encodeDescriptor(stdin, fd, NULL);
	} else
		/* Encode data from each specified file */
		while(--argc > 0) {
			int rc = encodeFile(*++argv, fd, NULL);

			if(rc != 0) {
				close(fd);
				return rc;
			}
		}

	/* Send terminating record to signal the end of transmission */
	if(write(fd, "S9030000FC\n", 11) < 0) {
		perror("write");
		close(fd);
		return errno;
	}

	return close(fd);
}

static int
out_stdout(int argc, const char **argv)
{
	argv++;

	/* No S0 (header) record is sent */

	if(--argc == 1)
		/* Encode data from standard input if no files are provided */
		encodeDescriptor(stdin, fileno(stdout), NULL);
	else
		/* Encode data from each specified file */
		while(--argc > 0) {
			int rc = encodeFile(*++argv, fileno(stdout), NULL);

			if(rc != 0)
				return rc;
		}

	/* Send terminating record to signal the end of transmission */
	if(write(fileno(stdout), "S9030000FC\n", 11) < 0) {
		perror("write");
		return errno;
	}

	return 0;
}

/*
 * Handles TCP or UDP communication: Resolves the host and port, establishes 
 * a connection, and sends S-records.
 */
static int
ip(int argc, const char **argv)
{
	int sockout, isTCP;
#ifdef	SO_REUSEADDR
	const int on = 1;
#endif
	struct sockaddr_in sin;
	const struct protoent *protoent;
	const char *protocol;
	in_port_t port;
	in_addr_t addr;

	if(argc < 4) {
		fprintf(stderr, "Usage: %s protocol host port [filenames...]\n", argv[0]);
		return 1;
	}

	protocol = argv[1];
	protoent = getprotobyname(protocol);

	if(protoent == NULL) {
		fprintf(stderr, "%s: Unknown protocol %s\n", argv[0], protocol);
		return 1;
	}

	/* Resolve port from service name or number */
	if(!isdigit(argv[3][0])) {
		const struct servent *servent = getservbyname(argv[3], protocol);

		if(servent == (const struct servent *)NULL) {
			fprintf(stderr, "%s: Unknown %s service '%s'\n",
				argv[0], protocol, argv[3]);
			return 1;
		}

		port = (in_port_t)servent->s_port;
	} else
		port = (in_port_t)htons((uint16_t)atoi(argv[3]));

	/* Resolve host address */
	addr = inet_addr(argv[2]);
	if(addr == (in_addr_t)-1) {
		const struct hostent *hostent = gethostbyname(argv[2]);

		if(hostent == NULL) {
			fprintf(stderr, "%s: IP address/hostname '%s' is invalid\n",
				argv[0], argv[2]);
			return 1;
		}
		/*addr = inet_addr(hostent->h_addr);

		if(addr == (in_addr_t)-1) {
			fprintf(stderr, "IP address '%s' for hostname '%s' is invalid\n", hostent->h_addr, argv[2]);
			return 1;
		}*/
		memcpy(&addr, hostent->h_addr, sizeof(in_addr_t));
	}

	/* Initialize socket address structure */
	memset(&sin, '\0', sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr.s_addr = addr;

	/* Create and configure the socket based on the protocol */
	if(strcasecmp(protocol, "tcp") == 0) {
		sockout = socket(AF_INET, SOCK_STREAM, protoent->p_proto);

		if(sockout < 0) {
			perror("socket");
			return errno;
		}

#ifdef	SO_REUSEADDR
		if(setsockopt(sockout, SOL_SOCKET, SO_REUSEADDR, &on, (socklen_t)sizeof(on)) < 0)
			perror("setsockopt");	/* warning */
#endif

		/* Connect to outgoing */
		if(connect(sockout, (struct sockaddr *)&sin, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
			perror("connect");
			close(sockout);
			return errno;
		}
		isTCP = 1;
	} else if(strcasecmp(protocol, "udp") == 0) {
		sockout = socket(AF_INET, SOCK_DGRAM, protoent->p_proto);

		if(sockout < 0) {
			perror("socket");
			return errno;
		}
		isTCP = 0;
	} else {
		fprintf(stderr, "%s: Only TCP and UDP are currently supported\n",
			argv[0]);
		return 1;
	}
	argv++;
	argv++;
	argv++;
	argc--;
	argc--;

	/* No S0 (header) record is sent */

	if(--argc == 1)
		/* Encode data from standard input if no files are provided */
		encodeDescriptor(stdin, sockout, (isTCP) ? NULL : &sin);
	else
		/* Encode data from each specified file */
		while(--argc > 0) {
			int rc = encodeFile(*++argv, sockout, (isTCP) ? NULL : &sin);

			if(rc != 0) {
				close(sockout);
				return rc;
			}
		}

	/* Send terminating record to signal the end of transmission */
#ifdef	WAIT_TIME_MS
	usleep(WAIT_TIME_MS * 1000);
#endif
	if(isTCP) {
		if(send(sockout, "S9030000FC\n", 11, 0) < 0) {
			perror("send");
			close(sockout);
			return errno;
		}
	} else if(sendto(sockout, "S9030000FC\n", 11, 0, (const struct sockaddr *)&sin, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
		perror("sendto");
		close(sockout);
		return errno;
	}

	return close(sockout);
}

/*
 * Encodes a file into S-records and sends it through the specified output
 */
static int
encodeFile(const char *fileName, int sockout, const struct sockaddr_in *sin)
{
	FILE *fin;

	if(strcmp(fileName, "-") == 0) {
		encodeDescriptor(stdin, sockout, sin);
		return 0;
	}

	fin = fopen(fileName, "r");

	if(fin == NULL) {
		perror(fileName);
		return errno;
	}
	encodeDescriptor(fin, sockout, sin);
	return fclose(fin);
}

static void
encodeDescriptor(FILE *fin, int sockout, const struct sockaddr_in *sin)
{
	uint16_t address = 0;

	for(;;) {
		int c;
		int length = 0;
		char in[IN_REC_LENGTH + 1];
		char out[(IN_REC_LENGTH * 2) + 1];
		char *ptr = in;

		while((c = getc(fin)) != EOF) {
			*ptr++ = (char)c;
			if(++length == (int)sizeof(in) - 1)
				break;
		}
		*ptr = '\0';

		toSrec((const unsigned char *)in, address, out);

#ifdef	WAIT_TIME_MS
		usleep(WAIT_TIME_MS * 1000);
#endif

		if(sin)	{
			/* UDP */
			if(sendto(sockout, out, strlen(out), 0, (const struct sockaddr *)sin, (socklen_t)sizeof(struct sockaddr_in)) < 0) {
				perror("sendto");
				break;
			}
		} else {
			/* TCP */
			if(write(sockout, out, strlen(out)) < 0) {
				perror("write");
				break;
			}
		}

		if(c == EOF)
			break;

		address += (uint16_t)length;
	}
}

static void
toSrec(const unsigned char *in, uint16_t address, char *out)
{
	char *len;
	uint16_t length = 0;
	unsigned int checksum = 0;

	/* Type 1 = address length is 2 bytes */
	*out++ = 'S';
	*out++ = '1';

	len = out;
	out += 2;

	/* Address location is 0 */
	*out++ = encodeNibble((address >> 12) & 0xF);
	*out++ = encodeNibble((address >> 8) & 0xF);
	*out++ = encodeNibble((address >> 4) & 0xF);
	*out++ = encodeNibble(address & 0xF);

	checksum += (address >> 8) & 0xFF;
	checksum += address & 0xFF;

	length += 2;	/* address */

	while(*in) {
		*out++ = encodeNibble((uint8_t)((*in >> 4) & 0xF));
		*out++ = encodeNibble((uint8_t)(*in & 0xF));
		checksum += (int)*in++;
		length++;
	}

	length++;	/* checksum */

	*len++ = encodeNibble((length >> 4) & 0xF);
	*len = encodeNibble(length & 0xF);
	checksum += length & 0xFF;

	checksum = ~checksum;

	*out++ = encodeNibble((uint8_t)((checksum >> 4) & 0xF));
	*out++ = encodeNibble((uint8_t)(checksum & 0xF));

	*out++ = '\n';
	*out = '\0';
}

static char
encodeNibble(uint8_t n)
{
	return "0123456789ABCDEF"[n & 0xF];
}
