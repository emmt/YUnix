/*
 * yor_unix.c --
 *
 * Implements Yorick plug-in to access low-level Unix functions.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2015: Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>
 *
 * See LICENSE.md for details.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include <pstdlib.h>
#include <play.h>
#include <yapi.h>

/* Define some macros to get rid of some GNU extensions when not compiling
   with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#   define __attribute__(x)
#   define __inline__
#   define __FUNCTION__        ""
#   define __PRETTY_FUNCTION__ ""
#endif

#define TRUE  1
#define FALSE 0

PLUG_API void y_error(const char *) __attribute__ ((noreturn));

static long get_size(int type);
static void push_string(const char* str);
static void define_int_const(const char* name, int value);

/*---------------------------------------------------------------------------*/
/* PSEUDO-OBJECTS FOR FILE DESCRIPTORS */

static void yfd_free(void*);
static void yfd_print(void*);
static void yfd_eval(void*, int);
static void yfd_extract(void*, char*);

typedef struct _yfd yfd_t;

struct _yfd {
  char* path;
  int ready;
  int fd;
  unsigned int flags, mode;
};

static y_userobj_t yfd_type = {
  "File descriptor", yfd_free, yfd_print, yfd_eval, yfd_extract
};

static void
yfd_free(void* addr)
{
  yfd_t* obj = (yfd_t*)addr;
  if (obj->ready) {
    obj->ready = 0;
    if (obj->path != NULL) {
      p_free(obj->path);
    }
    if (obj->fd >= 0) {
      close(obj->fd);
    }
  }
}

static void
yfd_print(void* addr)
{
  char buf[32];
  yfd_t* obj = (yfd_t*)addr;

  y_print(yfd_type.type_name, FALSE);
  if (! obj->ready) {
    y_print(" (uninitialized)", TRUE);
  } else {
    y_print(":", TRUE);
    if (obj->path == NULL) {
      y_print("  Path: NULL", TRUE);
    } else {
      y_print("  Path: \"", FALSE);
      y_print(obj->path, FALSE);
      y_print("\"", TRUE);
    }
    sprintf(buf, "%d", obj->fd);
    y_print("  Number: ", FALSE);
    y_print(buf, TRUE);
    sprintf(buf, "0x%08x", obj->flags);
    y_print("  Flags: ", FALSE);
    y_print(buf, TRUE);
    sprintf(buf, "%05o", obj->mode);
    y_print("  Mode: ", FALSE);
    y_print(buf, TRUE);
  }
}

static void
yfd_eval(void* addr, int argc)
{
  yfd_t* obj = (yfd_t*)addr;

  if (! obj->ready) {
    y_error("uninitialized file descriptor object");
  }
  y_error("eval method not yet implemented");
}

static void
yfd_extract(void* addr, char* member)
{
  yfd_t* obj = (yfd_t*)addr;

  if (! obj->ready) {
    y_error("uninitialized file descriptor object");
  }
  switch ((member != NULL ? member[0] : '\0')) {
  case 'p':
    if (strcmp(member, "path") == 0) {
      push_string(obj->path);
      return;
    }
    break;
  case 'f':
    if (strcmp(member, "flags") == 0) {
      ypush_int(obj->flags);
      return;
    }
    break;
  case 'm':
    if (strcmp(member, "mode") == 0) {
      ypush_int(obj->mode);
      return;
    }
  case 'n':
    if (strcmp(member, "number") == 0) {
      ypush_int(obj->fd);
      return;
    }
  }
  y_error("bad member name");
}

static yfd_t*
yfd_fetch(int iarg)
{
  return (yfd_t*)yget_obj(iarg, &yfd_type);
}

/*---------------------------------------------------------------------------*/

void
Y_unx_errno(int argc)
{
  ypush_int(errno);
}

void
Y_unx_strerror(int argc)
{
  int errnum;
  if (argc != 1) y_error("expecting exactly 1 argument");
  errnum = (yarg_nil(0) ? errno : ygets_i(0));
  push_string(strerror(errnum));
}

void
Y_unx_native(int argc)
{
  char* inp;
  if (argc != 1) y_error("expecting exactly 1 argument");
  inp = ygets_q(0);
  if (inp != NULL) {
    ypush_q(NULL)[0] = p_native(inp);
  }
}

void
Y_unx_open(int argc)
{
  yfd_t* obj;
  char* path;

  if (argc < 2 || argc > 3) y_error("expecting 2 or 3 arguments");
  path =  ygets_q(argc - 1);
  if (path == NULL || path[0] == '\0') y_error("illegal path");
  obj = (yfd_t*)ypush_obj(&yfd_type, sizeof(yfd_t));
  obj->fd = -1;
  obj->flags = (unsigned int)ygets_i(argc - 1);
  obj->mode = (unsigned int)(argc >= 3 ? ygets_i(argc - 2) : 0);
  obj->ready = TRUE;
  obj->path = p_native(path);
  obj->fd = open(obj->path, (int)obj->flags, (mode_t)obj->mode);
  if (obj->fd < 0) y_error(strerror(errno));
}

void
Y_unx_close(int argc)
{
  yfd_t* obj;
  char* path;
  int fd;

  if (argc != 1) y_error("expecting exactly one argument");
  obj = yfd_fetch(0);
  path = obj->path;
  fd = obj->fd;
  obj->ready = FALSE;
  p_free(path);
  obj->path = NULL;
  close(fd);
  obj->fd = -1;
  obj->flags = 0;
  obj->mode = 0;
  obj->ready = TRUE;
}

void
Y_unx_ioctl(int argc)
{
  yfd_t* obj;
  unsigned long request;
  void* data;
  long ntot;
  long dims[Y_DIMSIZE];
  int retval, type;

  if (argc != 3) y_error("expecting at least 3 arguments");
  obj = yfd_fetch(argc - 1);
  request = (unsigned long)ygets_l(argc - 2);
  data = (void*)ygeta_any(argc - 2, &ntot, dims, &type);

  if (! obj->ready) y_error("uninitialized file descriptor object");
  if (obj->fd < 0)  y_error("file descriptor has been closed");
  retval = ioctl(obj->fd, request, data);
  if (yarg_subroutine()) {
    if (retval == -1) {
      y_error(strerror(errno));
    }
  } else {
    ypush_int(retval);
  }
}

void
Y_unx_lseek(int argc)
{
  yfd_t* obj;
  off_t retval;
  long offset;
  int whence;

  if (argc != 3) y_error("expecting exactly 3 arguments");
  obj = yfd_fetch(argc - 1);
  offset = ygets_l(argc - 2);
  whence = ygets_i(argc - 3);
  if (! obj->ready) y_error("uninitialized file descriptor object");
  if (obj->fd < 0)  y_error("file descriptor has been closed");
  retval = lseek(obj->fd, offset, whence);
  if (yarg_subroutine()) {
    if (retval == (off_t)-1) {
      y_error(strerror(errno));
    }
  } else {
    ypush_long(retval);
  }
}

static void
read_or_write(int argc, int out)
{
  yfd_t* obj;
  unsigned char* buf;
  long ntot, size, offset, count, retval;
  int type;

  if (argc < 2 || argc > 4) y_error("expecting 2 to 4 arguments");
  obj = yfd_fetch(argc - 1);
  if (! obj->ready) y_error("uninitialized file descriptor object");
  if (obj->fd < 0)  y_error("file descriptor has been closed");
  buf = (unsigned char*)ygeta_any(argc - 2, &ntot, NULL, &type);
  size = get_size(type);
  if (size <= 0) y_error("unsupported data type");
  size *= ntot;
  if (argc < 3) {
    offset = 0;
  } else {
    offset = ygets_l(argc - 3);
    if (offset < 0 || offset > size) y_error("out of range offset");
  }
  if (argc < 4) {
    count = size - offset;
  } else {
    count = ygets_l(argc - 4);
    if (count < 0) y_error("bad number of bytes");
    if (offset + count > size) y_error("too many bytes");
  }
  if (count > 0) {
    if (out) {
      retval = write(obj->fd, buf + offset, count);
    } else {
      retval = read(obj->fd, buf + offset, count);
    }
    if (retval != count && yarg_subroutine()) {
      char* msg;
      if (retval == -1) {
        msg = strerror(errno);
      } else if (out) {
        msg = "not all data written";
      } else {
        msg = "short file";
      }
      y_error(msg);
    }
  } else {
    retval = 0;
  }
  ypush_long(retval);
}

void
Y_unx_read(int argc)
{
  read_or_write(argc, 0);
}

void
Y_unx_write(int argc)
{
  read_or_write(argc, 1);
}

void
Y_unx_init(int argc)
{

#define PREFIX "UNX_"
#define DEF_INT(n) define_int_const(PREFIX #n, n)

  define_int_const(PREFIX "SUCCESS",  0);
  define_int_const(PREFIX "FAILURE", -1);

  /* Whence for lseek. */
#ifdef SEEK_SET
  DEF_INT(SEEK_SET);
#endif
#ifdef SEEK_CUR
  DEF_INT(SEEK_CUR);
#endif
#ifdef SEEK_END
  DEF_INT(SEEK_END);
#endif

  /* Flags for open(2) */
#ifdef O_RDONLY
  DEF_INT(O_RDONLY);
#endif
#ifdef O_WRONLY
  DEF_INT(O_WRONLY);
#endif
#ifdef O_RDWR
  DEF_INT(O_RDWR);
#endif
#ifdef O_APPEND
  DEF_INT(O_APPEND);
#endif
#ifdef O_ASYNC
  DEF_INT(O_ASYNC);
#endif
#ifdef O_CLOEXEC
  DEF_INT(O_CLOEXEC);
#endif
#ifdef O_CREAT
  DEF_INT(O_CREAT);
#endif
#ifdef O_DIRECT
  DEF_INT(O_DIRECT);
#endif
#ifdef O_DIRECTORY
  DEF_INT(O_DIRECTORY);
#endif
#ifdef O_EXCL
  DEF_INT(O_EXCL);
#endif
#ifdef O_LARGEFILE
  DEF_INT(O_LARGEFILE);
#endif
#ifdef O_NOATIME
  DEF_INT(O_NOATIME);
#endif
#ifdef O_NOCTTY
  DEF_INT(O_NOCTTY);
#endif
#ifdef O_NOFOLLOW
  DEF_INT(O_NOFOLLOW);
#endif
#ifdef O_NONBLOCK
  DEF_INT(O_NONBLOCK);
#endif
#ifdef O_NDELAY
  DEF_INT(O_NDELAY);
#endif
#ifdef O_PATH
  DEF_INT(O_PATH);
#endif
#ifdef O_SYNC
  DEF_INT(O_SYNC);
#endif
#ifdef O_TMPFILE
  DEF_INT(O_TMPFILE);
#endif
#ifdef O_TRUNC
  DEF_INT(O_TRUNC);
#endif

  /* Permission flags/ */
#ifdef S_IRWXU
  DEF_INT(S_IRWXU);
#endif
#ifdef S_IRUSR
  DEF_INT(S_IRUSR);
#endif
#ifdef S_IWUSR
  DEF_INT(S_IWUSR);
#endif
#ifdef S_IXUSR
  DEF_INT(S_IXUSR);
#endif
#ifdef S_IRWXG
  DEF_INT(S_IRWXG);
#endif
#ifdef S_IRGRP
  DEF_INT(S_IRGRP);
#endif
#ifdef S_IWGRP
  DEF_INT(S_IWGRP);
#endif
#ifdef S_IXGRP
  DEF_INT(S_IXGRP);
#endif
#ifdef S_IRWXO
  DEF_INT(S_IRWXO);
#endif
#ifdef S_IROTH
  DEF_INT(S_IROTH);
#endif
#ifdef S_IWOTH
  DEF_INT(S_IWOTH);
#endif
#ifdef S_IXOTH
  DEF_INT(S_IXOTH);
#endif

  /* Error codes. */
#ifdef EPERM
  DEF_INT(EPERM);
#endif
#ifdef ENOENT
  DEF_INT(ENOENT);
#endif
#ifdef ESRCH
  DEF_INT(ESRCH);
#endif
#ifdef EINTR
  DEF_INT(EINTR);
#endif
#ifdef EIO
  DEF_INT(EIO);
#endif
#ifdef ENXIO
  DEF_INT(ENXIO);
#endif
#ifdef E2BIG
  DEF_INT(E2BIG);
#endif
#ifdef ENOEXEC
  DEF_INT(ENOEXEC);
#endif
#ifdef EBADF
  DEF_INT(EBADF);
#endif
#ifdef ECHILD
  DEF_INT(ECHILD);
#endif
#ifdef EAGAIN
  DEF_INT(EAGAIN);
#endif
#ifdef ENOMEM
  DEF_INT(ENOMEM);
#endif
#ifdef EACCES
  DEF_INT(EACCES);
#endif
#ifdef EFAULT
  DEF_INT(EFAULT);
#endif
#ifdef ENOTBLK
  DEF_INT(ENOTBLK);
#endif
#ifdef EBUSY
  DEF_INT(EBUSY);
#endif
#ifdef EEXIST
  DEF_INT(EEXIST);
#endif
#ifdef EXDEV
  DEF_INT(EXDEV);
#endif
#ifdef ENODEV
  DEF_INT(ENODEV);
#endif
#ifdef ENOTDIR
  DEF_INT(ENOTDIR);
#endif
#ifdef EISDIR
  DEF_INT(EISDIR);
#endif
#ifdef EINVAL
  DEF_INT(EINVAL);
#endif
#ifdef ENFILE
  DEF_INT(ENFILE);
#endif
#ifdef EMFILE
  DEF_INT(EMFILE);
#endif
#ifdef ENOTTY
  DEF_INT(ENOTTY);
#endif
#ifdef ETXTBSY
  DEF_INT(ETXTBSY);
#endif
#ifdef EFBIG
  DEF_INT(EFBIG);
#endif
#ifdef ENOSPC
  DEF_INT(ENOSPC);
#endif
#ifdef ESPIPE
  DEF_INT(ESPIPE);
#endif
#ifdef EROFS
  DEF_INT(EROFS);
#endif
#ifdef EMLINK
  DEF_INT(EMLINK);
#endif
#ifdef EPIPE
  DEF_INT(EPIPE);
#endif
#ifdef EDOM
  DEF_INT(EDOM);
#endif
#ifdef ERANGE
  DEF_INT(ERANGE);
#endif
#ifdef EDEADLK
  DEF_INT(EDEADLK);
#endif
#ifdef ENAMETOOLONG
  DEF_INT(ENAMETOOLONG);
#endif
#ifdef ENOLCK
  DEF_INT(ENOLCK);
#endif
#ifdef ENOSYS
  DEF_INT(ENOSYS);
#endif
#ifdef ENOTEMPTY
  DEF_INT(ENOTEMPTY);
#endif
#ifdef ELOOP
  DEF_INT(ELOOP);
#endif
#ifdef EWOULDBLOCK
  DEF_INT(EWOULDBLOCK);
#endif
#ifdef ENOMSG
  DEF_INT(ENOMSG);
#endif
#ifdef EIDRM
  DEF_INT(EIDRM);
#endif
#ifdef ECHRNG
  DEF_INT(ECHRNG);
#endif
#ifdef EL2NSYNC
  DEF_INT(EL2NSYNC);
#endif
#ifdef EL3HLT
  DEF_INT(EL3HLT);
#endif
#ifdef EL3RST
  DEF_INT(EL3RST);
#endif
#ifdef ELNRNG
  DEF_INT(ELNRNG);
#endif
#ifdef EUNATCH
  DEF_INT(EUNATCH);
#endif
#ifdef ENOCSI
  DEF_INT(ENOCSI);
#endif
#ifdef EL2HLT
  DEF_INT(EL2HLT);
#endif
#ifdef EBADE
  DEF_INT(EBADE);
#endif
#ifdef EBADR
  DEF_INT(EBADR);
#endif
#ifdef EXFULL
  DEF_INT(EXFULL);
#endif
#ifdef ENOANO
  DEF_INT(ENOANO);
#endif
#ifdef EBADRQC
  DEF_INT(EBADRQC);
#endif
#ifdef EBADSLT
  DEF_INT(EBADSLT);
#endif
#ifdef EDEADLOCK
  DEF_INT(EDEADLOCK);
#endif
#ifdef EBFONT
  DEF_INT(EBFONT);
#endif
#ifdef ENOSTR
  DEF_INT(ENOSTR);
#endif
#ifdef ENODATA
  DEF_INT(ENODATA);
#endif
#ifdef ETIME
  DEF_INT(ETIME);
#endif
#ifdef ENOSR
  DEF_INT(ENOSR);
#endif
#ifdef ENONET
  DEF_INT(ENONET);
#endif
#ifdef ENOPKG
  DEF_INT(ENOPKG);
#endif
#ifdef EREMOTE
  DEF_INT(EREMOTE);
#endif
#ifdef ENOLINK
  DEF_INT(ENOLINK);
#endif
#ifdef EADV
  DEF_INT(EADV);
#endif
#ifdef ESRMNT
  DEF_INT(ESRMNT);
#endif
#ifdef ECOMM
  DEF_INT(ECOMM);
#endif
#ifdef EPROTO
  DEF_INT(EPROTO);
#endif
#ifdef EMULTIHOP
  DEF_INT(EMULTIHOP);
#endif
#ifdef EDOTDOT
  DEF_INT(EDOTDOT);
#endif
#ifdef EBADMSG
  DEF_INT(EBADMSG);
#endif
#ifdef EOVERFLOW
  DEF_INT(EOVERFLOW);
#endif
#ifdef ENOTUNIQ
  DEF_INT(ENOTUNIQ);
#endif
#ifdef EBADFD
  DEF_INT(EBADFD);
#endif
#ifdef EREMCHG
  DEF_INT(EREMCHG);
#endif
#ifdef ELIBACC
  DEF_INT(ELIBACC);
#endif
#ifdef ELIBBAD
  DEF_INT(ELIBBAD);
#endif
#ifdef ELIBSCN
  DEF_INT(ELIBSCN);
#endif
#ifdef ELIBMAX
  DEF_INT(ELIBMAX);
#endif
#ifdef ELIBEXEC
  DEF_INT(ELIBEXEC);
#endif
#ifdef EILSEQ
  DEF_INT(EILSEQ);
#endif
#ifdef ERESTART
  DEF_INT(ERESTART);
#endif
#ifdef ESTRPIPE
  DEF_INT(ESTRPIPE);
#endif
#ifdef EUSERS
  DEF_INT(EUSERS);
#endif
#ifdef ENOTSOCK
  DEF_INT(ENOTSOCK);
#endif
#ifdef EDESTADDRREQ
  DEF_INT(EDESTADDRREQ);
#endif
#ifdef EMSGSIZE
  DEF_INT(EMSGSIZE);
#endif
#ifdef EPROTOTYPE
  DEF_INT(EPROTOTYPE);
#endif
#ifdef ENOPROTOOPT
  DEF_INT(ENOPROTOOPT);
#endif
#ifdef EPROTONOSUPPORT
  DEF_INT(EPROTONOSUPPORT);
#endif
#ifdef ESOCKTNOSUPPORT
  DEF_INT(ESOCKTNOSUPPORT);
#endif
#ifdef EOPNOTSUPP
  DEF_INT(EOPNOTSUPP);
#endif
#ifdef EPFNOSUPPORT
  DEF_INT(EPFNOSUPPORT);
#endif
#ifdef EAFNOSUPPORT
  DEF_INT(EAFNOSUPPORT);
#endif
#ifdef EADDRINUSE
  DEF_INT(EADDRINUSE);
#endif
#ifdef EADDRNOTAVAIL
  DEF_INT(EADDRNOTAVAIL);
#endif
#ifdef ENETDOWN
  DEF_INT(ENETDOWN);
#endif
#ifdef ENETUNREACH
  DEF_INT(ENETUNREACH);
#endif
#ifdef ENETRESET
  DEF_INT(ENETRESET);
#endif
#ifdef ECONNABORTED
  DEF_INT(ECONNABORTED);
#endif
#ifdef ECONNRESET
  DEF_INT(ECONNRESET);
#endif
#ifdef ENOBUFS
  DEF_INT(ENOBUFS);
#endif
#ifdef EISCONN
  DEF_INT(EISCONN);
#endif
#ifdef ENOTCONN
  DEF_INT(ENOTCONN);
#endif
#ifdef ESHUTDOWN
  DEF_INT(ESHUTDOWN);
#endif
#ifdef ETOOMANYREFS
  DEF_INT(ETOOMANYREFS);
#endif
#ifdef ETIMEDOUT
  DEF_INT(ETIMEDOUT);
#endif
#ifdef ECONNREFUSED
  DEF_INT(ECONNREFUSED);
#endif
#ifdef EHOSTDOWN
  DEF_INT(EHOSTDOWN);
#endif
#ifdef EHOSTUNREACH
  DEF_INT(EHOSTUNREACH);
#endif
#ifdef EALREADY
  DEF_INT(EALREADY);
#endif
#ifdef EINPROGRESS
  DEF_INT(EINPROGRESS);
#endif
#ifdef ESTALE
  DEF_INT(ESTALE);
#endif
#ifdef EUCLEAN
  DEF_INT(EUCLEAN);
#endif
#ifdef ENOTNAM
  DEF_INT(ENOTNAM);
#endif
#ifdef ENAVAIL
  DEF_INT(ENAVAIL);
#endif
#ifdef EISNAM
  DEF_INT(EISNAM);
#endif
#ifdef EREMOTEIO
  DEF_INT(EREMOTEIO);
#endif
#ifdef EDQUOT
  DEF_INT(EDQUOT);
#endif
#ifdef ENOMEDIUM
  DEF_INT(ENOMEDIUM);
#endif
#ifdef EMEDIUMTYPE
  DEF_INT(EMEDIUMTYPE);
#endif
#ifdef ECANCELED
  DEF_INT(ECANCELED);
#endif
#ifdef ENOKEY
  DEF_INT(ENOKEY);
#endif
#ifdef EKEYEXPIRED
  DEF_INT(EKEYEXPIRED);
#endif
#ifdef EKEYREVOKED
  DEF_INT(EKEYREVOKED);
#endif
#ifdef EKEYREJECTED
  DEF_INT(EKEYREJECTED);
#endif
#ifdef EOWNERDEAD
  DEF_INT(EOWNERDEAD);
#endif
#ifdef ENOTRECOVERABLE
  DEF_INT(ENOTRECOVERABLE);
#endif
#ifdef ERFKILL
  DEF_INT(ERFKILL);
#endif
#ifdef EHWPOISON
  DEF_INT(EHWPOISON);
#endif


#undef DEF_INT
#undef PREFIX

  ypush_nil();
}

/*---------------------------------------------------------------------------*/
/* UTILITIES */

static long
get_size(int type)
{
  switch (type) {
  case Y_CHAR: return sizeof(char);
  case Y_SHORT: return sizeof(short);
  case Y_INT: return sizeof(int);
  case Y_LONG: return sizeof(long);
  case Y_FLOAT: return sizeof(float);
  case Y_DOUBLE: return sizeof(double);
  case Y_COMPLEX: return 2*sizeof(double);
  default: return -1;
  }
}

static void
push_string(const char* str)
{
  ypush_q(NULL)[0] = p_strcpy(str);
}

static void
define_int_const(const char* name, int value)
{
  ypush_int(value);
  yput_global(yget_global(name, 0), 0);
  yarg_drop(1);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * fill-column: 79
 * coding: utf-8
 * ispell-local-dictionary: "american"
 * End:
 */
