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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vigor.h>

#ifndef CONFIG_FILE
#define CONFIG_FILE "/etc/iota/counterd.conf"
#endif

typedef struct {
	uint16_t    buffers;
	uint16_t    port;
	uint16_t    flush;
} options_t;

static void read_options(options_t *opt, const char *file);

int main(int argc, char **argv)
{
	options_t opt;
	read_options(&opt, CONFIG_FILE);
	fprintf(stderr, "starting up\n"
	                "%u buffers flushed every %us\n"
	                "bind *:%u\n",
	                opt.buffers, opt.flush, opt.port);

	counter_set_t *COUNTERS = counter_set_new(opt.buffers);
	assert(COUNTERS);

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fd >= 0);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htons(INADDR_ANY);
	sa.sin_port = htons(opt.port);

	int rc = bind(fd, (struct sockaddr*)(&sa), sizeof(sa));
	assert(rc == 0);

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	struct timeval timeout;
	timeout.tv_sec = opt.flush;
	timeout.tv_usec = 0;

	while ((rc = select(fd + 1, &fds, NULL, NULL, &timeout)) >= 0) {
		FD_SET(fd, &fds);

		if (rc == 0) {
			timeout.tv_sec = opt.flush;

			pid_t pid = fork();
			if (pid < 0) {
				perror("fork");
				continue;
			}
			if (pid == 0) {
				fprintf(stderr, "=====[ flushing data... ]=====\n");

				char buf[1024];
				size_t i;
				for (i = 0; ; i++) {
					counter_t *c = counter_at(COUNTERS, i);
					if (!c) break;

					counter_to_string(c, buf, 1024);
					fprintf(stderr, "%s\n", buf);
				}
				exit(0);
			}

		} else {
			packet_t pkt;
			size_t nread = recv(fd, &pkt, sizeof(pkt), MSG_WAITALL);
			if (nread < 0) continue;
			if (!packet_is_valid(&pkt)) continue;

			int errno_ = errno; errno = 0;
			uint8_t incr = packet_payload_u8(&pkt);
			if (errno) {
				fprintf(stderr, "BOGUS increment value '%s'\n", packet_payload(&pkt));
				continue;
			}
			errno = errno_;

			counter_t *c = counter_find(COUNTERS, packet_metric(&pkt));
			if (!c) {
				fprintf(stderr, "Ran out of counter slots.  You should think about tuning.\n");
				continue;
			}

			counter_inc(c, incr);
			fprintf(stderr, "incr %s by %u to %lu\n", counter_name(c), incr, counter_value(c));
		}
	}

	return 0;
}

static void read_options(options_t *opt, const char *file)
{
	assert(opt);

	CONFIG(config);
	config_set(&config, "buffers", "2048");
	config_set(&config, "port",    "5015");
	config_set(&config, "flush",   "5");

	FILE *io = fopen(file, "r");
	if (!io) {
		perror(file);
		exit(1);
	}

	int rc = config_read(&config, io);
	assert(rc == 0);
	fclose(io);

	/* save errno */
	int errno_ = errno; errno = 0;
	errno = 0;

	char *val, *end;
	unsigned long ul;

	val = config_get(&config, "buffers");
	ul = strtoul(val, &end, 0);
	if (errno) {
		perror("buffers");
		exit(2);
	}
	if (ul > 65535) {
		fprintf(stderr, "buffers value %lu is invalid (must be less than 65536)\n", ul);
		exit(2);
	}
	opt->buffers = ul;

	val = config_get(&config, "port");
	ul = strtoul(val, &end, 0);
	if (errno) {
		perror("port");
		exit(2);
	}
	if (ul > 65535) {
		fprintf(stderr, "port value %lu is invalid (must be 0-65535)\n", ul);
		exit(2);
	}
	opt->port = ul;

	val = config_get(&config, "flush");
	ul = strtoul(val, &end, 0);
	if (errno) {
		perror("flush");
		exit(2);
	}
	if (ul > 86400) {
		fprintf(stderr, "flush value %lu is larger than one day...\n", ul);
		exit(2);
	}
	opt->flush = ul;

	errno = errno_;
}
