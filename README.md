# MediaHandling [![Codacy Badge](https://api.codacy.com/project/badge/Grade/fd1f6eda59fd4bdbbe54c90bac7300d5)](https://www.codacy.com/manual/jonno85uk/mediahandling?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=jonno85uk/mediahandling&amp;utm_campaign=Badge_Grade)[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

This is a library primarily intended for the use in the project [Chestnut](https://github.com/jonno85uk/chestnut)
by handling all media-file operations (properties/decoding/encoding).
This is done by abstracting the underlying media library with a common interface.

Currently, only the [FFmpeg](https://ffmpeg.org/) library is being used.

# Usage

## Properties

### Get file properties
    auto source =  media_handling::createSource("somefile.mov");
    bool okay;
    auto format       = source->property<std::string>(MediaProperty::FILE_FORMAT, okay);
    auto duration     = source->property<int64_t>(MediaProperty::DURATION, okay);
    auto stream_count = source->property<int32_t>(MediaProperty::STREAMS, okay);
    auto bitrate      = source->property<int32_t>(MediaProperty::BITRATE, okay);
    auto all_props    = source->getProperties();

### Get stream properties
#### Video
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->visualStream(0);
    bool okay;
    auto codec          = stream->property<Codec>(MediaProperty::CODEC, okay);
    auto codec_profile  = stream->property<Profile>(MediaProperty::PROFILE, okay);
    auto dims           = stream->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
    auto frame_rate     = stream->property<Rational>(MediaProperty::FRAME_RATE, okay);
    auto bitrate        = stream->property<int32_t>(MediaProperty::BITRATE, okay);
    auto field_order    = stream->property<FieldOrder>(MediaProperty::FIELD_ORDER, okay);
    auto frame_count    = stream->property<int64_t>(MediaProperty::FRAME_COUNT, okay);
    auto pixel_format   = stream->property<PixelFormat>(MediaProperty::PIXEL_FORMAT, okay);
    auto par            = stream->property<Rational>(MediaProperty::PIXEL_ASPECT_RATIO, okay);
    auto dar            = stream->property<Rational>(MediaProperty::DISPLAY_ASPECT_RATIO, okay);
    auto colour_space   = stream->property<ColourSpace>(MediaProperty::COLOUR_SPACE, okay);
### Audio
    auto source =  media_handling::createSource("somefile.mov");
    auto stream = source->audioStream(0);
    bool okay;
    auto channel_layout = stream->property<SampleFormat>(MediaProperty::AUDIO_LAYOUT, okay);
    auto channel_count  = stream->property<int32_t>(MediaProperty::AUDIO_CHANNELS, okay);
    auto sample_rate    = stream->property<int32_t>(MediaProperty::AUDIO_SAMPLING_RATE, okay);
    auto sample_fmt     = stream->property<SampleFormat>(MediaProperty::AUDIO_SAMPLING_RATE, okay);


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

# Installation

In Arch or Manjaro Linux, a package is available via AUR. To install:
  
<pre><code>pacaur -S mediahandling</code></pre>

## Dependencies

[fmt](https://github.com/fmtlib/fmt) and [FFmpeg](https://ffmpeg.org/) development headers and libraries

## Building

Change directory to the top-level of the project.

<pre><code>mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/ -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install</code></pre>

## Regression Tests

Install [git-lfs](https://git-lfs.github.com/) and [google-test](https://github.com/google/googletest)
<pre><code>git-lfs fetch
git-lfs pull
</pre></code>

Change directory to RegressionTests
<pre><code>mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/ -DCMAKE_BUILD_TYPE=Release ..
make</code></pre>

Change back to RegressionTests directory and run

<pre>build/mh_regression</pre>
