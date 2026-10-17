#ifndef MOHHDY_CICD_H
#define MOHHDY_CICD_H
/* Lot 7 (US-108): a real local CI/CD pipeline for PromptMessage programs,
 * in guest C. Stages, each one stops the pipeline when it fails:
 *   1 lint     pm_compile (syntax, limits) of the source
 *   2 build    bytecode image (pm_image_write), re-read and checked
 *   3 test     the test file runs in the VM; it needs >= 1 expect and 0
 *              failures
 *   4 deploy   the image is written to the deploy path
 *   5 verify   the deployed bytes are read back, compared and loaded
 * Local only: no remote repository, no runners, no network. */
#include "promptmessage.h"

typedef struct {
    int stage;          /* last stage reached 1..5, 6 = success */
    int passed;         /* stages passed */
    int failed_stage;   /* 0 or the failing stage */
    int image_bytes;
    int statements;
    unsigned int expects, expect_failed;
    char error[64];
} cicd_result_t;

/* src: program source; test_src: test source (may be the same text);
 * host: file access and output; deploy_path: where the image goes. */
int cicd_run(const char* src, int len, const char* test_src, int test_len, const pm_host_t* host,
             const char* deploy_path, cicd_result_t* r, char* out, int cap);
const char* cicd_stage_name(int stage);

#endif
