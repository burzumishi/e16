/*
 * Copyright (C) 2000-2007 Carsten Haitzler, Geoff Harrison and various contributors
 * Copyright (C) 2004-2014 Kim Woelders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "E.h"

#if HAVE_SOUND
#include "sound.h"

#if USE_SOUND_LOADER_AUDIOFILE
#include <audiofile.h>

int
SoundSampleGetData(const char *file, SoundSampleData * ssd)
{
   AFfilehandle        in_file;
   int                 in_format, in_width, bytes_per_frame, frame_count;
   int                 frames_read;

   in_file = afOpenFile(file, "r", NULL);
   if (!in_file)
      return -1;

   frame_count = afGetFrameCount(in_file, AF_DEFAULT_TRACK);
   ssd->channels = afGetChannels(in_file, AF_DEFAULT_TRACK);
   ssd->rate = (unsigned int)(afGetRate(in_file, AF_DEFAULT_TRACK) + .5);
   afGetSampleFormat(in_file, AF_DEFAULT_TRACK, &in_format, &in_width);
   ssd->bit_per_sample = in_width;
#ifdef WORDS_BIGENDIAN
   afSetVirtualByteOrder(in_file, AF_DEFAULT_TRACK, AF_BYTEORDER_BIGENDIAN);
#else
   afSetVirtualByteOrder(in_file, AF_DEFAULT_TRACK, AF_BYTEORDER_LITTLEENDIAN);
#endif

   bytes_per_frame = (ssd->bit_per_sample * ssd->channels) / 8;
   ssd->size = frame_count * bytes_per_frame;
   ssd->data = EMALLOC(unsigned char, ssd->size);

   if (EDebug(EDBUG_TYPE_SOUND))
      Eprintf("%s: frames=%u chan=%u width=%u rate=%u\n", __func__,
	      frame_count, ssd->channels, ssd->bit_per_sample, ssd->rate);

   frames_read =
      afReadFrames(in_file, AF_DEFAULT_TRACK, ssd->data, frame_count);

   afCloseFile(in_file);

   if (frames_read <= 0)
     {
	ssd->size = 0;
	_EFREE(ssd->data);
	return -1;
     }

   return 0;
}

#endif /* USE_SOUND_LOADER_AUDIOFILE */

#if USE_SOUND_LOADER_SNDFILE
#include <sndfile.h>

int
SoundSampleGetData(const char *file, SoundSampleData * ssd)
{
   SNDFILE            *sf;
   SF_INFO             sf_info;
   int                 bytes_per_frame, frame_count, frames_read;

   memset(&sf_info, 0, sizeof(sf_info));
   sf = sf_open(file, SFM_READ, &sf_info);
   if (!sf)
      return -1;

   ssd->channels = (unsigned int)sf_info.channels;
   ssd->rate = (unsigned int)sf_info.samplerate;
   ssd->bit_per_sample = 16;

   frame_count = sf_info.frames;
   bytes_per_frame = (ssd->bit_per_sample * ssd->channels) / 8;
   ssd->size = frame_count * bytes_per_frame;
   ssd->data = EMALLOC(unsigned char, ssd->size);

   if (EDebug(EDBUG_TYPE_SOUND))
      Eprintf("%s: frames=%u chan=%u width=%u rate=%u\n", __func__,
	      frame_count, ssd->channels, ssd->bit_per_sample, ssd->rate);

   frames_read = sf_readf_short(sf, (short *)ssd->data, frame_count);

   sf_close(sf);

   if (frames_read <= 0)
     {
	ssd->size = 0;
	_EFREE(ssd->data);
	return -1;
     }

   ssd->size = frames_read * bytes_per_frame;

   return 0;
}

#endif /* USE_SOUND_LOADER_SNDFILE */

#endif /* HAVE_SOUND */
