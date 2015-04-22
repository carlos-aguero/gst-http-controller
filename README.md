# gst-http-controller

Autogenerate:
./autogen.sh

Compile:
make

Run:
GST_DEBUG=httpcontrol:5 GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 videotestsrc ! httpcontrol ! fakesink  silent=true -v

Open a web browser and use computers IP address:
http://<IP>:8080

For example http://10.251.101.11:8080/ or http://127.0.0.1:8080/

