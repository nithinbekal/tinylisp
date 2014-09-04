#include "value.h"

VAL tl_val_num(long x) {
  VAL v = malloc(sizeof(TLVAL));
  v->type = TL_INTEGER;
  v->num = x;
  return v;
}
