#include "cookie.h"
#include <unistd.h>
#include <assert.h>

#define TRY_OR(__try__, __or__) ((__try__) ? (__try__) : (__or__))

int main (int argc, char** argv) {
  assert(argc == 2);
  char buf[256];
  const char* k;
  const char* v;
  const char* p = argv[1];
  int k_len, v_len, err;

  while ( (p = cookie_iter(p, &k, &k_len, &v, &v_len, &err)), err >= 0) {
    // Key.
    memcpy(buf, k, k_len);
    buf[k_len] = 0;
    puts(buf);

    // Value.
    if (v[0] == '"') {
      v++;
      v_len -= 2;
    }
    memcpy(buf, v, v_len);
    buf[v_len] = 0;
    puts(buf);

    if (err == 0)
      break;
  }

  printf("--END--\n");
  return err;
}
