#include <iostream>
#include "mediahandling.h"
#include "ffmpeg/ffmpegsource.h"
#include <iostream>

using namespace media_handling;
using namespace media_handling::ffmpeg;

constexpr auto filename = "../RegressionTests/ReferenceMedia/Video/h264/h264_yuv420p_avc1_fhd.mp4";


static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                      char *filename)
{
     FILE *f;
     int i;

     f=fopen(filename,"w");
     fprintf(f,"P5\n%d %d\n%d\n",xsize,ysize,255);
     for(i=0;i<ysize;i++)
         fwrite(buf + i * wrap,1,xsize,f);
     fclose(f);
}

int main()
{
    IMediaSource* src = new FFMpegSource(std::string(filename));
    bool is_valid;
    auto dur = src->property<int64_t>(MediaProperty::DURATION, is_valid);
    auto fmt = src->property<std::string>(MediaProperty::FILE_FORMAT, is_valid);
    auto strm = src->property<uint32_t>(MediaProperty::STREAMS, is_valid);
    auto v_streams = src->property<int32_t>(MediaProperty::VIDEO_STREAMS, is_valid);
    auto a_streams = src->property<int32_t>(MediaProperty::AUDIO_STREAMS, is_valid);

    auto v_s = src->visualStream(0);
    auto timescale = v_s->property<double>(MediaProperty::TIMESCALE, is_valid);
    auto format = v_s->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, is_valid);
    auto dims = v_s->property<Dimensions>(MediaProperty::DIMENSIONS, is_valid);
    auto frame = v_s->frame(100);
    auto codec = v_s->property<std::string>(MediaProperty::CODEC, is_valid);
    auto frames = dur*timescale;

    pgm_save(frame->data_[0], frame->line_size_,
    frame->line_size_, frame->line_count_, "output.ppm");

    std::cout << "duration:" << dur << "\nformat:" << fmt << "\nstreams:"<< strm << "\nvideo_streams:" << v_streams
              << "\naudio_streams:" << a_streams << "\ntimescale:" << timescale << "\ncodec:" << codec << "\nframes:" << frames << std::endl;

    return 0;
}
