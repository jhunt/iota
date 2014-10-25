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
#include "test.h"

int within(long double x, long double y, long double e)
{
	long double d = x - y;
	return d * (d < 0.0 ? -1 : 1) < e;
}

TESTS {
	sample_t _s, *s = &_s;
	sample_reset(s);

	sample_add(s, 15.0);
	sample_add(s, 10.0);
	sample_add(s, 20.0);

	ok(sample_n(s) == 3, "have 3 samples");
	ok(within(sample_min(s),      10.000, 0.001), "minimum value of the set is ~10.0");
	ok(within(sample_max(s),      20.000, 0.001), "maximum value of the set is ~20.0");
	ok(within(sample_mean(s),     15.000, 0.001), "mean value of the set is ~15.0");
	ok(within(sample_variance(s), 16.667, 0.001), "variance of the set is ~16.667");
	ok(within(sample_stddev(s),    4.082, 0.001), "standard deviation of the set is ~4.082");

	/****************************************/

	sample_reset(s);

	ok(sample_n(s) == 0, "have no samples");
	ok(within(sample_min(s),      0.000, 0.001), "minimum value of the set is ~0.000");
	ok(within(sample_max(s),      0.000, 0.001), "maximum value of the set is ~0.000");
	ok(within(sample_mean(s),     0.000, 0.001), "mean value of the set is ~0.000");
	ok(within(sample_variance(s), 0.000, 0.001), "variance of the set is ~0.000");
	ok(within(sample_stddev(s),   0.000, 0.001), "standard deviation of the set is ~0.000");

	/****************************************/

	sample_reset(s);
	sample_add(s, 1.444);

	ok(sample_n(s) == 1, "have only one sample");
	ok(within(sample_min(s),      1.444, 0.001), "minimum value of the set is ~1.444");
	ok(within(sample_max(s),      1.444, 0.001), "maximum value of the set is ~1.444");
	ok(within(sample_mean(s),     1.444, 0.001), "mean value of the set is ~1.444");
	ok(within(sample_variance(s), 0.000, 0.001), "variance of the set is ~0.000");
	ok(within(sample_stddev(s),   0.000, 0.001), "standard deviation of the set is ~0.000");

	/****************************************/

	sample_reset(s);
	sample_add(s, 3.0);
	sample_add(s, 14.0);
	sample_add(s, 159.0);
	sample_add(s, 1.0);
	sample_add(s, 1.0);
	sample_add(s, 1.0);
	sample_add(s, 1.0);
	sample_add(s, 1.0);
	sample_add(s, 1.0);

	ok(sample_n(s) == 9, "have 9 samples");
	ok(within(sample_min(s),         1.000, 0.001), "minimum value of the set is ~1.0");
	ok(within(sample_max(s),       159.000, 0.001), "maximum value of the set is ~159.0");
	ok(within(sample_mean(s),       20.222, 0.001), "mean value of the set is ~20.222");
	ok(within(sample_variance(s), 2423.506, 0.001), "variance of the set is ~2423.506");
	ok(within(sample_stddev(s),     49.229, 0.001), "standard deviation of the set is ~49.229");

	/*
	diag("n/min/max = %lu/%Lf/%Lf", sample_n(s), sample_min(s), sample_max(s));
	diag("mean/var  = %Lf/%Lf", sample_mean(s), sample_variance(s));
	*/

	done_testing();
}
