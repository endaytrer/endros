#include "timer.h"
#include "sbi.h"

void set_next_trigger(void) {
    set_timer(get_time() + clock_freq / TICKS_PER_SEC);
}