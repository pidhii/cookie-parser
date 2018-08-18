#include "cookie.h"

const char* cookie_iter(const char* _data, const char** _key_, int* _key_len_, const char** _val_, int* _val_len_, int* _err_) {
  const char *p1, *p2;
  int qq;

  // Skip OWS.
  p1 = _data;
  while ((qq = skip_ows(p1)))
    p1 += qq;
  *_key_ = p1;

  // Parse cookie-name.
  p2 = parse_cookie_name(p1);
  // MUST contain at least one token.
  if (p2 == p1 || *p2++ != '=')
    goto L_RET_ERR;

  // Parse cookie-value.
  if (!(p1 = parse_cookie_value(p2, &qq)))
    goto L_RET_ERR;

  *_val_ = p2;
  *_key_len_ = p2 - *_key_ - 1;

  // Process end/delimiter.
  if (!*p1 || skip_ows(p1)) {
    *_val_len_ = p1 - p2;

    // Check trailing OWS.
    while (( qq = skip_ows(p1) )) {
      p1 += qq;
    }
    if (*p1)
      goto L_RET_ERR;

    // End.
    *_err_ = 0;
    return NULL;

  } else if (*p1++ != ';' || *p1++ != ' ') {
    goto L_RET_ERR;
  }

  *_val_len_ = p1 - p2 - 2;

  // Ok + have next pair.
  *_err_ = 1;
  return p1;

L_RET_ERR:
  *_err_ = -1;
  return NULL;
}

