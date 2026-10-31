/* wf_inflate.h - Lot 69: bounded DEFLATE (RFC 1951) decoder with gzip
 * (RFC 1952) and zlib (RFC 1950) wrappers, for HTTP Content-Encoding.
 * No allocation: the compressed bytes are moved to the end of the caller
 * buffer and inflated towards its start, so one buffer of `cap` bytes
 * holds both. Output that would overwrite unread input is cut and
 * reported as truncated. */
#ifndef WF_INFLATE_H
#define WF_INFLATE_H
#include <stdint.h>

#define WF_ENC_IDENTITY 0
#define WF_ENC_GZIP 1
#define WF_ENC_DEFLATE 2   /* "deflate": zlib stream (raw deflate accepted too) */

/* Raw DEFLATE from in[0..in_len) to out[0..out_cap). Returns bytes written,
 * -1 corrupt. *truncated = 1 when out_cap stopped it. *in_used set. The
 * output may share memory with the input only if out is before in and the
 * caller passes limit = distance (see wf_decode_body_inplace). */
int wf_inflate(const uint8_t* in, int in_len, int* in_used, uint8_t* out, int out_cap, int* truncated);

/* Decodes buf[0..len) (gzip or zlib/deflate) in place inside buf[0..cap).
 * Returns the decoded length or -1 (corrupt). *truncated is set when the
 * output did not fit or the input was itself cut short. */
int wf_decode_body_inplace(int enc, uint8_t* buf, int len, int cap, int* truncated);
#endif
