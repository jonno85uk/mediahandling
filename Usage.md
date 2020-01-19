# Usage

## File Properties

## Get file properties
    auto source =  media_handling::createSource("somefile.mov");
    bool okay;
    auto format       = source->property<std::string>(MediaProperty::FILE_FORMAT, okay);
    auto duration     = source->property<int64_t>(MediaProperty::DURATION, okay);
    auto stream_count = source->property<int32_t>(MediaProperty::STREAMS, okay);
    auto bitrate      = source->property<int32_t>(MediaProperty::BITRATE, okay);


### Get video dimensions
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    bool okay;
    auto dims = stream->property<Dimensions>(MediaProperty::DIMENSIONS, okay);


### Get video Framerate
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    bool okay;
    auto rate = stream->property<Rational>(MediaProperty::FRAME_RATE, okay);


### Get video Bitrate
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    bool okay;
    auto rate = stream->property<Rational>(MediaProperty::FRAME_RATE, okay);


### Get video field order
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    bool okay;
    auto rate = stream->property<FieldOrder>(MediaProperty::FIELD_ORDER, okay);


## Decoding

### Read first video frame from a file
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    auto frame = stream->frame(); 


### Read video frame from a file at timestamp
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    auto frame = stream->frame(timestamp); 

### Read video frames concurrently from the beginning to end
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    while (true) {
        auto frame = stream->frame(); 
        if (frame == nullptr) {
            // End of stream
            break;
        }
    }

### Read first video frame from a file with specific pixel format
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    stream->setOutputFormat(PixelFormat::RGBA);
    auto frame = stream->frame(); 

### Read first video frame from a file with specific pixel format and dimensions
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    stream->setOutputFormat(PixelFormat::RGBA, {320, 240});
    auto frame = stream->frame(); 

### Read first video frame from a file with specific pixel format, dimensions and interpolation method
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    stream->setOutputFormat(PixelFormat::RGBA, {320, 240}, InterpolationMethod::BICUBIC);
    auto frame = stream->frame(); 

## Encoding

### Write a video frame to an mp4 file with only a video stream using the h264 encoder
    MediaFrame frame;

    ...

    auto sink =  media_handling::createSink("somefile.mp4", {Codec::H264}, {});
    sink.initialise();
    auto stream = sink.visualStream(0);
    stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
    stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
    stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
    stream->setProperty(MediaProperty::BITRATE, 1'000'000);
    stream->setInputFormat(PixelFormat::YUV420);
    stream->writeFrame(frame);
    // To finish the stream
    stream->writeFrame(nullptr);

### Configure profile and preset of the h264 encoder
    auto sink =  media_handling::createSink("somefile.mp4", {Codec::H264}, {});
    sink.initialise();
    auto stream = sink.visualStream(0);
    stream->setProperty(MediaProperty::FRAME_RATE, Rational(25));
    stream->setProperty(MediaProperty::DIMENSIONS, Dimensions({320,240}));
    stream->setProperty(MediaProperty::COMPRESSION, CompressionStrategy::TARGETBITRATE);
    stream->setProperty(MediaProperty::BITRATE, 1'000'000);
    stream->setInputFormat(PixelFormat::YUV420);
    stream->setProperty(MediaProperty::PROFILE, Profile::H264_HIGH422);
    stream->setProperty(MediaProperty::PRESET, Preset::X264_FAST);