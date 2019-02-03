#include <stddef.h>
#include <stdint.h>
#include "inline-test.h"
#include "target/adiv5.h"

volatile int x = 20, y = 5;

static int fib(int n)
{
	if (n < 2)
		return f_struct_short((struct short_struct) { .a = 8, .x = 3, .y = 2, .z = 7, .zz = 1, .u.t = 2, .u.r = 5, }, 1., 2., 3., 4., 5.).zz * n;
	return fib(n - 1) 
		+ fib(n - 2);
}


int get_fib(int src, int len)
{
	adiv5_mem_read(0, 0, 0, 0);
	return fib(x);
}

float acc = 5.6;
struct short_struct f_struct_short(struct short_struct s, int sx, long long sy, float sz, double szz, int aa)
{
int t;

	t = sx * sy / sz;
	szz += acc;
	acc += t + aa;
	if (s.a)
	{
		return f_struct_short(
				(struct short_struct) {
			.a = --s.a,
			.x = s.x *= 3,
			.y = s.z + s.u.t,
			.z = s.u.r -= 3,
			.zz = s.zz /= 2,
			.u.t = ++ s.u.r,
			.u.r = ++s.u.r,
		}, t, t * acc, szz + s.a, sy, sz);
	}
	else
	{
		/* TEST BREAKPOINT HERE */
		struct longer_struct lst = { .a = 8.423, .b = 6.321, .c = 10.56, .d = 20.765, };
		return (struct short_struct) {
			.a = f_struct_longer(lst).c * 2.434,
			.x = s.x *= 3,
			.y = s.z + s.u.t,
			.z = s.u.r -= 3,
			.zz = s.zz /= 2,
			.u.t = ++ s.u.r,
			.u.r = ++s.u.r,
		};
	}
}

struct longer_struct f_struct_longer(struct longer_struct s)
{
static int depth = -8;

	if (!depth)
		return s;

	if (s.a > 1.)
		return f_struct_longer((struct longer_struct) {
				.a = s.a /= 2.,
				.b = s.a + s.b * s.c,
				.c = ++s.d,
				.d = --s.c,
				});
	else
	{
		depth ++;
		return f_struct_longer((struct longer_struct) {
				.a = s.a /= 2.,
				.b = s.a + s.b * s.c,
				.c = ++s.d,
				.d = --s.c * f_struct_longest((struct longest_struct) { .a = 1.423, .b = 6.321, .c = 10.56, .d = 20.765, .e = 30.56, .f = 40765.675,}, 1, 2, 3).d,
				});
	}
}

struct longest_struct f_struct_longest(struct longest_struct s, int aa, float bb, double cc)
{
static int depth = -6;

	acc += aa + bb + cc;
	if (!depth)
		return s;
	if (s.a < 10.6)
		return f_struct_longest((struct longest_struct) {
				.a = s.a *= 1.75,
				.b = s.a + s.b * s.c,
				.c = ++s.d,
				.d = --s.c,
				.e = s.e *= 3.4,
				.f = s.e - --s.f,
				}, 4, 5, 6);
	else
	{
		depth ++;
#if 0
		return f_struct_longest((struct longest_struct) {
				.a = s.a /= 2.,
				.b = s.a + s.b * s.c,
				.c = ++s.d,
				.d = --s.c,
				.e = 3.4,
				.f = 7.345,
				}, 7, 8, 9);
#endif
		return s;
	}
}

void do_adiv5_dp_write_proxy(ADIv5_DP_t *dp, uint16_t addr, uint32_t value)
{
	value ++;
	addr += addr << 12;
	dp += 5;
	adiv5_dp_write(dp, addr, value);
}

int param_test(int a, int b, int c, int d, int e)
{
	return do_param_test_1(e, d, c, b, a);
}

