/*
 * bns_slm.i --
 *
 * Interface to BNS SLM mirror via ioctl codes.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2015: Aurélien Jarno (Python code).
 * Copyright (C) 2015: Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>
 *
 * All rights reserved, see LICENSE.md for details.
 *
 */

require, "unix.i";

if (sizeof(int) != 4) error, "this code assume int is 32-bit integer";

/*---------------------------------------------------------------------------*/
/* IOCTL CODES */

/* The BNS_SLM_WRITE_IMAGE ioctl request takes as argument a 512x512
   array of uint8_t.  Other ioctl requests take an uint32_t. */

/* Debug mode */
BNS_SLM_DEBUG = _UNX_IO('|', 0);

/* Specify the liquid crystal type 0 = FLC (amplitude), 1 = Nematic (phase) */
BNS_SLM_SET_LC_TYPE = _UNX_IOW('|', 1, int);

/* Set image alternate rate */
BNS_SLM_SET_ALT_RATE = _UNX_IOW('|', 2, int);

/* Set frame rate */
BNS_SLM_SET_FRAME_RATE = _UNX_IOW('|', 3, int);

/* Write image frame */
BNS_SLM_WRITE_IMAGE = _UNX_IOW ('|', 4, int);

/* Select image */
BNS_SLM_SELECT_IMAGE = _UNX_IOW ('|', 5, pointer);

/* Start */
BNS_SLM_START = _UNX_IO('|', 6);

/* Stop */
BNS_SLM_STOP = _UNX_IO('|', 7);

#if 1
write, format="BNS_SLM_DEBUG          = 0x%08x\n", BNS_SLM_DEBUG;
write, format="BNS_SLM_SET_LC_TYPE    = 0x%08x\n", BNS_SLM_SET_LC_TYPE;
write, format="BNS_SLM_SET_ALT_RATE   = 0x%08x\n", BNS_SLM_SET_ALT_RATE;
write, format="BNS_SLM_SET_FRAME_RATE = 0x%08x\n", BNS_SLM_SET_FRAME_RATE;
write, format="BNS_SLM_WRITE_IMAGE    = 0x%08x\n", BNS_SLM_WRITE_IMAGE;
write, format="BNS_SLM_SELECT_IMAGE   = 0x%08x\n", BNS_SLM_SELECT_IMAGE;
write, format="BNS_SLM_START          = 0x%08x\n", BNS_SLM_START;
write, format="BNS_SLM_STOP           = 0x%08x\n", BNS_SLM_STOP;
#endif

/*---------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS */

local _BNS_SLM_FD;              // current file descriptor
_BNS_SLM_DEV = "/dev/bns_slm0"; // default device name
func bns_slm_open(dev)
/* DOCUMENT bns_slm_open;
         or bns_slm_open, dev;

     Open (or re-open) BNS SLM device.  The path to the device may be
     specified by argument DEV.  A default device is used if DEV is
     omitted.

     Currently a single BNS SLM device can be used at a time.

   SEE ALSO: bns_slm_send_image.
 */
{
  // Open the device
  extern _BNS_SLM_FD;
  if (is_void(dev)) dev = _BNS_SLM_DEV;
  _BNS_SLM_FD = unx_open(dev, UNX_O_RDWR);

  // Set crystal type (0 = FLC, 1 = Nematic)
  buf = [int(0)];
  unx_ioctl, _BNS_SLM_FD, BNS_SLM_SET_LC_TYPE, buf;

  // Set frame rate
  buf = [int(0x00040008)];
  unx_ioctl, _BNS_SLM_FD, BNS_SLM_SET_FRAME_RATE, buf;

  // Start with an empty image
  bns_slm_send_image, array(char, 512, 512);
}

func bns_slm_send_image(img)
/* DOCUMENT bns_slm_send_image, img;

     Set the shape of the BNS SLM mirror.  IMG is a 512x512 array of char.
     The BNS SLM device is automatically open (with a default device name)
     if not yet done.  You may directly call bns_slm_open to use a
     different device.

   SEE ALSO: bns_slm_open
 */
{
  extern _BNS_SLM_FD;
  if (is_void(_BNS_SLM_FD) || _BNS_SLM_FD.number < 0) {
    bns_slm_open;
  }
  if (structof(img) != char || numberof(img) != 0x40000) {
    error, "bad image type or size";
  }
  unx_ioctl, _BNS_SLM_FD, BNS_SLM_WRITE_IMAGE, img;
}
