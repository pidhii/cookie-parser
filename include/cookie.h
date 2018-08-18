#ifndef COOKIE_H
#define COOKIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NONE "NONE"

typedef struct { uint32_t key, val; } _cookie_keyval;

typedef struct cookie {
  _cookie_keyval* kvs;
  uint32_t nkvs;
  uint32_t last_val_len;
} cookie;

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
static inline
int is_sep(char c) {
  static char seps[] = "()<>@,;:\\\"/[]?={} \t";
  int i;
  for (i = 0; seps[i] != c && seps[i]; i++);
  return seps[i] == c;
}

/*
 * DESCRIPTION:
 * Check if octet is a valid cookie-octet (according to RFC2616):
 * ``
 * cookie-octet = %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E
 * ``
 */
static inline
int is_cookie_octet(char c) {
  return (0x21 <= c && c <= 0x7E) 
      && c != 0x22 && c != 0x2C
      && c != 0x3B && c != 0x5C;
}

/*
 * DESCRIPTION:
 * Check if octet is a valid token element (according to RFC2616):
 * ``
 * token = 1*<any CHAR except CTLs or separators>
 * ``
 */
static inline
int is_token(char c) {
  return 32 <= c && c <= 126 && !is_sep(c);
}

#define is_ws(__c__) ((__c__) == ' ' || (__c__) == '\t' || (__c__) == '\v' || (__c__) == '\n')
static inline
int skip_ows(const char* s) {
  return (s[0] == '\r' && s[1] == '\n' && is_ws(s[2])) ? 3 : is_ws(*s) ? 1 : 0;
}

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
static inline
const char* parse_cookie_name(const char* s) {
  while (is_token(*s++));
  return s - 1;
}

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
static inline
const char* parse_cookie_value(const char* s, int* _isquoted_) {
  *_isquoted_ = *s == '"' && s++;
  while (is_cookie_octet(*s++));
  if (*_isquoted_ && *(s++ - 1) != '"') {
    return NULL;
  }
  return s - 1;
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
extern
const char* cookie_iter(const char* _data, const char** key, int* key_len, const char** val, int* val_len, int* err);

#endif
