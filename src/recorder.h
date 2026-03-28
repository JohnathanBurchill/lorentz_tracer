#ifndef RECORDER_H
#define RECORDER_H

#ifdef HAS_LIBAV
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#endif

typedef struct {
#ifdef HAS_LIBAV
    AVFormatContext *fmt_ctx;
    AVCodecContext  *codec_ctx;
    AVStream        *stream;
    AVFrame         *frame;
    AVPacket        *pkt;
    struct SwsContext *sws;
#endif
    int width, height, fps;
    long long pts;
    int active;
    char filename[256];
} Recorder;

/* Returns 0 on success, -1 on failure (libav not available or error). */
int  recorder_start(Recorder *rec, int width, int height, int fps,
                    const char *filename);
void recorder_write_frame(Recorder *rec, const unsigned char *rgba,
                          int width, int height);
void recorder_stop(Recorder *rec);
int  recorder_available(void);

#endif
