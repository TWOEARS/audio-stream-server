#  Copyright (c) 2014, LAAS/CNRS
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, 
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice, 
#    this list of conditions and the following disclaimer in the documentation 
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its contributors 
#    may be used to endorse or promote products derived from this software 
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
#
lib_LTLIBRARIES = libcapture_codels.la

libcapture_codels_la_SOURCES=	capture_c_types.h
libcapture_codels_la_SOURCES+=	capture_codels.c
libcapture_codels_la_SOURCES+=	capture_retrieve_data_codels.c
libcapture_codels_la_SOURCES+=	AudioCapture.h
libcapture_codels_la_SOURCES+=	AudioCapture.c
libcapture_codels_la_SOURCES+=	Ports.h
libcapture_codels_la_SOURCES+=	Ports.c
libcapture_codels_la_SOURCES+=	capture_socket_codels.c

libcapture_codels_la_CPPFLAGS=	$(requires_CFLAGS)
libcapture_codels_la_LDFLAGS=	$(requires_LIBS)
libcapture_codels_la_LDFLAGS+=	-release $(PACKAGE_VERSION)
libcapture_codels_la_LDFLAGS+=  -lasound
libcapture_codels_la_LDFLAGS+=  -Werror


# idl  mappings
BUILT_SOURCES=	capture_c_types.h
CLEANFILES=	${BUILT_SOURCES}
capture_c_types.h: ${top_srcdir}/capture.gen
	${genom3}  mappings \
	  -MD -MF .deps/$@.d -MT $@ --signature -l c $< >$@

-include .deps/capture_c_types.h.d
