#include "target/adiv5.h"

struct short_struct { short a, b; };
struct longer_struct { long a, b, c, d; };
struct long_struct {long long a, b, c, d, e, f; };


void f_struct_short(struct short_struct s);
void f_struct_longer(struct longer_struct s);
void f_struct_long(struct long_struct s);

void do_adiv5_dp_write_proxy(ADIv5_DP_t *dp, uint16_t addr, uint32_t value);

int param_test(int a, int b, int c, int d, int e);

