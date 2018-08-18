#include "cookie.h"
#include <assert.h>

int main(int argc, char** argv) {
  assert(argc == 2);

  char* buf;
  const char* k;
  const char* v;
  const char* p;
  int k_len, v_len, err;

  buf = malloc(0x1000);

  printf("[");
  p = argv[1];
  while ( (p = cookie_iter(p, &k, &k_len, &v, &v_len, &err)), err >= 0) {
    // Key.
    memcpy(buf, k, k_len);
    buf[k_len] = 0;
    printf("{ \"key\": \"%s\",", buf);

    // Value.
    if (v[0] == '"') {
      v++;
      v_len -= 2;
    }
    memcpy(buf, v, v_len);
    buf[v_len] = 0;
    printf("\"val\": \"%s\"}", buf);

    if (err == 0)
      break;
    else
      printf(",");
  }
  printf("]");

  free(buf);
  return err;
}
