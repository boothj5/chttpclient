library_sources = \
	src/httpclient/httperr.c \
	src/httpclient/httpclient.h \
	src/httpclient/net/httpnet.c src/httpclient/net/httpnet.h \
	src/httpclient/net/httpbodystreams.c src/httpclient/net/httpbodystreams.h \
	src/httpclient/httpcontext.c src/httpclient/httpcontext.h \
	src/httpclient/httprequest.c src/httpclient/httprequest.h \
	src/httpclient/httpresponse.c src/httpclient/httpresponse.h \
	src/httpclient/httputil.c src/httpclient/httputil.h

bin_PROGRAMS = simple cmdline download tester
simple_SOURCES = src/simple.c $(library_sources)
cmdline_SOURCES = src/cmdline.c $(library_sources)
download_SOURCES = src/download.c $(library_sources)
tester_SOURCES = src/tester.c $(library_sources)
