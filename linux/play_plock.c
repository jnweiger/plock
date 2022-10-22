/*

Reference: https://en.wikipedia.org/wiki/Au_file_format

plock sound files are stored in SUN AU ulaw format.  8khz, mono.
We prepare the alsa drive to read this format.

*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <stdint.h>	// uint32_t

uint32_t read_uint32(int fd)
{
  unsigned char ch[4]; 
  int rc = read(fd, ch, 4);
  if (rc != 4) return -1;
  
  return (ch[0] << 24) + (ch[1] << 16) + (ch[2] << 8) + ch[3];
}

#define DEBUG 0		// 1: babble to stdout while reading au header, 0: silent

typedef struct au_header {
  uint32_t magic;
  uint32_t data_offset;
  uint32_t data_size;
  uint32_t encoding;  // 1= 8-bit G7.11 u-law, 2=8-bit-linear PCM, 3=16bit linear pcm, ... 27 = 8-bit G.711 A-law
  uint32_t samples_sec;
  uint32_t channels;
} au_header;

// read an au header, position the filedescriptor at the correct data_offset, return the number of bytes to read from there.
// returns -1 when the magic is wrong.
int read_au_header(int fd, au_header *au)
{
  au->magic       = read_uint32(fd);  
  au->data_offset = read_uint32(fd);  
  au->data_size   = read_uint32(fd);  
  au->encoding    = read_uint32(fd);  // 1= 8-bit G7.11 u-law, 2=8-bit-linear PCM, 3=16bit linear pcm, ... 27 = 8-bit G.711 A-law
  au->samples_sec = read_uint32(fd);
  au->channels    = read_uint32(fd);

  if (au->magic != 0x2e736e64)	// ".snd"
    {
#if DEBUG == 1
      printf("Magic .snd seen %08x (expected 0x2e736e64)\n", au->magic);
#endif
      return -1;
    }

#if DEBUG == 1
  printf("Magic .snd   %08x\n", au->magic);
  printf("Data offset  %d\n", au->data_offset);
  printf("data size    %d\n", au->data_size);
  printf("Encoding:    %d (1= 8-bit G7.11 u-law, 2=8-bit-linear PCM, 3=16bit linear pcm, ... 27 = 8-bit G.711 A-law\n", au->encoding);
  printf("Samples/sec: %d \n", au->samples_sec);
  printf("Channels:    %d \n", au->channels);
#endif

  int consumed = 6 * 4;
  while (consumed < au->data_offset)
    {
#if DEBUG == 1
      printf("read extra 4 bytes until data_offset\n");
#endif
      (void) read_uint32(fd);
      consumed += 4;
    }
  return au->data_size;
}

int main() {
  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;

  /* Open PCM device for playback. */
  rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode, no, as we only have one channel */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_NONINTERLEAVED);

  struct au_header au;
  int bytes_to_read = read_au_header(0, &au);

  /* also supports u-law, that is cool. we don't need to convert things ourselves. */
  if (au.encoding == 1)
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_MU_LAW);
  else
    {
      fprintf(stderr, "ERROR: encoding=%d not impl.\n", au.encoding);
      exit(1);
    }

  /* One channel (mono) */
  snd_pcm_hw_params_set_channels(handle, params, au.channels);
  if (au.channels != 1 && au.channels != 2)
    {
      fprintf(stderr, "ERROR: channels=%d not impl.\n", au.encoding);
      exit(1);
    }
  /* 8000 bits/second sampling rate (pretty poor quality) */
  val = au.samples_sec;
  snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

  /* Set period size to 32 frames. */
  frames = 32;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames,
                                    &dir);
  size = frames * 1 * au.channels; /* 1 byte/sample, 1 (or 2) channels */
  buffer = (char *) malloc(size);

  while (bytes_to_read > 0) {
    int frames_wanted = frames;
    int size_wanted = size;
    
    if (bytes_to_read >= size)
      bytes_to_read -= size;
    else
      {
        frames_wanted = (int)(bytes_to_read / au.channels);
        size_wanted = bytes_to_read;
        bytes_to_read = 0;
      }

#if DEBUG == 1
    printf("read(%d) remaining %d\n", size_wanted, bytes_to_read);
#endif
    rc = read(0, buffer, size_wanted);
    if (rc == 0) {
      fprintf(stderr, "end of file on input\n");
      break;
    } else if (rc != size_wanted) {
      fprintf(stderr,
              "short read: read %d bytes, (expected %d)\n", rc, size_wanted);
    }
    rc = snd_pcm_writei(handle, buffer, frames_wanted);
    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(rc));
    }  else if (rc != (int)frames_wanted) {
      fprintf(stderr,
              "short write, write %d frames (expected %d)\n", rc, frames_wanted);
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
