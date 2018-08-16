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
int skip_ows(const char* s) {
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

typedef struct { uint32_t key, val; } _cookie_keyval;

typedef struct cookie {
  _cookie_keyval* kvs;
  uint32_t nkvs;
  uint32_t last_val_len;
} cookie;

static
cookie* cookie_load(cookie* self, const char* data) {
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
    if (p2 == p1 || *p2++ != '=') {
      // fprintf(stderr, "ERR at %d\n", __LINE__);
      return NULL;
    }
    kvs[nkvs].key = p1 - data;

    // Parse cookie-value.
    if (!(p1 = parse_cookie_value(p2, &qq))) {
      // fprintf(stderr, "ERR at %d\n", __LINE__);
      return NULL;
    }
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
      if (*p1) {
        // fprintf(stderr, "ERR at %d: *p1 = 0x%x\n", __LINE__, *(uint8_t*)p1);
        return NULL;
      }

      // End.
      // fputs("end\n", stderr);
      break;

    } else if (*p1++ != ';' || *p1++ != ' ') {
      // fprintf(stderr, "ERR at %d\n", __LINE__);
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
 *  ...
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
