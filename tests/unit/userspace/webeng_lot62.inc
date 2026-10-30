/* Lot 62 suites. Each run function registers its own cases. */
#include "webeng_lot62_common.inc"
#include "webeng_lot62_alpha.inc"
#include "webeng_lot62_border.inc"
#include "webeng_lot62_transform.inc"
#include "webeng_lot62_decor.inc"
#include "webeng_lot62_deco.inc"
#include "webeng_lot62_spacing.inc"
#include "webeng_lot62_gradient.inc"

static void lot62_run(void) {
    lot62_alpha_run();
    lot62_border_run();
    lot62_transform_run();
    lot62_decor_run();
    lot62_deco_run();
    lot62_spacing_run();
    lot62_gradient_run();
}
