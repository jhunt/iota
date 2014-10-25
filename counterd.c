/*
  Copyright 2014 James Hunt <james@jameshunt.us>

  This file is part of iota.

  iota is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  iota is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along
  with iota.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "core.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define COUNTERD_BUFFER 2048
#define COUNTERD_PORT 5015

int main(int argc, char **argv)
{
	counter_set_t *COUNTERS = counter_set_new(COUNTERD_BUFFER);
	assert(COUNTERS);

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fd >= 0);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htons(INADDR_ANY);
	sa.sin_port = htons(COUNTERD_PORT);

	int rc = bind(fd, (struct sockaddr*)(&sa), sizeof(sa));
	assert(rc == 0);

	packet_t pkt;
	size_t nread;
	while ((nread = recv(fd, &pkt, sizeof(pkt), MSG_WAITALL)) > 0) {
		if (!packet_is_valid(&pkt)) continue;

		errno = 0;
		uint8_t incr = packet_payload_u8(&pkt);
		if (errno) {
			fprintf(stderr, "BOGUS increment value '%s'\n", packet_payload(&pkt));
			continue;
		}

		counter_t *c = counter_find(COUNTERS, packet_metric(&pkt));
		if (!c) {
			fprintf(stderr, "Ran out of counter slots.  You should think about tuning.\n");
			continue;
		}

		counter_inc(c, incr);
		fprintf(stderr, "incr %s by %u to %lu\n", counter_name(c), incr, counter_value(c));
	}

	return 0;
}
