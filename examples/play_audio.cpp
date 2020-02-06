#include "mediahandling/mediahandling.h"
#include <iostream>
#include <ao/ao.h>
#include <cassert>

namespace mh = media_handling;

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Feed me a file" << std::endl;
        return -1;
    }

    auto source = mh::createSource(argv[1]);
    auto stream = source->audioStream(0);
    bool okay = stream->setOutputFormat(mh::SampleFormat::UNSIGNED_8);
    assert(okay);
    const auto rate = stream->property<mh::SampleRate>(mh::MediaProperty::AUDIO_SAMPLING_RATE, okay);
    assert(okay);
    const auto channels = stream->property<int32_t>(mh::MediaProperty::AUDIO_CHANNELS, okay);
    assert(okay);
    
    ao_initialize();
    const int driver = ao_default_driver_id();
    ao_info* info = ao_driver_info(driver);
    ao_sample_format sample_format;
    sample_format.bits = 8;
    sample_format.channels = channels;
    sample_format.rate = rate;
    sample_format.byte_format = AO_FMT_NATIVE;
    sample_format.matrix = 0;

    // This may return null if device is already in use and may require to run as root
    ao_device* device = ao_open_live(driver, &sample_format, nullptr);
    assert(device);

    for (;;) {
        auto frame = stream->frame();
        if (!frame) {
        break;
        }
        auto data = frame->data();
        ao_play(device, reinterpret_cast<char*>(*data.data_), data.data_size_);
    }
}