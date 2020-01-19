# Usage

## File Properties

## Get file properties
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  bool okay;
  auto format       = source->property<std::string>(MediaProperty::FILE_FORMAT, okay);
  auto duration     = source->property<int64_t>(MediaProperty::DURATION, okay);
  auto stream_count = source->property<int32_t>(MediaProperty::STREAMS, okay);
  auto bitrate      = source->property<int32_t>(MediaProperty::BITRATE, okay);
</code></pre>

### Get video dimensions
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  bool okay;
  auto dims = stream->property<Dimensions>(MediaProperty::DIMENSIONS, okay);
</code></pre>

### Get video Framerate
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  bool okay;
  auto rate = stream->property<Rational>(MediaProperty::FRAME_RATE, okay);
</code></pre>

### Get video Bitrate
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  bool okay;
  auto rate = stream->property<Rational>(MediaProperty::FRAME_RATE, okay);
</code></pre>

### Get video field order
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  bool okay;
  auto rate = stream->property<FieldOrder>(MediaProperty::FIELD_ORDER, okay);
</code></pre>

## Decoding

### Read first video frame from a file
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  auto frame = stream->frame(); 
</code></pre>


### Read video frame from a file at timestamp
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  auto frame = stream->frame(timestamp); 
</code></pre>

### Read video frames concurrently from the beggining to end
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  while (true) {
      auto frame = stream->frame(); 
      if (frame == nullptr) {
          // End of stream
          break;
      }
  }
</code></pre>

### Read first video frame from a file with specific pixel format
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  stream->setOutputFormat(PixelFormat::RGBA);
  auto frame = stream->frame(); 
</code></pre>

### Read first video frame from a file with specific pixel format and dimensions
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  stream->setOutputFormat(PixelFormat::RGBA, {320, 240});
  auto frame = stream->frame(); 
</code></pre>

### Read first video frame from a file with specific pixel format, dimensions and interpolation method
<pre><code>
  auto source =  media_handling::createSource("somefile.mov");
  auto stream = source->visualStream(0);
  stream->setOutputFormat(PixelFormat::RGBA, {320, 240}, InterpolationMethod::BICUBIC);
  auto frame = stream->frame(); 
</code></pre>

