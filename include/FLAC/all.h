/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002,2003,2004,2005  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLAC__ALL_H
#define FLAC__ALL_H

#include "export.h"

#include "assert.h"
#include "callback.h"
#include "file_decoder.h"
#include "file_encoder.h"
#include "format.h"
#include "metadata.h"
#include "ordinals.h"
#include "seekable_stream_decoder.h"
#include "seekable_stream_encoder.h"
#include "stream_decoder.h"
#include "stream_encoder.h"

/** \mainpage
 *
 * \section intro Introduction
 *
 * This is the documentation for the FLAC C and C++ APIs.  It is
 * highly interconnected; this introduction should give you a top
 * level idea of the structure and how to find the information you
 * need.  As a prerequisite you should have at least a basic
 * knowledge of the FLAC format, documented
 * <A HREF="../format.html">here</A>.
 *
 * \section c_api FLAC C API
 *
 * The FLAC C API is the interface to libFLAC, a set of structures
 * describing the components of FLAC streams, and functions for
 * encoding and decoding streams, as well as manipulating FLAC
 * metadata in files.  The public include files will be installed
 * in your include area as <include>/FLAC/...
 *
 * By writing a little code and linking against libFLAC, it is
 * relatively easy to add FLAC support to another program.  The
 * library is licensed under <A HREF="../license.html">Xiph's BSD license</A>.
 * Complete source code of libFLAC as well as the command-line
 * encoder and plugins is available and is a useful source of
 * examples.
 *
 * Aside from encoders and decoders, libFLAC provides a powerful
 * metadata interface for manipulating metadata in FLAC files.  It
 * allows the user to add, delete, and modify FLAC metadata blocks
 * and it can automatically take advantage of PADDING blocks to avoid
 * rewriting the entire FLAC file when changing the size of the
 * metadata.
 *
 * libFLAC usually only requires the standard C library and C math
 * library. In particular, threading is not used so there is no
 * dependency on a thread library. However, libFLAC does not use
 * global variables and should be thread-safe.
 *
 * There is also a new libOggFLAC library which wraps around libFLAC
 * to provide routines for encoding to and decoding from FLAC streams
 * inside an Ogg container.  The interfaces are very similar or identical
 * to their counterparts in libFLAC.  libOggFLAC is also licensed under
 * <A HREF="../license.html">Xiph's BSD license</A>.
 *
 * \section cpp_api FLAC C++ API
 *
 * The FLAC C++ API is a set of classes that encapsulate the
 * structures and functions in libFLAC.  They provide slightly more
 * functionality with respect to metadata but are otherwise
 * equivalent.  For the most part, they share the same usage as
 * their counterparts in libFLAC, and the FLAC C API documentation
 * can be used as a supplement.  The public include files
 * for the C++ API will be installed in your include area as
 * <include>/FLAC++/...
 *
 * There is also a new libOggFLAC++ library, which provides classes
 * for encoding to and decoding from FLAC streams in an Ogg container.
 * The classes are very similar to their counterparts in libFLAC++.
 *
 * Both libFLAC++ libOggFLAC++ are also licensed under
 * <A HREF="../license.html">Xiph's BSD license</A>.
 *
 * \section getting_started Getting Started
 *
 * A good starting point for learning the API is to browse through
 * the <A HREF="modules.html">modules</A>.  Modules are logical
 * groupings of related functions or classes, which correspond roughly
 * to header files or sections of header files.  Each module includes a
 * detailed description of the general usage of its functions or
 * classes.
 *
 * From there you can go on to look at the documentation of
 * individual functions.  You can see different views of the individual
 * functions through the links in top bar across this page.
 *
 * \section embedded_developers Embedded Developers
 *
 * libFLAC has grown larger over time as more functionality has been
 * included, but much of it may be unnecessary for a particular embedded
 * implementation.  Unused parts may be pruned by some simple editing of
 * src/libFLAC/Makefile.am.  In general, the decoders, encoders, and
 * metadata interface are all independent from each other.
 *
 * It is easiest to just describe the dependencies:
 *
 * - All modules depend on the \link flac_format Format \endlink module.
 * - The decoders and encoders are independent of each other.
 * - The metadata interface requires the file decoder.
 * - The decoder and encoder layers depend on the layers below them, but
 *   not above them; e.g. the seekable stream decoder depends on the stream
 *   decoder but not the file decoder
 *
 * For example, if your application only requires the stream decoder, no
 * encoders, and no metadata interface, you can remove the seekable stream
 * decoder, file decoder, all encoders, and the metadata interface, which
 * will greatly reduce the size of the library.
 */

/** \defgroup flac FLAC C API
 *
 * The FLAC C API is the interface to libFLAC, a set of structures
 * describing the components of FLAC streams, and functions for
 * encoding and decoding streams, as well as manipulating FLAC
 * metadata in files.
 *
 * You should start with the format components as all other modules
 * are dependent on it.
 */

#endif
