/* Lot 61 suites. Each run function registers its own cases. */
#include "webeng_lot61_common.inc"
#include "webeng_lot61_names.inc"
#include "webeng_lot61_repeat.inc"
#include "webeng_lot61_baseline.inc"
#include "webeng_lot61_subgrid.inc"
#include "webeng_lot61_viewport.inc"
#include "webeng_lot61_clip.inc"
#include "webeng_lot61_table.inc"
#include "webeng_lot61_collapse.inc"
#include "webeng_lot61_stack.inc"
#include "webeng_lot61_page.inc"

static void lot61_run(void) {
    lot61_names_run();
    lot61_repeat_run();
    lot61_baseline_run();
    lot61_subgrid_run();
    lot61_viewport_run();
    lot61_clip_run();
    lot61_table_run();
    lot61_collapse_run();
    lot61_stack_run();
    lot61_page_run();
}
