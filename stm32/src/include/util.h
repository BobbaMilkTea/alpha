#ifndef __UTIL_H__
#define __UTIL_H__

void micro_wait(unsigned int n);
void tim6_init();
void tim6_start();
int tim6_stop();
int value_check(int actual, int expected, int tolerance);

#endif
