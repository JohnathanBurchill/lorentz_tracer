#include "recorder.h"
#include <string.h>

#ifdef HAS_LIBAV

int recorder_start(Recorder *rec, int width, int height, int fps,
                   const char *filename)
{
    memset(rec, 0, sizeof(*rec));
    rec->width = width;
    rec->height = height;
    rec->fps = fps;
    snprintf(rec->filename, sizeof(rec->filename), "%s", filename);

    avformat_alloc_output_context2(&rec->fmt_ctx, NULL, NULL, filename);
    if (!rec->fmt_ctx) return -1;

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) { avformat_free_context(rec->fmt_ctx); return -1; }

    rec->stream = avformat_new_stream(rec->fmt_ctx, NULL);
    rec->codec_ctx = avcodec_alloc_context3(codec);

    rec->codec_ctx->width = width;
    rec->codec_ctx->height = height;
    rec->codec_ctx->time_base = (AVRational){1, fps};
    rec->codec_ctx->framerate = (AVRational){fps, 1};
    rec->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    rec->codec_ctx->gop_size = 30;

    av_opt_set(rec->codec_ctx->priv_data, "crf", "18", 0);
    av_opt_set(rec->codec_ctx->priv_data, "preset", "medium", 0);

    if (rec->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        rec->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(rec->codec_ctx, codec, NULL) < 0) goto fail;

    avcodec_parameters_from_context(rec->stream->codecpar, rec->codec_ctx);
    rec->stream->time_base = rec->codec_ctx->time_base;

    if (!(rec->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&rec->fmt_ctx->pb, filename, AVIO_FLAG_WRITE) < 0)
            goto fail;
    }

    if (avformat_write_header(rec->fmt_ctx, NULL) < 0) goto fail;

    rec->frame = av_frame_alloc();
    rec->frame->format = AV_PIX_FMT_YUV420P;
    rec->frame->width = width;
    rec->frame->height = height;
    av_frame_get_buffer(rec->frame, 0);

    rec->pkt = av_packet_alloc();

    rec->sws = sws_getContext(width, height, AV_PIX_FMT_RGBA,
                              width, height, AV_PIX_FMT_YUV420P,
                              SWS_BILINEAR, NULL, NULL, NULL);
    if (!rec->sws) goto fail;

    rec->pts = 0;
    rec->active = 1;
    return 0;

fail:
    avcodec_free_context(&rec->codec_ctx);
    avformat_free_context(rec->fmt_ctx);
    memset(rec, 0, sizeof(*rec));
    return -1;
}

void recorder_write_frame(Recorder *rec, const unsigned char *rgba,
                          int width, int height)
{
    if (!rec->active) return;

    const uint8_t *src[1] = {rgba};
    int src_stride[1] = {width * 4};

    av_frame_make_writable(rec->frame);
    sws_scale(rec->sws, src, src_stride, 0, height,
              rec->frame->data, rec->frame->linesize);

    rec->frame->pts = rec->pts++;

    avcodec_send_frame(rec->codec_ctx, rec->frame);
    while (avcodec_receive_packet(rec->codec_ctx, rec->pkt) == 0) {
        av_packet_rescale_ts(rec->pkt, rec->codec_ctx->time_base,
                             rec->stream->time_base);
        rec->pkt->stream_index = rec->stream->index;
        av_interleaved_write_frame(rec->fmt_ctx, rec->pkt);
    }
}

void recorder_stop(Recorder *rec)
{
    if (!rec->active) return;

    /* Flush encoder */
    avcodec_send_frame(rec->codec_ctx, NULL);
    while (avcodec_receive_packet(rec->codec_ctx, rec->pkt) == 0) {
        av_packet_rescale_ts(rec->pkt, rec->codec_ctx->time_base,
                             rec->stream->time_base);
        rec->pkt->stream_index = rec->stream->index;
        av_interleaved_write_frame(rec->fmt_ctx, rec->pkt);
    }

    av_write_trailer(rec->fmt_ctx);

    sws_freeContext(rec->sws);
    av_frame_free(&rec->frame);
    av_packet_free(&rec->pkt);
    avcodec_free_context(&rec->codec_ctx);
    if (!(rec->fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&rec->fmt_ctx->pb);
    avformat_free_context(rec->fmt_ctx);

    rec->active = 0;
}

int recorder_available(void) { return 1; }

#else /* no libav */

int recorder_start(Recorder *rec, int width, int height, int fps,
                   const char *filename)
{
    (void)rec; (void)width; (void)height; (void)fps; (void)filename;
    return -1;
}

void recorder_write_frame(Recorder *rec, const unsigned char *rgba,
                          int width, int height)
{
    (void)rec; (void)rgba; (void)width; (void)height;
}

void recorder_stop(Recorder *rec) { (void)rec; }
int recorder_available(void) { return 0; }

#endif
