#include <stdio.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <string.h>
#include <arpa/inet.h>

int
main()
{
	u_char *p, *end;
	int len, i;
	const HEADER *hp;
	union {
		HEADER h;
		u_char u[PACKETSZ];
	} q;
	char buf[BUFSIZ];

	len = res_query("www.google.co.uk", C_IN, T_A, (u_char *)&q, sizeof(q));

	if(len < 0)
		return 1;

	hp = &(q.h);
	p = q.u + HFIXEDSZ;
	end = q.u + len;

	for(i = ntohs(hp->qdcount); i--; p += len + QFIXEDSZ)
		if((len = dn_skipname(p, end)) < 0)
			return 1;

	i = ntohs(hp->ancount);

        while((--i >= 0) && (p < end)) {
                u_short type;
                u_long ttl;
                const char *ip;
                struct in_addr addr;

                if((len = dn_expand(q.u, end, p, buf, sizeof(buf) - 1)) < 0)
                        return 1;
                p += len;
                GETSHORT(type, p);
                p += INT16SZ;
                GETLONG(ttl, p);        /* unused */
                GETSHORT(len, p);
                if(type != T_A) {
                        p += len;
                        continue;
                }
                memcpy(&addr, p, sizeof(struct in_addr));
                p += 4; /* Should check len == 4 */
                ip = inet_ntoa(addr);
                if(ip)
			puts(ip);
	}
	return 0;
}
