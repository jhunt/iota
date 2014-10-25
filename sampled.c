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

#define SAMPLED_BUFFER 2048
#define SAMPLED_PORT 5016

int main(int argc, char **argv)
{
	sample_set_t *SAMPLES = sample_set_new(SAMPLED_BUFFER);
	assert(SAMPLES);

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fd >= 0);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htons(INADDR_ANY);
	sa.sin_port = htons(SAMPLED_PORT);

	int rc = bind(fd, (struct sockaddr*)(&sa), sizeof(sa));
	assert(rc == 0);

	packet_t pkt;
	size_t nread;
	while ((nread = recv(fd, &pkt, sizeof(pkt), MSG_WAITALL)) > 0) {
		if (!packet_is_valid(&pkt)) continue;

		errno = 0;
		long double v = packet_payload_ld(&pkt);
		if (errno) {
			fprintf(stderr, "BOGUS payload value '%s'\n", packet_payload(&pkt));
			continue;
		}

		sample_t *c = sample_find(SAMPLES, packet_metric(&pkt));
		if (!c) {
			fprintf(stderr, "Ran out of sample slots.  You should think about tuning.\n");
			continue;
		}

		sample_add(c, v);
		fprintf(stderr, "add %Lf to %s;\n"
		                "      n/min/max/sum = %lu/%Lf/%Lf/%Lf\n"
		                "    mean/var/stddev = %Lf/%Lf/%Lf\n",
		                v, sample_name(c),
		                sample_n(c), sample_min(c), sample_max(c), sample_sum(c),
		                sample_mean(c), sample_variance(c), sample_stddev(c));
	}

	return 0;
}
