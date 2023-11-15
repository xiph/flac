This document specifies three metadata application IDs to store foreign
metadata.

- For WAVE and RF64 files, ID "riff" (0x72696666) is used.
- For AIFF and AIFF-C files, ID "aiff" (0x61696666) is used.
- For Wave64 files, ID "w64 " (0x773634C0) is used.

When converting one of the aforementioned filetypes to FLAC and storage
of the foreign metadata is desired, all chunks are copied into metadata
blocks, one for each chunk. All chunks are copied completely, except for
the first chunk (which contains all others) and the chunk containing the
audio, of which only the headers (identifier and size) are copied. It is
important for proper restoration that the order of the chunks is
retained.

This format is used by the `flac` command line tool when supplied with
the `--keep-foreign-metadata` option.

It might seem superfluous to store the header of the main chunk and of
the chunk carrying audio. Similarly, much data in the WAVE fmt chunk
is duplicated in the FLAC streaminfo metadata block. However, these
chunks are kept to simplify restoring them and as an extra integrity
check. Additionally, by duplicating the header of the chunk containing
the audio data, it is possible to find out which chunks should precede
the audio data and which chunks should trail it.

As an example, consider a very simple WAVE file consisting of the main
RIFF chunk, the WAVE subchunk, the fmt subchunk and the data subchunk.
When converted with `flac --keep-foreign-metadata`, the resulting FLAC
file will have 4 application metadata blocks with ID "riff". The first
block only contains the header of the main chunk and therefore will have
8 bytes of content (and thus be 12 bytes long excluding header because
of the 4-byte ID and 16 bytes long including header). The second
metadata block will have 4 bytes of content ("WAVE"). The third metadata
block holds the complete fmt chunk. The fourth metadata block contains
only the header of the data chunk and therefore has only 8 bytes of
content.

For RF64, AIFF, AIFF-C and Wave64, the procedure is the same.
