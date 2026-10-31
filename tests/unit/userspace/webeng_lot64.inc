/* Lot 64 suites. Each run function registers its own cases. */
#include "webeng_lot64_vertical.inc"
#include "webeng_lot64_rtl.inc"
#include "webeng_lot64_radial.inc"
#include "webeng_lot64_arc.inc"
#include "webeng_lot64_bglayers.inc"
#include "webeng_lot64_marker.inc"
#include "webeng_lot64_columns.inc"
#include "webeng_lot64_edges.inc"

static void lot64_run(void) {
    lot64_vertical_run();
    lot64_rtl_run();
    lot64_radial_run();
    lot64_arc_run();
    lot64_bglayers_run();
    lot64_marker_run();
    lot64_columns_run();
    lot64_edges_run();
}
