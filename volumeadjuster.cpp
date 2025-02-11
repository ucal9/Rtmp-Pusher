#include "volumeadjuster.h"

VolumeAdjuster::VolumeAdjuster()
{

}


#include <stdio.h>


#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#ifdef __cplusplus
};
#endif

#define AUDIO_IN_FILENAME "input_audio_file.mp3"
#define AUDIO_OUT_FILENAME "output_audio_file.mp3"

inline char* av_err2str2(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}



int main2(void) {
    AVFormatContext *in_fmt_ctx = NULL, *out_fmt_ctx = NULL;
    AVCodecContext *dec_ctx = NULL, *enc_ctx = NULL;
    AVFilterGraph *filter_graph = NULL;
    AVFilterContext *in_filter_ctx = NULL, *out_filter_ctx = NULL;
    const char *filter_desc = "volume=2.0";
    int ret;



    // Open input file and read header
    if ((ret = avformat_open_input(&in_fmt_ctx, AUDIO_IN_FILENAME, NULL, NULL)) < 0) {
        fprintf(stderr, "Could not open input file: %s\n", av_err2str2(ret));
        return ret;
    }
    if ((ret = avformat_find_stream_info(in_fmt_ctx, NULL)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information: %s\n", av_err2str2(ret));
        return ret;
    }

    // Find audio stream information
    int audio_stream_idx = -1;
    for (unsigned int i=0; i<in_fmt_ctx->nb_streams; i++) {
        if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    if (audio_stream_idx == -1) {
        fprintf(stderr, "Could not find an audio stream in the input file\n");
        return AVERROR_INVALIDDATA;
    }

    // Get a pointer to the codec context for the audio stream
    dec_ctx = avcodec_alloc_context3(NULL);
    if (!dec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        return AVERROR(ENOMEM);
    }
    ret = avcodec_parameters_to_context(dec_ctx, in_fmt_ctx->streams[audio_stream_idx]->codecpar);
    if (ret < 0) {
        fprintf(stderr, "Failed to copy codec parameters to decoder context: %s\n", av_err2str2(ret));
        return ret;
    }

    // Find a decoder for the audio stream
    AVCodec *decoder = avcodec_find_decoder(dec_ctx->codec_id);
    if (!decoder) {
        fprintf(stderr, "Failed to find decoder for the input audio codec\n");
        return AVERROR_DECODER_NOT_FOUND;
    }

    // Open the decoder
    if ((ret = avcodec_open2(dec_ctx, decoder, NULL)) < 0) {
        fprintf(stderr, "Failed to open decoder for input audio stream: %s\n", av_err2str2(ret));
        return ret;
    }

    // Allocate memory for the output format context and create new output file
    if ((ret = avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, AUDIO_OUT_FILENAME)) < 0) {
        fprintf(stderr, "Could not create output context: %s\n", av_err2str2(ret));
        return ret;
    }

    // Create a new audio stream in the output file
    AVStream *audio_out_stream = avformat_new_stream(out_fmt_ctx, NULL);
    if (!audio_out_stream) {
        fprintf(stderr, "Failed allocating output audio stream\n");
        return AVERROR_UNKNOWN;
     }

     // Copy codec parameters from input stream to output stream
     ret = avcodec_parameters_copy(audio_out_stream->codecpar, in_fmt_ctx->streams[audio_stream_idx]->codecpar);
     if (ret < 0) {
        fprintf(stderr, "Failed to copy codec parameters: %s\n", av_err2str2(ret));
        return ret;
     }

    // Get a pointer to the codec context for the output audio stream
    enc_ctx = avcodec_alloc_context3(NULL);
    if (!enc_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        return AVERROR(ENOMEM);
    }
    ret = avcodec_parameters_to_context(enc_ctx, audio_out_stream->codecpar);
    if (ret < 0) {
        fprintf(stderr, "Failed to copy codec parameters to encoder context: %s\n", av_err2str2(ret));
        return ret;
    }

    // Find an encoder for the output audio stream
    AVCodec *encoder = avcodec_find_encoder(enc_ctx->codec_id);
    if (!encoder) {
        fprintf(stderr, "Failed to find encoder for output audio codec\n");
        return AVERROR_ENCODER_NOT_FOUND;
    }

    // Open the encoder
    if ((ret = avcodec_open2(enc_ctx, encoder, NULL)) < 0) {
        fprintf(stderr, "Failed to open encoder for output audio stream: %s\n", av_err2str2(ret));
        return ret;
    }

    // Create and configure filter graph
    filter_graph = avfilter_graph_alloc();

    // Create source filter and set its options
    const AVFilter *source_filter = avfilter_get_by_name("abuffer");
    char args[512];
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%",
             dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
             av_get_sample_fmt_name(dec_ctx->sample_fmt), dec_ctx->channel_layout);
    if (avfilter_graph_create_filter(&in_filter_ctx, source_filter, "in", args, NULL, filter_graph) < 0) {
        fprintf(stderr, "Cannot create audio buffer source filter\n");
        return -1;
    }

    // Create volume filter and set its options
    const AVFilter *volume_filter = avfilter_get_by_name("volume");
    if (!volume_filter) {
        fprintf(stderr, "Could not find the volume filter\n");
        return AVERROR_FILTER_NOT_FOUND;
    }
    if ((ret = avfilter_graph_create_filter(&out_filter_ctx, volume_filter, "out", filter_desc, NULL, filter_graph)) < 0) {
        fprintf(stderr,"Cannot create volume filter\n");
        return ret;
    }

    // Link input and output filters
    if ((ret = avfilter_link(in_filter_ctx, 0, out_filter_ctx, 0)) != 0) {
        fprintf(stderr,"Failed to link filters: %s\n", av_err2str2(ret));
        return ret;
    }

    // Configure the graph
    if ((ret = avfilter_graph_config(filter_graph,NULL)) < 0) {
        fprintf(stderr,"Error configuring the graph: %s\n",av_err2str2(ret));
        return ret;
    }

    // Write output file header
    if (avformat_write_header(out_fmt_ctx,NULL) < 0) {
        fprintf(stderr,"Error writing header.\n");
        return -1;
    }

    // Allocate memory for frames
    AVFrame *frame_in = av_frame_alloc();
    if (!frame_in){
        fprintf(stderr,"Could not allocate input audio frame\n");
        return AVERROR(ENOMEM);
    }
    AVFrame *frame_out = av_frame_alloc();
    if (!frame_out){
        fprintf(stderr,"Could not allocate output audio frame\n");
        return AVERROR(ENOMEM);
    }

    // Read frames from input file, filter them and write to output file
    while (1) {
        // Read an input frame
        AVPacket pkt;
        ret = av_read_frame(in_fmt_ctx, &pkt);
        if (ret < 0)
            break;

        if (pkt.stream_index == audio_stream_idx) {
            // Send the packet to the decoder
            int decr_ret = avcodec_send_packet(dec_ctx, &pkt);
            if (decr_ret < 0) {
                fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str2(decr_ret));
                return decr_ret;
            }

            while (decr_ret >= 0) {
                // Receive decoded frames from decoder
                decr_ret = avcodec_receive_frame(dec_ctx, frame_in);
                if (decr_ret == AVERROR(EAGAIN) || decr_ret == AVERROR_EOF)
                    break;
                else if (decr_ret < 0) {
                    fprintf(stderr, "Error during decoding (%s)\n", av_err2str2(decr_ret));
                    return decr_ret;
                }

                // Set properties of the output frame based on the input frame
                frame_out->format = enc_ctx->sample_fmt;
                frame_out->channel_layout = enc_ctx->channel_layout;
                frame_out->sample_rate = enc_ctx->sample_rate;

                // Allocate memory for the output buffer
                ret = av_samples_alloc(frame_out->extended_data,
                                       NULL,
                                       enc_ctx->channels,
                                       frame_in->nb_samples,
                                       enc_ctx->sample_fmt,
                                       0);
                if (ret < 0) {
                    fprintf(stderr, "Could not allocate output audio samples: %s\n", av_err2str2(ret));
                    return ret;
                }

                // Process the input frame through the filter graph
                if ((ret = av_buffersrc_add_frame_flags(in_filter_ctx, frame_in, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
                    fprintf(stderr,"Error submitting a frame to the filter graph (%s)\n",av_err2str2(ret));
                    return ret;
                }
                while (1) {
                    ret = av_buffersink_get_frame (out_filter_ctx, frame_out);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        fprintf(stderr,"Error during filtering (%s)\n",av_err2str2(ret));
                        return ret;
                    }

                    // Encode and write the filtered output frame
                    AVPacket pkt_out = { 0 };
                    int encr_ret = avcodec_send_frame(enc_ctx, frame_out);
                    if (encr_ret < 0) {
                        fprintf(stderr, "Error encoding audio frame (%s)\n", av_err2str2(encr_ret));
                        return encr_ret;
                    }

                    while (encr_ret >= 0) {
                        encr_ret = avcodec_receive_packet(enc_ctx, &pkt_out);
                        if (encr_ret == AVERROR(EAGAIN) || encr_ret == AVERROR_EOF)
                            break;
                        else if (encr_ret < 0) {
                            fprintf(stderr, "Error during encoding (%s)\n", av_err2str2(encr_ret));
                            return encr_ret;
                        }

                        pkt_out.stream_index = audio_out_stream->index;
                        av_packet_rescale_ts(&pkt_out, enc_ctx->time_base, audio_out_stream->time_base);
                        ret = av_interleaved_write_frame(out_fmt_ctx, &pkt_out);

                        if (ret < 0) {
                            fprintf(stderr, "Error while writing output packet (%s)\n", av_err2str2(ret));
                            return ret;
                        }
                    }

                    // Free memory for the filtered frame
                    av_frame_free(&frame_out);
                }
            }

            // Free memory for input packet
            av_packet_unref(&pkt);
        } else {
            // Free memory for input packet that is not from the audio stream
            av_packet_unref(&pkt);
        }
    }

    // Flush filter graphs and write output file trailer
    if (avfilter_graph_request_oldest(filter_graph) < 0) {
        fprintf(stderr,"Error flushing filter graph.\n");
        return -1;
    }
    if (av_write_trailer(out_fmt_ctx) < 0) {
        fprintf(stderr,"Error writing trailer.\n");
        return -1;
    }

    // Close all open files and free all allocated memory
    if (in_filter_ctx)
        avfilter_free(in_filter_ctx);
    if (out_filter_ctx)
        avfilter_free(out_filter_ctx);
    if (filter_graph)
        avfilter_graph_free(&filter_graph);

    if (dec_ctx)
        avcodec_free_context(&dec_ctx);
    if (enc_ctx)
        avcodec_free_context(&enc_ctx);

    if (in_fmt_ctx)
        avformat_close_input(&in_fmt_ctx);
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&out_fmt_ctx->pb);
    if (out_fmt_ctx)
        avformat_free_context(out_fmt_ctx);

    return 0;
}
