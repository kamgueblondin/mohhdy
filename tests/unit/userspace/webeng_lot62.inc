/* Lot 62 suites. Each run function registers its own cases. */
#include "webeng_lot62_common.inc"
#include "webeng_lot62_alpha.inc"
#include "webeng_lot62_border.inc"

static void lot62_run(void) {
    lot62_alpha_run();
    lot62_border_run();
}
