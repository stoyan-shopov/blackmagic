#include <stdint.h>
#include "target/adiv5.h"

struct short_struct {
	uint16_t	a;
	uint8_t		x : 2;
	uint8_t		: 3;
	uint8_t		y : 1;
	struct
	{
		uint8_t		z : 3;
		uint8_t		: 2;
		uint8_t		zz : 3;
	};
	
	union un
	{
		uint8_t		t : 2;
		uint8_t		r : 5;
	} u;
};
struct longer_struct { float a, b, c, d; };
struct longest_struct { double a, b, c, d, e, f; };


struct short_struct f_struct_short(struct short_struct s, int sx, long long sy, float sz, double szz, int aa);
struct longer_struct f_struct_longer(struct longer_struct s);
struct longest_struct f_struct_longest(struct longest_struct s, int aa, float bb, double cc);

void do_adiv5_dp_write_proxy(ADIv5_DP_t *dp, uint16_t addr, uint32_t value);

int param_test(int a, int b, int c, int d, int e);

