/* test_osui_form.c - Unit test for reusable GUI form component (PR 4) */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../../userspace/osui_form.h"
#include "../../../userspace/fsops.h"
#include "../../../userspace/osui_app.h"

static void test_form_collab_offer(void) {
    osui_form_t f;
    osui_app_view_t v;
    osui_form_collab_offer_init(&f);

    assert(f.num_fields == 4);
    assert(f.active_field == 0);
    assert(strcmp(f.title, "Formulaire: Creer une offre (Collab)") == 0);
    assert(strcmp(f.fields[0].value, "t1") == 0);

    /* Render test */
    osui_form_render(&f, &v);
    assert(v.nrows > 0);

    /* Backspace default value "t1" */
    osui_form_key(&f, '\b', NULL);
    osui_form_key(&f, '\b', NULL);
    assert(f.fields[0].val_len == 0);

    /* Input text in field 0 (Task) */
    osui_form_key(&f, 'T', NULL);
    osui_form_key(&f, 'a', NULL);
    osui_form_key(&f, 's', NULL);
    osui_form_key(&f, 'k', NULL);
    osui_form_key(&f, '1', NULL);
    assert(strcmp(f.fields[0].value, "Task1") == 0);

    /* Tab to field 1 (Amount) */
    osui_form_key(&f, 9, NULL); /* Tab */
    assert(f.active_field == 1);

    /* Down arrow to field 2 (Description) */
    osui_form_key(&f, 31, NULL); /* Down */
    assert(f.active_field == 2);

    /* Down arrow to submit button */
    osui_form_key(&f, 31, NULL); /* Down */
    assert(f.active_field == 3);

    printf("test_form_collab_offer passed\n");
}

static void test_grep_pagination(void) {
    fsops_io_t io;
    int hits = 0, total = 0, files = 0, cut = 0;
    /* Basic sanity check for fsops_grep_page symbol availability */
    memset(&io, 0, sizeof(io));
    hits = fsops_grep_page(&io, "/nonexistent", "motif", 0, 64, NULL, NULL, &total, &files, &cut);
    assert(hits < 0);
    printf("test_grep_pagination passed\n");
}

int main(void) {
    test_form_collab_offer();
    test_grep_pagination();
    printf("All osui_form tests passed successfully!\n");
    return 0;
}
