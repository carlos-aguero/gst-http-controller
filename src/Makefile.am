# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgsthttpcontrol.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgsthttpcontrol_la_SOURCES = gsthttpcontrol.c gsthttpcontrol.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgsthttpcontrol_la_CFLAGS = $(GST_CFLAGS)
libgsthttpcontrol_la_LIBADD = $(GST_LIBS)
libgsthttpcontrol_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgsthttpcontrol_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gsthttpcontrol.h
