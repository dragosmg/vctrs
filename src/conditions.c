#include "vctrs.h"
#include "utils.h"

#define DEFAULT_ARG_BUF 100


static r_ssize_t arg_fill(struct vctrs_arg* self, char* buf, r_ssize_t remaining) {
  const char* src = ((struct vctrs_arg_wrapper*) self)->arg;

  size_t len = strlen(src);

  if (len >= remaining) {
    return -1;
  }

  memcpy(buf, src, len);
  buf[len] = '\0';

  return len;
}

struct vctrs_arg_wrapper new_vctrs_arg(struct vctrs_arg* parent, const char* arg) {
  struct vctrs_arg iface = {
    .parent = parent,
    .fill = &arg_fill
  };

  struct vctrs_arg_wrapper wrapper = {
    .iface = iface,
    .arg = arg
  };

  return wrapper;
}

static int fill_arg_buffer(struct vctrs_arg* arg,
                           char* buf,
                           r_ssize_t cur_size,
                           r_ssize_t tot_size) {
  if (arg->parent) {
    cur_size = fill_arg_buffer(arg->parent, buf, cur_size, tot_size);

    if (cur_size < 0) {
      return cur_size;
    }
  }

  r_ssize_t written = arg->fill(arg, buf + cur_size, tot_size - cur_size);

  if (written < 0) {
    return written;
  } else {
    return cur_size + written;
  }
}

SEXP vctrs_arg(struct vctrs_arg* arg) {
  r_ssize_t next_size = DEFAULT_ARG_BUF;
  r_ssize_t size;

  SEXP buf_holder = PROTECT(R_NilValue);
  char* buf;

  do {
    size = next_size;

    UNPROTECT(1);
    buf_holder = PROTECT(Rf_allocVector(RAWSXP, size));
    buf = (char*) RAW(buf_holder);

    // Reallocate a larger buffer at the next iteration if the current
    // buffer turns out too small
    next_size *= 1.5;
  } while (fill_arg_buffer(arg, buf, 0, size) < 0);

  SEXP out = Rf_mkString(buf);

  UNPROTECT(1);
  return out;
}


void stop_scalar_type(SEXP x, struct vctrs_arg* arg) {
  SEXP call = PROTECT(Rf_lang3(Rf_install("stop_scalar_type"),
                               PROTECT(r_protect(x)),
                               PROTECT(vctrs_arg(arg))));
  Rf_eval(call, vctrs_ns_env);
  Rf_error("Internal error: `stop_scalar_type()` should have jumped");
}

void vec_assert(SEXP x, struct vctrs_arg* arg) {
  if (!vec_is_vector(x)) {
    stop_scalar_type(x, arg);
  }
}
