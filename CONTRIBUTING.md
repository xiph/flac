Thanks for considering contributing to the FLAC project!

Contributing to FLAC is possible in many ways. Among them are

- Reporting bugs or other issues at https://github.com/xiph/flac/issues
- Submitting patches at https://github.com/xiph/flac/pulls
- Testing FLAC playing devices and software at
  https://wiki.hydrogenaud.io/index.php?title=FLAC_decoder_testbench

General communication not specific to issues is generally done through
the FLAC mailing lists:

- For user questions and discussions:
  https://lists.xiph.org/mailman/listinfo/flac 
- For developer questions and discussions:
  https://lists.xiph.org/mailman/listinfo/flac-dev

## Goals

Since FLAC is an open-source project, it's important to have a set of
goals that everyone works to. They may change slightly from time to time
but they're a good guideline. Changes should be in line with the goals
and should not attempt to embrace any of the anti-goals.

**Goals**

- FLAC should be and stay an open format with an open-source reference
  implementation.
- FLAC should be lossless. This seems obvious but lossy compression
  seems to creep into every audio codec. This goal also means that flac
  should stay archival quality and be truly lossless for all input.
  Testing of releases should be thorough.
- FLAC should yield respectable compression, on par or better than other
  lossless codecs.
- FLAC should allow at least realtime decoding on even modest hardware.
- FLAC should support fast sample-accurate seeking.
- FLAC should allow gapless playback of consecutive streams. This follows from the lossless goal.
- The FLAC project owes a lot to the many people who have advanced the
  audio compression field so freely, and aims also to contribute through
  the open-source development of new ideas.

**Anti-goals**

- Lossy compression. There are already many suitable lossy formats (Ogg
  Vorbis, MP3, etc.).
- Copy prevention, DRM, etc. There is no intention to add any copy
  prevention methods. Of course, we can't stop someone from encrypting a
  FLAC stream in another container (e.g. the way Apple encrypts AAC in
  MP4 with FairPlay), that is the choice of the user.


## Contributing patches

Contributions to FLAC should be licensed with the same license as the
part of the FLAC project the contribution belongs to. These are

- libFLAC and libFLAC++ are licensed under Xiph.org's
  BSD-like license (see COPYING.Xiph), so contributions to these
  libraries should also be licensed under this license, otherwise they
  cannot be accepted
- the flac and metaflac command line programs are licensed under GPLv2, 
  see COPYING.GPL
- the helper libraries for flac and metaflac (which are in src/share)
  are licensed under varying licenses, see the license preamble for each
  file to see how they are licensed

Patches can be contributed through GitHub as a Pull Request.
Alternatively you can supply patches through the mailing list.

## Code style

FLAC does have its own peculiar coding style that does not seem to fit
general categories. You can use `git clang-format` to have your patch
auto-formatted similar to the rest of the code.
