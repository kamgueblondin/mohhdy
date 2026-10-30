/* Lot 61 suites. Each run function registers its own cases. */
#include "webeng_lot61_common.inc"
#include "webeng_lot61_names.inc"
#include "webeng_lot61_repeat.inc"
#include "webeng_lot61_baseline.inc"
#include "webeng_lot61_subgrid.inc"

static void lot61_run(void) {
    lot61_names_run();
    lot61_repeat_run();
    lot61_baseline_run();
    lot61_subgrid_run();
}
