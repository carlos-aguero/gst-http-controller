# GStreamer-web-plugin - webplugin-v0.4


GStreamer element that creates a filter element capable of exposing all the pipeline elements in a WebServer (), it is possible to modify the elements properties from the web page

* Something like this should work:
** GST_PLUGIN_PATH=. gst-launch-1.0 videotestsrc name=src1 ! webcontrol name=p1 ! autovideosink
** GST_PLUGIN_PATH=. gst-launch-1.0 fakesrc name=src1 ! webcontrol name=p1 ! fakesink name=sink

Some required installs are:
- sudo apt install libmicrohttpd-dev
- sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev

Open a web browser and use the address:
- localhost:8080

Or use a CLI to emit the command, modify as required:
- curl "http://127.0.0.1:8080/set?element=<ELEMENT_NAME>&property=<PROP_NAME>&value=<PROP_VAL>"

