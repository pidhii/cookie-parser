#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NONE "NONE"

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
int skip_ows(char* s) {
  int ret =  (s[0] == '\r' && s[1] == '\n' && is_ws(s[2])) ? 3 : is_ws(*s) ? 1 : 0;
  // printf("OWS: %s = %d\n", s, ret);
  return ret;
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
char* parse_cookie_name(char* s) {
  // MUST contain at least one token.
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
char* parse_cookie_value(char* s, int* _isquoted_) {
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
}

typedef struct { uint16_t key, val; } _cookie_keyval;

typedef struct cookie {
  char* data;
  _cookie_keyval* kvs;
  uint16_t nkvs;
} cookie;

/*
 * 
 */
static
cookie* cookie_load(cookie* self, const char* _str) {
  int nkvs = 0;
  int str_len = strlen(_str);
  _cookie_keyval kvs[str_len];
  char* data;
  char *p1, *p2;
  int qq;

  if (!(data = (char*)malloc(str_len + 1)))
    return NULL;
  memcpy(data, _str, str_len + 1);

  /*
   * RFC 4.2.1: ``cookie-header = "Cookie:" OWS cookie-string OWS``
   * => Trim forwarding OWS.
   */
  p1 = data;
  while ((qq = skip_ows(p1)))
    p1 += qq;

  while (1) {
    // Parse cookie-name.
    p2 = parse_cookie_name(p1);
    if (p2 == p1 || *p2 != '=')
      goto L_ERR;
    kvs[nkvs].key = p1 - data;
    *p2++ = 0;

    // Parse cookie-value.
    if (!(p1 = parse_cookie_value(p2, &qq))) {
      goto L_ERR;
    } else if (qq) {
      p2++;
      p1[-1] = 0;
    }
    kvs[nkvs].val = p2 - data;
    nkvs++;

    // Process end/delimiter.
    if (!*p1 || (qq = skip_ows(p1))) {
      *p1 = 0;

      // Check trailing OWS.
      p1 += qq;
      while (( qq = skip_ows(p1) ))
        p1 += qq;
      if (*p1)
        goto L_ERR;

      // End.
      break;

    } else if (*p1++ != ';' || *p1++ != ' ')
      goto L_ERR;

    // Delimiter for value-string.
    p1[-2] = 0;
  }

  // Fill in the structure.
  self->nkvs = nkvs;
  self->data = data;
  if (!(self->kvs = (_cookie_keyval*)malloc(nkvs * sizeof(_cookie_keyval))))
    goto L_ERR;
  memcpy(self->kvs, kvs, nkvs * sizeof(_cookie_keyval));

  return self;

L_ERR:
  free(data);
  return NULL;
}

static inline
void cookie_free(cookie* self) {
  free(self->data);
  free(self->kvs);
}

#define KEY(__self__, __i__) ((__self__)->data + (__self__)->kvs[__i__].key)
#define VAL(__self__, __i__) ((__self__)->data + (__self__)->kvs[__i__].val)

/*
 * The user agent SHOULD sort the cookie-list in the following order:
 *     *  Cookies with longer paths are listed before cookies with
 *        shorter paths.
 *  ...
 */  
static inline
const char* cookie_val(const cookie* self, const char* _key) {
  for (int i = self->nkvs - 1; i >= 0; i--) {
    if (strcmp(KEY(self, i), _key) == 0)
      return VAL(self, i);
  }
  return NONE;
}
