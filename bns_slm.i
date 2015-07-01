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
 * See LICENSE.md for details.
 *
 */

require, "unix.i";

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
{
  extern _BNS_SLM_FD;
  if (is_void(dev)) dev = _BNS_SLM_DEV;
  _BNS_SLM_FD = unx_open(dev, UNX_O_RDWR);

  local int32_t;
  if (sizeof(int) == 4) int32_t = int;
  else if (sizeof(long) == 4) int32_t = long;
  else error, "no integer type found for 32-bit integer";

  // Set crystal type 0 = FLC, 1 = Nematic
  buf = [int32_t(0)];
  unx_ioctl(_BNS_SLM_FD, BNS_SLM_SET_LC_TYPE, buf);

  // Set frame rate
  buf = [int32_t(0x00040008)];
  unx_ioctl(_BNS_SLM_FD, BNS_SLM_SET_FRAME_RATE, buf);

  // Start with an empty image
  z = array(char, 512, 512);
  bns_slm_send_image(lut, z);
}

func bns_slm_send_image(lut, img)
{
  extern _BNS_SLM_FD;
  if (is_void(_BNS_SLM_FD) || _BNS_SLM_FD.number < 0) {
    bns_slm_open;
  }

  data = lut(image);
  buf = array.array('B', image.tostring()); //FIXME:
  unx_ioctl(_BNS_SLM_FD, BNS_SLM_WRITE_IMAGE, buf);
}

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
