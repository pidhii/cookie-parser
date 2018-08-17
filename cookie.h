#ifndef COOKIE_H
#define COOKIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NONE "NONE"


#ifdef COOKIE_TEST
# define COOKIE_COV_TESTS 7
static int cookie_cov_test[COOKIE_COV_TESTS];

#define CAT(A, B) A##B

# define TEST_COV(__fn__) static int __fn__ ## _cov_id = __COUNTER__;
# define ACCEPT_COV(__fn_id__) cookie_cov_test[__fn_id__ ## _cov_id] = 1
# define DECL_COV(__ret_ty__, __fn_id__, ...) \
   TEST_COV(__fn_id__) \
   __ret_ty__ __fn_id__ (__VA_ARGS__) { \
     ACCEPT_COV(__fn_id__);
#else
# define DECL_COV(__ret_ty__, __fn_id__, ...) __ret_ty__ __fn_id__ (__VA_ARGS__) {
#endif

#define  END_DECL_COV }

/*
 * DESCRIPTION:
 * Check if octet is a separator (according to RFC2616):
 * ``
 * separators     = "(" | ")" | "<" | ">" | "@"
 *                | "," | ";" | ":" | "\" | <">
 *                | "/" | "[" | "]" | "?" | "="
 *                | "{" | "}" | SP | HT
 * ``
 */
DECL_COV(static inline int, is_sep, char c)
  static char seps[] = "()<>@,;:\\\"/[]?={} \t";
  int i;
  for (i = 0; seps[i] != c && seps[i]; i++);
  return seps[i] == c;
END_DECL_COV

/*
 * DESCRIPTION:
 * Check if octet is a valid cookie-octet (according to RFC2616):
 * ``
 * cookie-octet = %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E
 * ``
 */
DECL_COV(static inline int, is_cookie_octet, char c)
  return (0x21 <= c && c <= 0x7E) 
      && c != 0x22 && c != 0x2C
      && c != 0x3B && c != 0x5C;
END_DECL_COV

/*
 * DESCRIPTION:
 * Check if octet is a valid token element (according to RFC2616):
 * ``
 * token = 1*<any CHAR except CTLs or separators>
 * ``
 */
DECL_COV(static inline int, is_token, char c)
  return 32 <= c && c <= 126 && !is_sep(c);
END_DECL_COV

#define is_ws(__c__) ((__c__) == ' ' || (__c__) == '\t' || (__c__) == '\v' || (__c__) == '\n')
DECL_COV(static inline int, skip_ows, const char* s)
  int ret =  (s[0] == '\r' && s[1] == '\n' && is_ws(s[2])) ? 3 : is_ws(*s) ? 1 : 0;
  // printf("OWS: %s = %d\n", s, ret);
  return ret;
END_DECL_COV

/*
 * DESCRIPTION:
 * Parse cookie-name (according to RFC6265):
 * ``
 * cookie-name = token
 * ``
 *
 * RETURN VALUE:
 * Pointer on character right after the end of the value.
 */
DECL_COV(static inline const char*, parse_cookie_name, const char* s)
  while (is_token(*s++));
  return s - 1;
END_DECL_COV

/*
 * DESCRIPTION:
 * Parse cookie-value (according to RFC6265):
 * ``
 * cookie-value = *cookie-octet / ( DQUOTE *cookie-octet DQUOTE )
 * ``
 *
 * RETURN VALUE:
 * Pointer on character right after the end of the value.
 */
DECL_COV(static inline const char*, parse_cookie_value, const char* s, int* _isquoted_)
  if (*s == '"') {
    *_isquoted_ = 1;
    s++;
  } else
    *_isquoted_ = 0;

  while (is_cookie_octet(*s++));

  if (*_isquoted_ && *(s++ - 1) != '"') {
    return NULL;
  }
  return s - 1;
END_DECL_COV

typedef struct { uint32_t key, val; } _cookie_keyval;

typedef struct cookie {
  _cookie_keyval* kvs;
  uint32_t nkvs;
  uint32_t last_val_len;
} cookie;

static cookie* cookie_load(cookie* self, const char* data) {
  int nkvs = 0;
  _cookie_keyval kvs[0x1000];
  const char *p1, *p2;
  int qq;

  // Skip OWS.
  p1 = data;
  while ((qq = skip_ows(p1)))
    p1 += qq;

  while (1) {
    // Parse cookie-name.
    p2 = parse_cookie_name(p1);
    // MUST contain at least one token.
    if (p2 == p1 || *p2++ != '=')
      return NULL;
    kvs[nkvs].key = p1 - data;

    // Parse cookie-value.
    if (!(p1 = parse_cookie_value(p2, &qq)))
      return NULL;
    // if (qq)
      // p2 += 1;

    kvs[nkvs].val = p2 - data;
    nkvs++;

    // Process end/delimiter.
    // fputs("finishing\n", stderr);
    if (!*p1 || (qq = skip_ows(p1))) {
      self->last_val_len = p1 - p2;

      // Check trailing OWS.
      while (( qq = skip_ows(p1) )) {
        // fputs("loopng...\n", stderr);
        p1 += qq;
      }
      if (*p1)
        return NULL;

      // End.
      break;

    } else if (*p1++ != ';' || *p1++ != ' ') {
      return NULL;
    }
  }

  // Fill in the structure.
  self->nkvs = nkvs;
  if (!(self->kvs = (_cookie_keyval*)malloc(nkvs * sizeof(_cookie_keyval)))) {
    // fprintf(stderr, "ERR at %d\n", __LINE__);
    return NULL;
  }
  memcpy(self->kvs, kvs, nkvs * sizeof(_cookie_keyval));

  return self;
}

static inline
void cookie_free(cookie* self) {
  free(self->kvs);
}

static inline
const char* cookie_get_val(const cookie* self, const char* _data, int _i) {
  return _data + self->kvs[_i].val;
}

static inline
const char* cookie_get_key(const cookie* self, const char* _data, int _i) {
  return _data + self->kvs[_i].key;
}

static inline
int cookie_key_len(const cookie* self, int _i) {
  return self->kvs[_i].val - self->kvs[_i].key - 1;
}

static inline
int cookie_val_len(const cookie* self, int _i) {
  return _i == self->nkvs - 1 ? self->last_val_len : self->kvs[_i+1].key - self->kvs[_i].val - 2;
}

/*
 * The user agent SHOULD sort the cookie-list in the following order:
 *     *  Cookies with longer paths are listed before cookies with
 *        shorter paths.
 *  => Iterate from the end.
 */  
static
int cookie_find(const cookie* self, const char* _data, const char* _key) {
  const char* key;

  for (int i = self->nkvs - 1; i >= 0; i--) {
    key = _data + self->kvs[i].key;
    if (strncmp(key, _key, cookie_key_len(self, i)) == 0)
      return i;
  }

  return -1;
}

/*
 * DESCRIPTION:
 * Iterate over cookie's key/value-pairs.
 *
 * ARGUMENTS:
 *  - data    [in]   : input cookie string, or pointer returned from last call (see "RETURN VALUE");
 *  - key     [out]  : after succesfull call will point on the cookie-name string;
 *  - key_len [out]  : after succesfull call will contain length of the cookie-name string;
 *  - val     [out]  : after succesfull call will point on the cookie-value string;
 *  - val_len [out]  : after succesfull call will contain length of the cookie-value string.
 *
 * RETURN VALUE:
 * Pointer to be passed to this function for next iteration.
 * Error status is returned via argument `err`:
 *  - parsing succeed:
 *      > 0: next key/value-pair detected;
 *       0 : this is the last key/value-pair.
 *  - parsing error:
 *      < 0: bad cookie.
 */
DECL_COV(static const char*, cookie_iter, const char* _data, const char** _key_, int* _key_len_, const char** _val_, int* _val_len_, int* _err_)
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
END_DECL_COV

#endif // #ifndef COOKIE_H
