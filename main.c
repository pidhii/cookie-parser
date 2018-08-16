#include "cookie.h"
#include <unistd.h>

#define TRY_OR(__try__, __or__) ((__try__) ? (__try__) : (__or__))

int main (int argc, char** argv) {
  int opt;

  char* cc = NULL;
  char* key = NULL;

  while ( (opt = getopt(argc, argv, "c:k:")) >= 0) {
    switch (opt) {
      case 'c':
        cc = strdup(optarg);
        break;

      case 'k':
        key = strdup(optarg);
        break;
    }
  }

  if (!cc) {
    printf("no cookie\n");
    return -1;
  }

  // for (int i = 0; i < c.nkvs; i++) {
  //   char* key = c.data + c.kvs[i].key;
  //   char* val = c.data + c.kvs[i].val;
  //   printf("{\"%s\" : \"%s\"} ", key, val);
  // }
  // printf("\n");

  if (key) {
    char buf[256];
    int id;

    cookie c;
    if (!cookie_load(&c, cc)) {
      printf("error: failed to load cookie.\n");
      return -1;
    }

    id = cookie_find(&c, cc, key);
    if (id >= 0) {
      memcpy(buf, cookie_get_val(&c, cc, id), cookie_val_len(&c, id));
      buf[cookie_val_len(&c, id)] = 0;
      printf("cookie[%s] = %s\n", key, buf);
    }

  } else {
    char key[256];
    char val[256];

    const char* k;
    const char* v;
    const char* p = cc;
    int k_len, v_len, err;
    while ( (p = cookie_iter(p, &k, &k_len, &v, &v_len, &err)), err >= 0) {
      memcpy(key, k, k_len);
      key[k_len] = 0;
      memcpy(val, v, v_len);
      val[v_len] = 0;

      printf("{ \"%s\" : \"%s\" } ", key, val);
      if (err == 0)
        break;
    }
    printf("\n");
    fflush(stdout);
    if (err < 0)
      return -1;
  }
}
