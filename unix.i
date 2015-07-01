/*
 * unix.i --
 *
 * Yorick plug-in to low-level Unix functions.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2015: Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>
 *
 * See LICENSE.md for details.
 *
 */

if (is_func(plug_in)) plug_in, "yor_unix";

extern unx_errno;
extern unx_strerror;
/* DOCUMENT err = unx_errno();
         or msg = unx_strerror();
         or msg = unx_strerror(err);

      The function unx_errno() returns the last error code of the C library.

      The function unx_errno() returns the explantory string for the last
      error code of the C library of for the error code ERR if specified.

      Global valriables with the name of the standard error codes (prefixed by
      UNX_) are predefined, for instance UNX_EINVAL is set with the value of
      the EINVAL error code.

   SEE ALSO:
 */

extern unx_native;
/* DOCUMENT str = unx_native(path);
     Get the native file name corresponding to PATH.  Leading tilde character
     (e.g., ~ or ~user) and shell variables (e.g., $HOME) are interpolated.

   SEE ALSO: unx_open.
 */

extern unx_open;
/* DOCUMENT fd = unx_open(path, flags);
         or fd = unx_open(path, flags, mode);

      Open a file descriptor to access the contents of file PATH.

      It is not necessary to explicitly close the file descriptor (via
      unx_close), this is automatically done when it is no longer referenced
      in YoricK.

      The returned object has the following members:

         fd.path    the path to the file
         fd.number  the file descriptor number
         fd.flags   the value of FLAGS
         fd.mode    the value of MODE

      Until the file descriptor is closed, the value of these members
      correspond to the ones used to open the file descriptor.


   SEE ALSO: unx_read, unx_write, unx_native, unx_lseek, unx_close.
 */

extern unx_close;
/* DOCUMENT unx_close, fd;
     Explicitly close file descriptor FD.

   SEE ALSO: unx_open.
 */

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
     bytes to write.  By default, OFF = 0 (i.e., writen bytes start at the
     beginning of ARR) and CNT = sizeof(ARR) - OFF (i.e., try to write all
     bytes remaining in ARR after offset OFF).  The returned value N is the
     number of bytes written, or -1 in case of error.

     When called as a subroutine, an error is thrown if the number of written
     bytes is not equal to the request value (CNT).

   SEE ALSO: unx_open, unx_write, unx_errno.
 */

extern unx_ioctl;
/* DOCUMENT rv = unx_init(fd, req, ...);
     Perform an IOCTL call on file descriptor FD.

   SEE ALSO: unx_open, unx_errno.
 */


extern unx_init;
/* DOCUMENT unx_init;
     Initialize internals of this plug-in.

   SEE ALSO: unx_open, unx_errno.
 */

/* Perform the initialization. */
unx_init;

/*
 * Local Variables:
 * mode: Yorick
 * tab-width: 8
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * fill-column: 78
 * coding: utf-8
 * End:
 */
