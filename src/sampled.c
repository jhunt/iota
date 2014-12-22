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
#include "iota.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vigor.h>

#ifndef SAMPLED_CONFIG_FILE
#define SAMPLED_CONFIG_FILE "/etc/iota/sampled.conf"
#endif

#define DEFAULT_PIDFILE     "/var/run/sampled.pid"
#define DEFAULT_RUNAS_USER  "root"
#define DEFAULT_RUNAS_GROUP "root"

static struct {
	int         foreground;
	char       *runas_user;
	char       *runas_group;
	char       *pidfile;
	uint16_t    buffers;
	uint16_t    port;
	uint16_t    flush;
} OPTIONS = { 0 };

static void read_options(const char *file);

int main(int argc, char **argv)
{
	OPTIONS.foreground  = 0;
	OPTIONS.pidfile     = strdup(DEFAULT_PIDFILE);
	OPTIONS.runas_user  = strdup(DEFAULT_RUNAS_USER);
	OPTIONS.runas_group = strdup(DEFAULT_RUNAS_GROUP);
	read_options(SAMPLED_CONFIG_FILE);
	fprintf(stderr, "starting up\n"
	                "%u buffers flushed every %us\n"
	                "bind *:%u\n",
	                OPTIONS.buffers, OPTIONS.flush, OPTIONS.port);

	sample_set_t *SAMPLES = sample_set_new(OPTIONS.buffers);
	assert(SAMPLES);

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fd >= 0);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htons(INADDR_ANY);
	sa.sin_port = htons(OPTIONS.port);

	int rc = bind(fd, (struct sockaddr*)(&sa), sizeof(sa));
	assert(rc == 0);

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	struct timeval timeout;
	timeout.tv_sec = OPTIONS.flush;
	timeout.tv_usec = 0;

	if (!OPTIONS.foreground) {
		rc = daemonize(OPTIONS.pidfile, OPTIONS.runas_user, OPTIONS.runas_group);
		assert(rc == 0);
	}

	while ((rc = select(fd + 1, &fds, NULL, NULL, &timeout)) >= 0) {
		FD_SET(fd, &fds);

		if (rc == 0) {
			timeout.tv_sec = OPTIONS.flush;

			pid_t pid = fork();
			if (pid < 0) {
				perror("fork");
				continue;
			}
			if (pid == 0) {
				fprintf(stderr, "=====[ flushing data... ]=====\n");

				int pipefd[2];
				rc = pipe(pipefd);
				assert(rc == 0);

				pid = fork();
				if (pid < 0) {
					perror("fork");
					exit(1);
				}

				if (pid == 0) {
					close(pipefd[1]);
					dup2(pipefd[0], 0);
					execl("/bin/cat", "cat", NULL);
					exit(2);

				} else {
					close(pipefd[0]);
					dup2(pipefd[1], 1);

					char buf[1024];
					size_t i;
					for (i = 0; ; i++) {
						sample_t *s = sample_at(SAMPLES, i);
						if (!s) break;

						sample_to_string(s, buf, 1024);
						printf("%s\n", buf);
					}
				}
				exit(0);
			}

		} else {
			packet_t pkt;
			size_t nread = recv(fd, &pkt, sizeof(pkt), MSG_WAITALL);
			if (nread < 0) continue;
			if (!packet_is_valid(&pkt)) continue;

			errno = 0;
			long double v = packet_payload_ld(&pkt);
			if (errno) {
				fprintf(stderr, "BOGUS payload value '%s'\n", packet_payload(&pkt));
				continue;
			}

			sample_t *s = sample_find(SAMPLES, packet_metric(&pkt));
			if (!s) {
				fprintf(stderr, "Ran out of sample slots.  You should think about tuning.\n");
				continue;
			}

			sample_add(s, v);
			fprintf(stderr, "add %Lf to %s;\n"
							"          n/min/max = %lu/%Lf/%Lf\n"
							"    mean/var/stddev = %Lf/%Lf/%Lf\n",
							v, sample_name(s),
							sample_n(s), sample_min(s), sample_max(s),
							sample_mean(s), sample_variance(s), sample_stddev(s));
		}

		while (waitpid(-1, &rc, WNOHANG) > 0)
			;
	}

	return 0;
}

static void read_options(const char *file)
{
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
	OPTIONS.buffers = ul;

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
	OPTIONS.port = ul;

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
	OPTIONS.flush = ul;

	errno = errno_;
}
