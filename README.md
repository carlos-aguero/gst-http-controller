# gst-http-controller

Autogenerate:
./autogen.sh

Compile: 
make

Run:
GST_DEBUG=httpcontrol:5 GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 videotestsrc ! httpcontrol ! fakesink  silent=true -v
