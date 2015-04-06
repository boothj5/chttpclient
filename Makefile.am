library_sources = \
	src/httpclient/httperr.c \
	src/httpclient/httpclient.h \
	src/httpclient/httpcontext.c src/httpclient/httpcontext.h \
	src/httpclient/httprequest.c src/httpclient/httprequest.h \
	src/httpclient/httpresponse.c src/httpclient/httpresponse.h \
	src/httpclient/httpnet.c src/httpclient/httpnet.h

bin_PROGRAMS = cmdline download
cmdline_SOURCES = src/cmdline.c $(library_sources)
download_SOURCES = src/download.c $(library_sources)
