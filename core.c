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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void* xmalloc(size_t n)
{
	void *x = calloc(1, n);
	if (x) return x;
	perror("malloc");
	abort();
}

#define E(s,n) ((s)->set+(n))

counter_set_t* counter_set_new(size_t len)
{
	counter_set_t *s = xmalloc(sizeof(counter_set_t));
	s->len = len;
	s->set = xmalloc(len * sizeof(counter_t));
	return s;
}

void counter_set_free(counter_set_t *s)
{
	if (!s) return;
	free(s->set);
	free(s);
}

counter_t* counter_find(counter_set_t *s, const char *key)
{
	assert(s);
	assert(key);

	size_t i;
	for (i = 0; i < s->next; i++)
		if (strcmp(E(s,i)->name, key) == 0)
			return E(s,i);
	return counter_next(s, key);
}

counter_t* counter_next(counter_set_t *s, const char *key)
{
	assert(s);
	assert(key);

	errno = ENOBUFS;
	if (s->next >= s->len)
		return NULL;

	memset(E(s,s->next), 0, sizeof(counter_t));
	strncpy(E(s,s->next)->name, key, METRIC_NAME_MAX);
	return E(s, s->next++);
}

int counter_to_string(counter_t *c, char *buf, size_t max)
{
	assert(c);
	assert(buf);
	assert(max > 0);

	size_t n = snprintf(buf, max, "%s=%lu %u",
			counter_name(c), counter_value(c), counter_rollover(c));

	if (n < max)
		return 0;

	errno = ENOBUFS;
	return -1;
}

int counter_inc(counter_t *c, uint8_t i)
{
	assert(c);
	assert(i > 0);

	if (COUNTER_MAX - i < c->value)
		c->rollover++;
	c->value += i;
	return 0;
}

int counter_reset(counter_t *c)
{
	assert(c);
	c->rollover = c->value = 0;
	return 0;
}


sample_set_t* sample_set_new(size_t len)
{
	sample_set_t *s = xmalloc(sizeof(sample_set_t));
	s->len = len;
	s->set = xmalloc(len * sizeof(sample_t));
	return s;
}

void sample_set_free(sample_set_t *s)
{
	if (!s) return;
	free(s->set);
	free(s);
}

sample_t* sample_find(sample_set_t *s, const char *key)
{
	assert(s);
	assert(key);

	size_t i;
	for (i = 0; i < s->next; i++)
		if (strcmp(E(s,i)->name, key) == 0)
			return E(s,i);
	return sample_next(s, key);
}

sample_t* sample_next(sample_set_t *s, const char *key)
{
	assert(s);
	assert(key);

	errno = ENOBUFS;
	if (s->next >= s->len)
		return NULL;

	memset(E(s,s->next), 0, sizeof(sample_t));
	strncpy(E(s,s->next)->name, key, METRIC_NAME_MAX);
	return E(s, s->next++);
}

int sample_to_string(sample_t *s, char *buf, size_t max)
{
	assert(s);
	assert(buf);
	assert(max > 0);

	size_t n = snprintf(buf, max, "%s=%lu %Le %Le %Le %Le %Le %Le",
			sample_name(s), sample_n(s),
			sample_sum(s), sample_min(s), sample_max(s),
			sample_mean(s), sample_variance(s), sample_stddev(s));

	if (n < max)
		return 0;

	errno = ENOBUFS;
	return -1;
}

int sample_add(sample_t *s, long double v)
{
	assert(s);

	if (s->n == 0) {
		s->min = s->max = v;

	} else {
		if (v < s->min) s->min = v;
		if (v > s->max) s->max = v;
	}
	s->sum += v;
	s->n++;

	/* incremental mean calculation */
	s->mean_ = s->mean;
	s->mean = s->mean_ + (v - s->mean_) / s->n;

	/* incremental variance calculation */
	s->var_ = s->var;
	s->var = ( (s->n - 1) * s->var_ + ( (v - s->mean_) * (v - s->mean) ) ) / s->n;

	return 0;
}

int sample_reset(sample_t *s)
{
	assert(s);

	s->n = 0;

	s->sum = s->min = s->max = 0.0;
	s->mean  = s->mean_ = 0.0;
	s->var   = s->var_  = 0.0;

	return 0;
}

packet_t* packet_new(int version)
{
	errno = EINVAL;
	if (version != PACKET_V1) return NULL;

	packet_t *p = xmalloc(sizeof(packet_t));
	p->magic[0] = 'S';
	p->magic[1] = 'T';
	p->magic[2] = 'A';
	p->magic[3] = 'T';
	p->version = version + '0';
	return p;
}

void packet_free(packet_t *p)
{
	free(p);
}

int packet_is_valid(packet_t *p)
{
	return p
	    && p->magic[0] == 'S'
	    && p->magic[1] == 'T'
	    && p->magic[2] == 'A'
	    && p->magic[3] == 'T'
	    && p->version  == '1';
}

int packet_set_metric(packet_t *p, const char *name)
{
	assert(p);
	return !strncpy(p->metric, name, METRIC_NAME_MAX);
}

int packet_set_payload(packet_t *p, const char *payload)
{
	assert(p);
	return !strncpy(p->payload, payload, PACKET_PAYLOAD_SIZE);
}

int packet_version(packet_t *p)
{
	assert(p);
	if (p->version == '1') return PACKET_V1;
	errno = EINVAL;
	return -1;
}

long double packet_payload_ld(packet_t *p)
{
	assert(p);
	char *end;
	long double v;

	int esave = errno;
	errno = 0;

	v = strtold(packet_payload(p), &end);
	if (*end)
		errno = EINVAL;

	if (errno != 0)
		return 0.0L;

	errno = esave;
	return v;
}

uint8_t packet_payload_u8(packet_t *p)
{
	assert(p);
	char *end;
	long unsigned v;

	int esave = errno;
	errno = 0;

	v = strtoul(packet_payload(p), &end, 0);
	if (*end)
		errno = EINVAL;
	if (v > 0xff)
		errno = ERANGE;
	if (errno != 0)
		return 0u;

	errno = esave;
	return v & 0xff;
}
