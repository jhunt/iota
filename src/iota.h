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
#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>

#define COUNTER_MAX 0xffffffffffffffff

#define METRIC_NAME_MAX     256
#define PACKET_PAYLOAD_SIZE  32
#define PACKET_V1             1

typedef struct {
	char      name[METRIC_NAME_MAX];
	uint64_t  value;
	uint8_t   rollover;
} counter_t;

typedef struct {
	size_t len, next;
	counter_t *set;
} counter_set_t;

typedef struct {
	char      name[METRIC_NAME_MAX];
	uint64_t  n;
	long double min, max;
	long double mean, mean_;
	long double var,  var_;
} sample_t;

typedef struct {
	size_t len, next;
	sample_t *set;
} sample_set_t;

typedef struct {
	char magic[4];
	char version;
	char metric[METRIC_NAME_MAX];
	char payload[PACKET_PAYLOAD_SIZE];
} packet_t;

counter_set_t* counter_set_new(size_t len);
void counter_set_free(counter_set_t *s);
counter_t* counter_at(counter_set_t *s, size_t idx);
counter_t* counter_find(counter_set_t *s, const char *key);
counter_t* counter_next(counter_set_t *s, const char *key);
int counter_to_string(counter_t *c, char *buf, size_t max);

#define counter_name(c)     (c)->name
#define counter_value(c)    (c)->value
#define counter_rollover(c) (c)->rollover

int counter_inc(counter_t *c, uint8_t i);
int counter_reset(counter_t *c);

sample_set_t* sample_set_new(size_t len);
void sample_set_free(sample_set_t *s);
sample_t* sample_at(sample_set_t *s, size_t idx);
sample_t* sample_find(sample_set_t *s, const char *key);
sample_t* sample_next(sample_set_t *s, const char *key);
int sample_to_string(sample_t *c, char *buf, size_t max);

int sample_add(sample_t *s, long double v);
int sample_reset(sample_t *s);

#define sample_name(s)     (s)->name
#define sample_n(s)        (s)->n
#define sample_min(s)      (s)->min
#define sample_max(s)      (s)->max
#define sample_mean(s)     (s)->mean
#define sample_variance(s) (s)->var
#define sample_stddev(s)   sqrtl((s)->var)

packet_t* packet_new(int version);
void packet_free(packet_t *p);
int packet_is_valid(packet_t *p);

int packet_set_metric(packet_t *p, const char *name);
int packet_set_payload(packet_t *p, const char *payload);

int packet_version(packet_t *p);
#define packet_metric(p)  (p)->metric
#define packet_payload(p) (p)->payload
long double packet_payload_ld(packet_t *p);
uint8_t packet_payload_u8(packet_t *p);

#endif
