/* Lot 63 suites. Each run function registers its own cases. */
#include "webeng_lot63_gen.inc"
#include "webeng_lot63_text.inc"
#include "webeng_lot63_radial.inc"
#include "webeng_lot63_transform.inc"
#include "webeng_lot63_border.inc"
#include "webeng_lot63_clip.inc"
#include "webeng_lot63_page.inc"

static void lot63_run(void) {
    lot63_gen_run();
    lot63_text_run();
    lot63_radial_run();
    lot63_transform_run();
    lot63_border_run();
    lot63_clip_run();
    lot63_page_run();
}
