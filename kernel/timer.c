#include "timer.h"
#include "sbi.h"

void set_next_trigger(void) {
    set_timer(get_time() + CLOCK_FREQ / TICKS_PER_SEC);
}