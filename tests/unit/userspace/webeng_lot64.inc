/* Lot 64 suites. Each run function registers its own cases. */
#include "webeng_lot64_vertical.inc"
#include "webeng_lot64_rtl.inc"

static void lot64_run(void) {
    lot64_vertical_run();
    lot64_rtl_run();
}
