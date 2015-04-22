# gst-http-controller

Autogenerate:
./autogen.sh

Compile: 
make

Run:
GST_DEBUG=myelement:7 GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 videotestsrc num-buffers=300 ! myelement ! fakesink  silent=true -v

