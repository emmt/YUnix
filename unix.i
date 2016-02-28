/*
 * unix.i --
 *
 * Yorick plug-in to low-level Unix functions.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2015-2016: Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>
 *
 * All rights reserved, see LICENSE.md for details.
 *
 */

if (is_func(plug_in)) plug_in, "yor_unix";

extern unx_errno;
extern unx_strerror;
/* DOCUMENT err = unx_errno();
         or msg = unx_strerror();
         or msg = unx_strerror(err);

      The function unx_errno() returns the last error code of the C library.

      The function unx_strerror() returns an explanatory string for the last
      error code of the C library of for the error code ERR if specified.

      Global variables with the name of the standard error codes (prefixed by
      UNX_) are predefined.  For instance, UNX_EINVAL is set with the value of
      the EINVAL error code.

   SEE ALSO:
 */

extern unx_native;
/* DOCUMENT str = unx_native(path);
     Get the native file name corresponding to PATH.  Leading tilde character
     (e.g., ~ or ~user) and shell variables (e.g., $HOME) are interpolated.

   SEE ALSO: unx_open.
 */

/*---------------------------------------------------------------------------*/
/* LOW LEVEL INPUT/OUTPUT */

extern unx_open;
/* DOCUMENT fd = unx_open(path, flags);
         or fd = unx_open(path, flags, mode);

      Open a file descriptor to access the contents of file PATH.

      It is not necessary to explicitly close the file descriptor (via
      unx_close), this is automatically done when it is no longer referenced
      in Yorick.

      The returned object has the following members:

         fd.path    the path to the file
         fd.number  the file descriptor number
         fd.flags   the value of FLAGS
         fd.mode    the value of MODE

      Until the file descriptor is closed, the values of these members
      correspond to the ones used when the file descriptor was open.


   SEE ALSO: unx_read, unx_write, unx_native, unx_lseek, unx_close.
 */

extern unx_close;
/* DOCUMENT unx_close, fd;
     Explicitly close file descriptor FD.

   SEE ALSO: unx_open.
 */

local UNX_SEEK_SET, UNX_SEEK_CUR, UNX_SEEK_END;
extern unx_lseek;
/* DOCUMENT pos = unx_lseek(fd, off, whence);

     Reposition read/write offset for file descriptor FD.  OFF is the offset
     (in bytes) counted relatively to WHENCE (see below).  The returned value
     is the new offset location as measured in bytes from the beginning of the
     file.  On error, the value -1 is returned (use unx_errno to retrieve the
     error code).

     Argument WHENCE is one of:

         UNX_SEEK_SET - The offset is set to offset bytes.
         UNX_SEEK_CUR - The offset is set to its current location plus offset
                        bytes.
         UNX_SEEK_END - The offset is set to the size of the file plus offset
                        bytes.

   SEE ALSO: unx_open, unx_read, unx_write, unx_errno.
 */

extern unx_read;
/* DOCUMENT n = unx_read(fd, arr);
         or n = unx_read(fd, arr, off);
         or n = unx_read(fd, arr, off, cnt);

     Read raw bytes from file descriptor FD into buffer array ARR.  OFF is an
     optional offset (in bytes) into ARR and CNT is an optional number of
     bytes to read.  By default, OFF = 0 (i.e., read bytes are stored starting
     at the beginning of ARR) and CNT = sizeof(ARR) - OFF (i.e., try to read
     all bytes remaining in ARR after offset OFF).  The returned value N is
     the number of bytes read, or -1 in case of error.

     When called as a subroutine, an error is thrown if the number of read
     bytes is not equal to the request value (CNT).

   SEE ALSO: unx_open, unx_write, unx_errno.
 */

extern unx_write;
/* DOCUMENT n = unx_write(fd, arr);
         or n = unx_write(fd, arr, off);
         or n = unx_write(fd, arr, off, cnt);

     Write raw bytes from buffer array ARR to file descriptor FD.  OFF is an
     optional offset (in bytes) into ARR and CNT is an optional number of
     bytes to write.  By default, OFF = 0 (i.e., written bytes start at the
     beginning of ARR) and CNT = sizeof(ARR) - OFF (i.e., try to write all
     bytes remaining in ARR after offset OFF).  The returned value N is the
     number of bytes written, or -1 in case of error.

     When called as a subroutine, an error is thrown if the number of written
     bytes is not equal to the request value (CNT).

   SEE ALSO: unx_open, unx_write, unx_errno.
 */

/*---------------------------------------------------------------------------*/
/* IOCTL */

extern unx_ioctl;
/* DOCUMENT rv = unx_ioctl(fd, req, ...);
     Perform an IOCTL call on file descriptor FD for request REQ.

   SEE ALSO: unx_open, unx_errno.
 */

/* The following has been extracted from /usr/include/asm-generic/ioctl.h */

/* ioctl command encoding: 32 bits total, command in lower 16 bits,
 * size of the parameter structure in the lower 14 bits of the
 * upper 16 bits.
 * Encoding the size of the parameter structure in the ioctl request
 * is useful for catching programs compiled with old versions
 * and to avoid overwriting user space outside the user buffer area.
 * The highest 2 bits are reserved for indicating the ``access mode''.
 * NOTE: This limits the max parameter size to 16kB -1 !
 */

/*
 * The following is for compatibility across the various Linux
 * platforms.  The generic ioctl numbering scheme doesn't really enforce
 * a type field.  De facto, however, the top 8 bits of the lower 16
 * bits are indeed used as a type field, so we might just as well make
 * this explicit here.  Please be sure to use the decoding macros
 * below from now on.
 */
_UNX_IOC_NRBITS    = 8n;
_UNX_IOC_TYPEBITS  = 8n;

/*
 * Let any architecture override either of the following before
 * including this file.
 */

_UNX_IOC_SIZEBITS  = 14n;
_UNX_IOC_DIRBITS   =  2n;

_UNX_IOC_NRMASK    = ((1n << _UNX_IOC_NRBITS)   - 1n);
_UNX_IOC_TYPEMASK  = ((1n << _UNX_IOC_TYPEBITS) - 1n);
_UNX_IOC_SIZEMASK  = ((1n << _UNX_IOC_SIZEBITS) - 1n);
_UNX_IOC_DIRMASK   = ((1n << _UNX_IOC_DIRBITS)  - 1n);

_UNX_IOC_NRSHIFT   = 0n;
_UNX_IOC_TYPESHIFT = (_UNX_IOC_NRSHIFT   + _UNX_IOC_NRBITS);
_UNX_IOC_SIZESHIFT = (_UNX_IOC_TYPESHIFT + _UNX_IOC_TYPEBITS);
_UNX_IOC_DIRSHIFT  = (_UNX_IOC_SIZESHIFT + _UNX_IOC_SIZEBITS);

/*
 * Direction bits, which any architecture can choose to override
 * before including this file.
 */

_UNX_IOC_NONE      = 0n;
_UNX_IOC_WRITE     = 1n;
_UNX_IOC_READ      = 2n;

func _UNX_IOC(dir, type, nr, size)
{
  return ((int(dir)  << _UNX_IOC_DIRSHIFT)  |
          (int(type) << _UNX_IOC_TYPESHIFT) |
          (int(nr)   << _UNX_IOC_NRSHIFT)   |
          (int(size) << _UNX_IOC_SIZESHIFT));
}

/* used to create numbers */
func _UNX_IO(type, nr) {
  return _UNX_IOC(_UNX_IOC_NONE, type, nr, 0);
}
func _UNX_IOR(type, nr, size) {
  return _UNX_IOC(_UNX_IOC_READ, type, nr, sizeof(size));
}
func _UNX_IOW(type, nr, size) {
  return _UNX_IOC(_UNX_IOC_WRITE, type, nr, sizeof(size));
}
func _UNX_IOWR(type, nr, size) {
  return _UNX_IOC(_UNX_IOC_READ|_UNX_IOC_WRITE, type, nr, sizeof(size));
}
func _UNX_IOR_BAD(type, nr, size) {
  return _UNX_IOC(_UNX_IOC_READ, type, nr, sizeof(size));
}
func _UNX_IOW_BAD(type, nr, size) {
  return _UNX_IOC(_UNX_IOC_WRITE, type, nr, sizeof(size));
}
func _UNX_IOWR_BAD(type, nr, size) {
  return _UNX_IOC(_UNX_IOC_READ|_UNX_IOC_WRITE, type, nr, sizeof(size));
}

/* used to decode ioctl numbers.. */
func _UNX_IOC_DIR(nr) {
  return ((int(nr) >> _UNX_IOC_DIRSHIFT) & _UNX_IOC_DIRMASK);
}
func _UNX_IOC_TYPE(nr) {
  return ((int(nr) >> _UNX_IOC_TYPESHIFT) & _UNX_IOC_TYPEMASK);
}
func _UNX_IOC_NR(nr) {
  return ((int(nr) >> _UNX_IOC_NRSHIFT) & _UNX_IOC_NRMASK);
}
func _UNX_IOC_SIZE(nr) {
  return ((int(nr) >> _UNX_IOC_SIZESHIFT) & _UNX_IOC_SIZEMASK);
}

/* ...and for the drivers/sound files... */
UNX_IOC_IN        = (_UNX_IOC_WRITE << _UNX_IOC_DIRSHIFT);
UNX_IOC_OUT       = (_UNX_IOC_READ << _UNX_IOC_DIRSHIFT);
UNX_IOC_INOUT     = ((_UNX_IOC_WRITE|_UNX_IOC_READ) << _UNX_IOC_DIRSHIFT);
UNX_IOCSIZE_MASK  = (_UNX_IOC_SIZEMASK << _UNX_IOC_SIZESHIFT);
UNX_IOCSIZE_SHIFT = (_UNX_IOC_SIZESHIFT);

/*---------------------------------------------------------------------------*/
/* MISCELLANEOUS */

extern unx_init;
/* DOCUMENT unx_init;
     Initialize internals of this plug-in and define constants.  This
     subroutine can be safely called again after initialization; for instance,
     to redefine constants.

   SEE ALSO: unx_open, unx_errno.
 */

/* Perform the initialization. */
unx_init;
