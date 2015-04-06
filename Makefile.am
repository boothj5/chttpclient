library_sources = \
	src/httpclient/httperr.c \
	src/httpclient/httpclient.h \
	src/httpclient/httpcontext.c src/httpclient/httpcontext.h \
	src/httpclient/httprequest.c src/httpclient/httprequest.h \
	src/httpclient/httpresponse.c src/httpclient/httpresponse.h \
	src/httpclient/httpnet.c src/httpclient/httpnet.h \
	src/httpclient/httputil.c

bin_PROGRAMS = simple cmdline download
simple_SOURCES = src/simple.c $(library_sources)
cmdline_SOURCES = src/cmdline.c $(library_sources)
download_SOURCES = src/download.c $(library_sources)
