% flac(1) Version 1.5.0 | Free Lossless Audio Codec conversion tool

# NAME

flac - Free Lossless Audio Codec

# SYNOPSIS

**flac** \[ *OPTIONS* \] \[ *infile.wav* \| *infile.rf64* \|
*infile.aiff* \| *infile.raw* \| *infile.flac* \| *infile.oga* \|
*infile.ogg* \| **-** *...* \]

**flac** \[ **-d** \| **\--decode** \| **-t** \| **\--test** \| **-a** \|
**\--analyze** \] \[ *OPTIONS* \] \[ *infile.flac* \| *infile.oga* \|
*infile.ogg* \| **-** *...* \]

# DESCRIPTION

**flac** is a command-line tool for encoding, decoding, testing and
analyzing FLAC streams.

# GENERAL USAGE

**flac** supports as input RIFF WAVE, Wave64, RF64, AIFF, FLAC or Ogg
FLAC format, or raw interleaved samples. The decoder currently can output
to RIFF WAVE, Wave64, RF64, or AIFF format, or raw interleaved samples.
flac only supports linear PCM samples (in other words, no A-LAW, uLAW,
etc.), and the input must be between 4 and 32 bits per sample.

flac assumes that files ending in ".wav" or that have the RIFF WAVE
header present are WAVE files, files ending in ".w64" or have the Wave64
header present are Wave64 files, files ending in ".rf64" or have the
RF64 header present are RF64 files, files ending in ".aif" or ".aiff" or
have the AIFF header present are AIFF files, files ending in ".flac"
or have the FLAC header present are FLAC files and files ending in ".oga"
or ".ogg" or have the Ogg FLAC header present are Ogg FLAC files.

Other than this, flac makes no assumptions about file extensions, though
the convention is that FLAC files have the extension ".flac"
(or ".fla" on ancient "8.3" file systems like FAT-16).

Before going into the full command-line description, a few other things
help to sort it out:

1.	flac encodes by default, so you must use -d to decode
2.	Encoding options -0 .. -8 (or \--fast and \--best) that control the
	compression level actually are just synonyms for different groups of
	specific encoding options (described later).  
3.	The order in which options are specified is generally not important 
	except when they contradict each other, then the latter takes 
	precedence except that compression presets are overridden by any
	option given before or after. For example, -0M, -M0, -M2 and -2M are 
	all the same as -1, and -l 12 -6 the same as -7.
4.	flac behaves similarly to gzip in the way it handles input and output 
	files

Skip to the EXAMPLES section below for examples of some typical tasks.

flac will be invoked one of four ways, depending on whether you are
encoding, decoding, testing, or analyzing. Encoding is the default
invocation, but can be switch to decoding with **-d**, analysis with
**-a** or testing with **-t**. Depending on which way is chosen,
encoding, decoding, analysis or testing options can be used, see section
OPTIONS for details. General options can be used for all.

If only one inputfile is specified, it may be "-" for stdin. When stdin
is used as input, flac will write to stdout. Otherwise flac will perform
the desired operation on each input file to similarly named output files
(meaning for encoding, the extension will be replaced with ".flac", or
appended with ".flac" if the input file has no extension, and for
decoding, the extension will be ".wav" for WAVE output and ".raw" for raw
output). The original file is not deleted unless \--delete-input-file is
specified.

If you are encoding/decoding from stdin to a file, you should use the -o
option like so:

    flac [options] -o outputfile
    flac -d [options] -o outputfile

which are better than:

    flac [options] > outputfile
    flac -d [options] > outputfile

since the former allows flac to seek backwards to write the STREAMINFO or
RIFF WAVE header contents when necessary.

Also, you can force output data to go to stdout using -c.

To encode or decode files that start with a dash, use \-- to signal the
end of options, to keep the filenames themselves from being treated as
options:

    flac -V -- -01-filename.wav

The encoding options affect the compression ratio and encoding speed. The
format options are used to tell flac the arrangement of samples if the
input file (or output file when decoding) is a raw file. If it is a RIFF
WAVE, Wave64, RF64, or AIFF file the format options are not needed since
they are read from the file's header.

In test mode, flac acts just like in decode mode, except no output file
is written. Both decode and test modes detect errors in the stream, but
they also detect when the MD5 signature of the decoded audio does not
match the stored MD5 signature, even when the bitstream is valid.

flac can also re-encode FLAC files. In other words, you can specify a
FLAC or Ogg FLAC file as an input to the encoder and it will decoder it
and re-encode it according to the options you specify. It will also
preserve all the metadata unless you override it with other options (e.g.
specifying new tags, seekpoints, cuesheet, padding, etc.).

flac has been tuned so that the default settings yield a good speed vs.
compression tradeoff for many kinds of input. However, if you are looking
to maximize the compression rate or speed, or want to use the full power
of FLAC's metadata system, see the page titled 'About the FLAC Format' on
the FLAC website.

# EXAMPLES

Some typical encoding and decoding tasks using flac:

## Encoding examples

`flac abc.wav`
:	Encode abc.wav to abc.flac using the default compression setting. abc.wav is not deleted.

`flac --delete-input-file abc.wav`
:	Like above, except abc.wav is deleted if there were no errors.

`flac --delete-input-file -w abc.wav`
:	Like above, except abc.wav is deleted if there were no errors and no warnings.

`flac --best abc.wav` or `flac -8 abc.wav`
:	Encode abc.wav to abc.flac using the highest compression preset. 

`flac --verify abc.wav` or `flac -V abc.wav`
:	Encode abc.wav to abc.flac and internally decode abc.flac to make sure it matches abc.wav.

`flac -o my.flac abc.wav`
:	Encode abc.wav to my.flac.

`flac abc.aiff foo.rf64 bar.w64`
:	Encode abc.aiff to abc.flac, foo.rf64 to foo.flac and bar.w64 to bar.flac

`flac *.wav *.aif?`
:	Wildcards are supported. This command will encode all .wav files and all 
	.aif/.aiff/.aifc files (as well as other supported files ending in 
 	.aif+one character) in the current directory.

`flac abc.flac --force` or `flac abc.flac -f`
:	Recompresses, keeping metadata like tags. The syntax is a little 
	tricky: this is an *encoding* command (which is the default: you need 
	to specify -d for decoded output), and will thus want to output the 
	file abc.flac - which already exists. flac will require the \--force 
	or shortform -f option to overwrite an existing file. Recompression 
	will first write a temporary file, which afterwards replaces the old 
	abc.flac (provided flac has write access to that file).
	The above example uses default settings. More often, recompression is
	combined with a different - usually higher - compression option.
 	Note: If the FLAC file does not end with .flac - say, it is abc.fla
	- the -f is not needed: A new abc.flac will be created and the old 
	kept, just like for an uncompressed input file.

`flac --tag-from-file="ALBUM=albumtitle.txt" -T "ARTIST=Queen" *.wav`
:	Encode every .wav file in the directory and add some tags. Every 
	file will get the same set of tags.
	Warning: Will wipe all existing tags, when the input file is (Ogg) 
	FLAC - not just those tags listed in the option. Use the metaflac 
	utility to tag FLAC files.

`flac --keep-foreign-metadata-if-present abc.wav`
:	FLAC files can store non-audio chunks of input WAVE/AIFF/RF64/W64
	files. The related option \--keep-foreign-metadata works the same
	way, but will instead exit with an error if the input has no such 
	non-audio chunks.
	The encoder only stores the chunks as they are, it cannot import 
	the content into its own tags (vorbis comments). To transfer such
	tags from a source file, use tagging software which supports them.

`flac -Vj2 -m3fo Track07.flac  -- -7.wav`
:	This longer example displays how shortform "single dash" options can 
	be written concatenated until one that takes an argument. In this 
	example, -j and -o do; thus, after the "j2" a whitespace is needed 
	to start new options with single/double dash. The -m option takes 
	no argument (the following "3" is the -3 compression preset). 
	This set of options could equally well have been written out as 
	-V -j 2 -m -3 -f -o Track07.flac , or as -foTrack07.flac -3mVj2. 
	The `-- ` (with whitespace!) signifies end of options, treating 
	everything to follow as filename; that is needed when an input 
	filename could otherwise be read as an option, like this "-7".  
	In total, this line takes the input file -7.wav as input; -o sets 
	output filename to Track07.flac, and the -f will overwrite if the
	file Track07.flac already exists. The encoder will select encoding 
	preset -3 modified with the -m switch, and use two CPU threads. 
	Afterwards, the -V will make it decode the FLAC file and compare 
	the audio to the input, to ensure they are indeed equal. 

## Decoding examples

`flac --decode abc.flac` or `flac -d abc.flac`
:	Decode abc.flac to abc.wav. abc.flac is not deleted. If abc.wav 
	already exists, the process will print an *error* instead of 
	overwriting; use --force / -f to force overwrite.
	NOTE: A mere flac abc.flac *without --decode or its shortform -d*, 
	would mean to re-encode abc.flac to abc.flac (see above), and that
	command would err out because abc.flac already exists.

`flac -d --force-aiff-format abc.flac` or `flac -d -o abc.aiff abc.flac`
:	Two different ways of decoding abc.flac to abc.aiff (AIFF format).
	abc.flac is not deleted. -d -o could be shortened to -do.
	The decoder can force other output formats, or different versions 
	of the WAVE/AIFF formats, see the options below.

`flac -d --keep-foreign-metadata-if-present abc.flac`
:	If the FLAC file has non-audio chunks stored from the original
	input file, this option will restore both audio and non-audio. 
	The chunks will reveal the original file type, and the decoder 
	will select output format and output file extension accordingly. 
	If attempting to force a particular output format, including with
	e.g. -o abc.wav, it must be the correct one according to the 
	metadata chunk - the decoder cannot convert chunks between formats.
	(If there are no such chunks stored, it will decode to abc.wav, 
	with a *warning*, whereas the related \--keep-foreign-metadata 
	option would exit with an *error* rather than decoding.)

`flac -d -F abc.flac`
:	Decode abc.flac to abc.wav, not aborting upon *error*. Potentially
	useful for recovering as much as possible from a corrupted file.  
	CAUTION: Be careful about trying to "repair" files this way. Often 
	it will only conceal errors, and not play any subjectively 
	"better" than the corrupted source file. It is a good idea to keep 
	the source, and if possible: try several decoders including the one 
	that generated the file (3rd party or older **flac**); and, listen 
	if one has less detrimental audible flaws than another (at limited 
	output volume - corrupted audio can generate loud noises). 


# OPTIONS

A summary of options follows; see also section **Negative options** for 
negating options. The **Format options** section describes ways to 
select format upon decoding, and upon encoding from raw or to Ogg FLAC.

## GENERAL OPTIONS

**-v**, **\--version**
:	Show the **flac** version number, and quit.

**-h**, **\--help**
:	Show basic usage and a list of all options, and quit.

**-d**, **\--decode**
:	Decode (the default is to encode, thus to re-encode if the infile is
	(Ogg) FLAC). Will exit with an *error* if the audio bit stream is not
	valid, including incorrect MD5 checksum. (-F will ignore any *error*.)

**-t**, **\--test**
:	Test a FLAC / Ogg FLAC encoded file. Works like -d except no decoded 
	output, but does some additional checks (including metadata). 

**-a**, **\--analyze**
:	Analyze a FLAC / Ogg FLAC encoded file. Works like -d except the 
	output is an analysis file (.ana by default), not a decoded file.

**-c**, **\--stdout**
:	Write output to stdout.

**-f**, **\--force**
:	Force overwriting output files. Without this option, the default is 
	to print a *warning* that the output file already exists, skip it and 
	continue to the next file.

**\--delete-input-file**
:	(Ignored for -a and -t modes.) Automatically delete the input file 
	upon successful encode or decode. If there was an *error* 
	(including a verify error) the input file is left intact.  
	Use -w to retain the input file also when there is a *warning*.

**-o** *FILENAME*, **\--output-name**=*FILENAME*
:	Set output file name (usually **flac** merely changes the extension); 
	and upon decoding, set also output file *type* by file extension. 
	Quote filename as needed. Option can only be used when processing a 
	single file. May not be used in conjunction with \--output-prefix. 

**\--output-prefix**=*STRING*
:	Prefix each output file name with the given string. Quote as needed.
	This can also be useful for outputting to a different directory - if 
	so, make sure the directory exists, and that the *STRING* ends with 
	a directory slash \`/' (not a Windows-style backslash).

**\--preserve-modtime**
:	(Enabled by default.) Output files have their timestamps/permissions 
	set to match those of their inputs. Use \--no-preserve-modtime to make
	output files have the current time and default permissions.

**\--keep-foreign-metadata**
:	Store/restore non-audio chunks of WAVE, RF64, Wave64 or AIFF files. 
	Input and output must be regular files (not stdin nor stdout). 
	Encoding: store these chunks as an APPLICATION metadata block. 
	(Upon re-encoding, any stored chunks will be retained automatically, 
	no matter whether **\--keep-foreign-metadata** is given or not.)  
	Decoding: restore any saved non-audio chunks to the decoded file,
	where output file type will be set according to metadata. 
	When this option is given, **flac** will exit with *error* if no such 
	chunks are found (use **\--keep-foreign-metadata-if-present** instead) 
	or if trying to decode to a file type not matching these chunks. 
	NOTE: **flac** cannot transcode foreign metadata; e.g. WAVE chunks can
	not be converted to FLAC tags, nor be restored when decoding to AIFF.

**\--keep-foreign-metadata-if-present**
:	Like \--keep-foreign-metadata, but will continue (although print a 
	*warning*) if no foreign metadata can be found or restored. Will 
	exit with *error* upon trying to decode to mismatching file type.

**\--skip**={\#\|*MM:SS*}
:	Skip the first \# samples of the input (of *each* input file - all 
	must exceed \# samples, or **flac** will exit with an *error*).  
	Alternatively: skip the first *MM:SS* minutes and seconds (needs at 
	least one digit on each side of the colon sign). Decimal point 
	must be locale-dependent, e.g. \--skip=0:9,87 in case of a comma.  
	This option cannot be used with -t. When used with -a, the analysis
	file will enumerate frames starting from the \--skip point.

**\--until**=\[+\|-\]{\#\|*MM:SS*}
:	\--until=\# discards everything after the \# first samples; in 
	conjunction with \--skip=n, only \# minus n samples are processed.
	\--until=+\# will process \# samples after the \--skip point (in 
	the absence of such: from the beginning, like \--until=\#).  
	\--until=-\# will discard the last \# samples from the input.  
	**flac** will exit with an error if \# exceeds input size or is 
	too small/too large for the \--skip point. \--until also accepts a
	time argument *MM:SS* like \--skip. It cannot be used with -t.

**-s**, **\--silent**
:	Do not print runtime encode/decode statistics to console/stderr.

**\--totally-silent**
:	Do not print anything of any kind, including *warnings* or *errors*. 
	Only the exit code will inform about successful completion or not.

**-w**, **\--warnings-as-errors**
:	Treat any *warning* as an *error*, causing **flac** to terminate with a
	non-zero exit code. (Exceptions apply, will be stated.)

**\--** 
:	End of options; everything following `-- ` is input file(s) even if 
	starting with "-". E.g. `flac -d *.flac` fails upon encountering the
	file "-1.flac", but `flac -d -- *.flac` works.


## DECODING OPTIONS

**-F**, **\--decode-through-errors**
:	By default flac stops decoding with an error message and removes the
	partially decoded file if it encounters a bitstream error. With -F,
	errors are still printed but flac will continue decoding to
	completion. Note that errors may cause the decoded audio to be
	missing some samples or have silent sections.

**\--cue**=\[\#.#\]\[-\[\#.#\]\]
:	Set the beginning and ending cuepoints to decode. Decimal points are
	locale-dependent (dot or comma). The optional first \#.# is the track
	and index point at which decoding will start; the default is the
	beginning of the stream. The optional second \#.# is the track and
	index point at which decoding will end; the default is the end of
	the stream. If the cuepoint does not exist, the closest one before
	it (for the start point) or after it (for the end point) will be
	used. If those don't exist , the start of the stream (for the start
	point) or end of the stream (for the end point) will be used. The
	cuepoints are merely translated into sample numbers then used as
	\--skip and \--until. A CD track can always be cued by, for example,
	\--cue=9.1-10.1 for track 9, even if the CD has no 10th track.

**--decode-chained-stream**
: Decode all links in a chained Ogg stream, not just the first one.

**\--apply-replaygain-which-is-not-lossless**\[=*SPECIFICATION*\]
:	Applies ReplayGain values while decoding. **WARNING: THIS IS NOT
	LOSSLESS. DECODED AUDIO WILL NOT BE IDENTICAL TO THE ORIGINAL WITH
	THIS OPTION.** This option is useful for example in transcoding
	media servers, where the client does not support ReplayGain. For
	details on the use of this option, see the section **ReplayGain
	application specification**.


## ENCODING OPTIONS

The encoder will auto-detect input format except headerless raw PCM, and 
by default output *.flac* file extension - though *.oga* if \--ogg sets 
Ogg FLAC output. See section **Format options** for raw / Ogg FLAC.

Encoding defaults to the -5 compression preset, working single-threaded.  
Each preset -0 to -8 is a synonym for a set of options, options explained 
after -8 below. All presets comply with the stricter *streamable subset* 
of the FLAC format (see RFC 9639 section 7); the encoder requires the 
\--lax option to output non-Subset FLAC, or it will exit with an *error*.

**-0**, **\--compression-level-0**, **\--fast**
:	Fastest compression preset. Currently synonymous with `-l 0 -b 1152 -r 3 --no-mid-side`

**-1**, **\--compression-level-1**
:	Currently synonymous with `-l 0 -b 1152 -M -r 3`

**-2**, **\--compression-level-2**
:	Currently synonymous with `-l 0 -b 1152 -m -r 3`

**-3**, **\--compression-level-3**
:	Currently synonymous with `-l 6 -b 4096 -r 4 --no-mid-side`

**-4**, **\--compression-level-4**
:	Currently synonymous with `-l 8 -b 4096 -M -r 4`

**-5**, **\--compression-level-5**
:	Currently synonymous with `-l 8 -b 4096 -m -r 5`

**-6**, **\--compression-level-6**
:	Currently synonymous with `-l 8 -b 4096 -m -r 6 -A "subdivide_tukey(2)"`

**-7**, **\--compression-level-7**
:	Currently synonymous with `-l 12 -b 4096 -m -r 6 -A "subdivide_tukey(2)"`

**-8**, **\--compression-level-8**, **\--best**
:	Currently synonymous with `-l 12 -b 4096 -m -r 6 -A "subdivide_tukey(3)"`


**-l** \#, **\--max-lpc-order**=\#
:	Sets the maximum LPC order. This number must be \<= 32. 
	For *subset* streams, it must be \<=12 if the sample rate is \<=48kHz. 
	If set to 0, the encoder will not attempt generic linear prediction, and
	choose only among a set of "fixed" predictors hard-coded in the FLAC 
	format - faster, but compresses weaker (typically a few percent). 

**-b** \#, **\--blocksize**=\#
:	Sets blocksize in samples, 16 \<= \# \<= 65535. Current default is
	1152 for -l 0, else 4096. For *subset* streams, \# must be \<= 4608 
	if the sample rate is \<= 48kHz and \<= 16384 for higher sample rates. 

**-m**, **\--mid-side**
:	Try mid-side coding for each frame in addition to left and right, and 
	select the best compression. (Stereo only, ignored otherwise.)

**-M**, **\--adaptive-mid-side**
:	Like -m, but choose with a heuristic (faster, slightly weaker
	compression).

**-r** \[\#,\]\#, **\--rice-partition-order**=\[\#,\]\#
:	Set the \[min,\]max residual partition order (0..15). For *subset* 
	streams, "max" must be \<=8. "min" defaults to 0. Default is -r 5.
	Actual partitioning will be restricted by block size and prediction 
	order, and the encoder will silently reduce too high values. 

**-A** *FUNCTION(S)*, **\--apodization**=*FUNCTION(S)*
:	Apply apodization *FUNCTION* in the LPC analysis (if more are given: 
	try them all), see subsection **Apodization functions for encoding** 
	for how to use this option. Does nothing if using -l 0.

**-e**, **\--exhaustive-model-search**
:	Do exhaustive model search (expensive!).

**-q** \#, **\--qlp-coeff-precision**=\#
:	Set precision (in bits) of the quantized linear-predictor 
	coefficients, 5\<= \# \<=15 or the default 0 to let encoder decide. 
	Does nothing if using -l 0. The encoder may reduce the actual 
	quantization below the \# number by signal and prediction order.

**-p**, **\--qlp-coeff-precision-search**
:	Do exhaustive search for optimal LP coefficient precision 
	(expensive!). Overrides -q; does nothing if using -l 0.

**\--lax**
:	Allow encoding to non-*subset* FLAC files, see RFC 9639 section 7.  
	CAUTION: non-*subset* files may fail decoding/streaming/playback 
	in certain applications (especially legacy hardware devices).

**\--limit-min-bitrate**
:	Ensure that bitrate stays at least 1 bit/sample at any time (e.g. 
	48 kbit/s for 48 kHz). Mainly useful for internet streaming.

**\--ignore-chunk-sizes**
:	Ignore file size headers in WAVE or AIFF source files. Useful when 
	certain applications write malformed WAVE/AIFF files allowing the 
	audio to extend past the maximum possible size of the format; this 
	option reads to the end of file, intended to capture all audio.  
	CAUTION: Use only when needed. Even if those malformed files described
	are often intended to be read this way, *compliant* files could have 
	the audio chunk followed by data (tags including pictures), and which 
	will then be (mis-) interpreted as audio by \--ignore-chunk-sizes. 
	Thus, this option cannot be used with \--keep-foreign-metadata /
	\--keep-foreign-metadata-if-present, nor \--cue, \--cuesheet, \--until. 

**-j** \#, **\--threads**=\#
:	By default, **flac** will encode with one thread. This option enables 
	multithreading with max \# number of threads, although "0" to let the 
	encoder decide. Currently, -j 0 is synonymous with -j 1 (i.e. no
	multithreading), and the max supported number is 64; both could change
	in the future. If \# exceeds the supported maximum (64), **flac** will 
	encode with a single thread (and print a *warning*). The same happens 
	(for any \#) if **flac** was compiled with multithreading disabled. 
	NOTE: Exceeding the *actual* available CPU threads, harms speed.

**-V**, **\--verify**
:	Verify a correct encoding by decoding the output in parallel and
	comparing the audio bit by bit to the original.


**\--picture**={*FILENAME\|SPECIFICATION*}
:	Import a picture and store it in a PICTURE metadata block, one per 
	\--picture option given (keeping existing ones upon re-encoding). 
	A *FILENAME* argument is shorthand for a *SPECIFICATION* with default 
	values applied (type set to front cover, properties to be inferred 
	from the picture file); see subsection **Picture specification**.
	Currently the **flac** encoder handles up to 64 \--picture options; 
	for more, add afterwards (**metaflac** or tagging software).  
	NOTE: The FLAC format is limited to 16 MiB *total* metadata. Also, 
	several applications may reject FLAC files with very high *total* 
	picture count (like a thousand) even when the 16 MiB bound is met. 

**\--cuesheet**=*FILENAME*
:	Import file *FILENAME* and store its content in a CUESHEET metadata 
	block. This option may only be used when encoding a single file, 
	and only one CUESHEET block can exist; giving more \--cuesheet 
	options will discard all but the last. A *warning* will be printed 
	when \--cuesheet overwrites an existing CUESHEET upon re-encoding. 
	The content of the cuesheet file will be converted to the CUESHEET 
	block's format, silently discarding other information (like most 
	"CD-TEXT" content, including PERFORMER or COMPOSER data; ISRC is 
	stored. For details, see the format specification section 8.7).
	Each index point will get a seekpoint added to the SEEKTABLE, 
	unless overridden by \--no-cued-seekpoints.

**\--no-utf8-convert**
:	Upon tagging, do *not* convert tags from local charset to UTF-8. 
	This is useful for scripts, and for overriding a wrong locale.  
	NOTE: This option must appear *before* the applicable tag options!

**-T** "*FIELD=VALUE*"**, \--tag**="*FIELD=VALUE*"
:	Set a FLAC tag; like Ogg, the FLAC format uses Vorbis comments, see 
	the format specification section 8.6. Quote content as necessary.
	NOTE: All -T options will apply to every output (Ogg) FLAC file.
	CAUTION: -T or \--tag or \--tag-from-file will completely disable 
	the usual tag transfer from FLAC sources. Do not use -T upon 
	re-encoding unless you want to erase *all* existing tags. 

**\--tag-from-file**="*FIELD=FILENAME*"
:	Like \--tag, except populates the FIELD by the verbatim content of 
	file FILENAME, for example \--tag-from-file="LYRICS=hello.lrc"  
	NOTE: Do not try to store binary data in tag fields! Use PICTURE 
	blocks for pictures and APPLICATION blocks for other binary data.

**\--replay-gain**
:	Calculate ReplayGain values and store them as FLAC tags, using the 
	original ReplayGain algorithm (similar to vorbisgain; not EBU128). 
	Track gain / track peak will be computed for each input file, and 
	an album gain/peak will be computed over all input files. 
	Only mono and stereo are supported, and all files must share channel 
	count, bits per sample, and sample rate, which also must be among 
	the following: 8, 11.025, 12, 16, 18.9, 22.05, 24, 28, 32, 36, 37.8, 
	44.1, 48, 56, 64, 72, 75.6, 88.2, 96, 112, 128, 144, 151.2, 176.4, 
	192, 224, 256, 288, 302.4, 352.8, 384, 448, 512, 576, or 604.8 kHz.  
	NOTE: This option cannot be used when encoding to stdout nor Ogg FLAC.

**-S** {\#\|\#x\|\#s\|X}, **\--seekpoint**={\#\|\#x\|\#s\|X}
:	Sets seekpoint(s), overriding the default choice of one per ten seconds
	('-s 10s'). When several -S options are given, the resulting SEEKTABLE 
	will contain all the seekpoints (duplicates removed), max 32768.  
	Seekpoints will be added as follows: \# for one at that sample number, 
	ignored if exceeding the total sample count; \#x for \# evenly spaced 
	seek points, the first at sample 0; \#s for one every \# seconds (with 
	locale-dependent decimal point, e.g. '-s 9.5s' or '-s 9,5s'). X will 
	add a *placeholder point* at the end of the table.  
	NOTE: If the encoder cannot determine the input size before starting,
	'-S \#' results in a placeholder, while \#x and \#s will be ignored.
	Use \--no-seektable for no SEEKTABLE. 

**-P** \#, **\--padding**=\#
:	Writes a PADDING block of \# bytes (plus 4 for block header) in the 
	metadata section before the audio, overriding the encoder's default 
	(of 8192, although 65536 for input \> 20 min.); if more -P options 
	are given, all but the last are silently ignored. PADDING blocks 
	are useful for later tagging, leaving room to overwrite only the 
	metadata section instead of having to rewrite the entire file. 

## FORMAT OPTIONS

Encoding defaults to FLAC and not OGG. Decoding defaults to WAVE (more
specifically WAVE\_FORMAT\_PCM for mono/stereo with 8/16 bits, and to 
WAVE\_FORMAT\_EXTENSIBLE otherwise), except: will be overridden by chunks 
found by \--keep-foreign-metadata-if-present or \--keep-foreign-metadata 

**\--ogg**
:	When encoding, generate Ogg FLAC output instead of native FLAC. Ogg
	FLAC streams are FLAC streams wrapped in an Ogg transport layer. The
	resulting file should have an '.oga' extension and will still be
	decodable by flac. When decoding, force the input to be treated as
	Ogg FLAC. This is useful when piping input from stdin or when the
	filename does not end in '.oga' or '.ogg'.

**\--serial-number**=\#
:	When used with \--ogg, specifies the serial number to use for the
	first Ogg FLAC stream, which is then incremented for each additional
	stream. When encoding and no serial number is given, flac uses a
	random number for the first stream, then increments it for each
	additional stream. When decoding and no number is given, flac uses
	the serial number of the first page.

**\--force-aiff-format**  
**\--force-rf64-format**  
**\--force-wave64-format**
:	For decoding: Override default output format and force output to 
	AIFF/RF64/WAVE64, respectively.
	This option is not needed if the output filename (as set by -o) 
	ends with *.aif* or *.aiff*, *.rf64* and *.w64* respectively. 
 	The encoder auto-detects format and ignores this option. 

**\--force-legacy-wave-format**  
**\--force-extensible-wave-format**
:	Instruct the decoder to output a WAVE file with WAVE\_FORMAT\_PCM and
	WAVE\_FORMAT\_EXTENSIBLE respectively, overriding default choice.

**\--force-aiff-c-none-format**  
**\--force-aiff-c-sowt-format**
:	Instruct the decoder to output an AIFF-C file with format NONE and
	sowt respectively.

**\--force-raw-format**
:	Decoding: set output to raw samples. Output will default to *.raw* file 
	extension but the decoder will not object if -o sets a different one.  
	Encoding from uncompressed: the option should usually be omitted, as 
	the mandatory raw options will make the encoder treat input as raw; 
	to guard against user mistakes, the encoder will normally print a 
	*warning* if encountering a WAVE/AIFF/RF64/W64 input file to be
	treated as raw, but \--force-raw-format will suppress this warning.  
	Re-encoding from FLAC or Ogg FLAC: If you really wish to treat a FLAC 
	or Ogg FLAC as raw, you need to invoke \--force-raw-format, as **flac**
	will otherwise exit with an *error* (again, to guard against mistakes).

### raw format options

For raw PCM, the format must be specified by the user - although for 
decoding, only the properties not known from the FLAC stream (sign and
endianness). **flac** will exit with an *error* both upon a missing 
mandatory option, and upon encountering one that should not be there.

**\--sign**={signed\|unsigned}
:	(Input from raw or output to raw) Signedness of each sample.

**\--endian**={big\|little}
:	(Input from raw or output to raw) Byte order of each sample.

**\--channels**=\#
:	(Input only) Number of channels. The channels must be interleaved,
	and in the order of the FLAC format, see the format specification; 
	the encoder (/decoder) cannot re-order channels.

**\--bps**={8\|16\|24\|32}
:	(Input only) Bits per sample (per channel: 16 for CDDA.)

**\--sample-rate**=\#
:	(Input only) Sample rate (in Hz. Only integers supported.)

**\--input-size**=\#
:	(Input from stdin only) Size of the raw input in bytes. 
	This option can only be used when encoding from stdin, and is only 
	needed in conjunction with options that need to know the input size 
	beforehand (like, \--skip, \--until, \--cuesheet ) 
	Upon a mismatch to actual size, the encoder will either truncate or 
	print a *warning* about unexpected end-of-file. 

## ANALYSIS OPTIONS

**\--residual-text**
:	Includes the residual signal in the analysis file. This will make the
	file very big, much larger than even the decoded file.

**\--residual-gnuplot**
:	Generates a gnuplot file for every subframe; each file will contain
	the residual distribution of the subframe. This will create a lot of
	files. gnuplot must be installed separately. 

## NEGATIVE OPTIONS

The following will negate an option (a default or one previously given):

**\--no-adaptive-mid-side**  
**\--no-cued-seekpoints**  
**\--no-decode-through-errors**  
**\--no-delete-input-file**  
**\--no-preserve-modtime**  
**\--no-keep-foreign-metadata**  
**\--no-exhaustive-model-search**  
**\--no-force**  
**\--no-lax**  
**\--no-mid-side**  
**\--no-ogg**  
**\--no-padding**  
**\--no-qlp-coeff-prec-search**  
**\--no-replay-gain**  
**\--no-residual-gnuplot**  
**\--no-residual-text**  
**\--no-seektable**  
**\--no-silent**  
**\--no-verify**  
**\--no-warnings-as-errors**

## ReplayGain application specification
The option \--apply-replaygain-which-is-not-lossless\[=\<specification\>\]
applies ReplayGain values while decoding. **WARNING: THIS IS NOT
LOSSLESS. DECODED AUDIO WILL NOT BE IDENTICAL TO THE ORIGINAL WITH THIS
OPTION.** This option is useful for example in transcoding media servers,
where the client does not support ReplayGain.
	
The \<specification\> is a shorthand notation for describing how to	apply
ReplayGain. All elements are optional - defaulting to 0aLn1 - but order 
is important.  The format is: 
	
\[\<preamp\>\]\[a\|t\]\[l\|L\]\[n{0\|1\|2\|3}\]

In which the following parameters are used:

-	**preamp**: A floating point number in dB. This is added to the 
	existing gain value.

-	**a\|t**: Specify 'a' to use the album gain, or 't' to use the track
	gain. If tags for the preferred kind (album/track) do not exist but
	tags for the other (track/album) do, those will be used instead.  

-	**l\|L**: Specify 'l' to peak-limit the output, so that the 
	ReplayGain peak value is full-scale. Specify 'L' to use a 6dB hard
	limiter that kicks in when the signal approaches full-scale.

-	**n{0\|1\|2\|3}**: Specify the amount of noise shaping. ReplayGain
	synthesis happens in floating point; the result is dithered before
	converting back to integer. This quantization adds noise. Noise
	shaping tries to move the noise where you won't hear it as much.
	0 means no noise shaping, 1 means 'low', 2 means 'medium', 3 means
	'high'.

For example, the default of 0aLn1 means 0dB preamp, use album gain, 6dB
hard limit, low noise shaping. \--apply-replaygain-which-is-not-lossless=3
means 3dB preamp, use album gain, no limiting, no noise shaping.

flac uses the ReplayGain tags for the calculation. If a stream does
not have the required tags or they can't be parsed, decoding will
continue with a warning, and no ReplayGain is applied to that stream.

## Picture specification
This described the specification used for the **\--picture** option.
\[*TYPE*\]\|\[*MIME-TYPE*\]\|\[*DESCRIPTION*\]\|\[*WIDTHxHEIGHTxDEPTH*\[/*COLORS*\]\]\|*FILE*

*TYPE* is optional; it is a number from one of:

 0. Other
 1. PNG file icon of 32x32 pixels (see RFC 2083)
 2. General file icon
 3. Front cover
 4. Back cover
 5. Liner notes page
 6. Media label (e.g., CD, Vinyl or Cassette label)
 7. Lead artist, lead performer, or soloist
 8. Artist or performer
 9. Conductor
 10. Band or orchestra
 11. Composer
 12. Lyricist or text writer
 13. Recording location
 14. During recording
 15. During performance
 16. Movie or video screen capture
 17. A bright colored fish (from ID3v2, use discouraged)
 18. Illustration
 19. Band or artist logotype
 20. Publisher or studio logotype

The default is 3 (front cover). There may only be one picture each of
type 1 and 2 in a file.

*MIME-TYPE* is optional; if left blank, it will be detected from the file.
For best compatibility with players, use pictures with MIME type
image/jpeg or image/png. The MIME type can also be \--\> to mean that
FILE is actually a URL to an image, though this use is discouraged.

*DESCRIPTION* is optional; the default is an empty string.

The next part specifies the resolution and color information. If the
*MIME-TYPE* is image/jpeg, image/png, or image/gif, you can usually leave
this empty and they can be detected from the file. Otherwise, you must
specify the width in pixels, height in pixels, and color depth in
bits-per-pixel. If the image has indexed colors you should also specify
the number of colors used. When manually specified, it is not checked
against the file for accuracy.

*FILE* is the path to the picture file to be imported, or the URL if MIME
type is \--\>

**Specification examples:** 
"\|image/jpeg\|\|\|../cover.jpg" will embed the 
JPEG file at ../cover.jpg, defaulting to type 3 (front cover) and an 
empty description. The resolution and color info will be retrieved 
from the file itself. 
"4\|\--\>\|CD\|320x300x24/173\|http://blah.blah/backcover.tiff" will
embed the given URL, with type 4 (back cover), description "CD", and a
manually specified resolution of 320x300, 24 bits-per-pixel, and 173
colors. The file at the URL will not be fetched; the URL itself is
stored in the PICTURE metadata block.

## Apodization functions
To improve LPC analysis, the audio data is windowed. An **-A** option 
applies the specified apodization function(s) instead of the default 
(which is "tukey(5e-1)", though different for presets -6 to -8.)
Specifying one more function effectively means, for each subframe, to 
try another weighting of the data and see if it happens to result in a 
smaller encoded subframe. Specifying several functions is time-expensive, 
at typically diminishing compression gains. 

The subdivide_tukey(*N*) functions (see below) used in presets -6 to -8 
were developed to recycle calculations for speed, compared to using a 
number of independent functions. Even then, a high number like *N*\>4 
or 5, will often become less efficient than other options considered 
expensive, like the slower -p, though results vary with signal.

Up to 32 functions can be given as semicolon-separated list and/or as 
individual **-A** options. Quote as necessary. Any mis-specified function 
is silently ignored. Currently the following functions are implemented: 
bartlett, bartlett_hann, blackman, blackman_harris_4term_92db, connes, 
flattop, gauss(*STDDEV*), hamming, hann, kaiser_bessel, nuttall, 
rectangle, triangle, tukey(*P*), partial_tukey(*N*\[/*OV*\[/*P*\]\]), 
punchout_tukey(*N*\[/*OV*\[/*P*\]\]), subdivide_tukey(*N*\[/*P*\]), welch.

For parameters *P*, *STDDEV* and *OV*, scientific notation is supported, e.g. 
tukey(5e-1). Otherwise, the decimal point must agree with the locale, 
e.g. tukey(0.5) or tukey(0,5) depending on your system.

- For gauss(*STDDEV*), *STDDEV* is the standard deviation (0\<*STDDEV*\<=5e-1).

- For tukey(*P*), *P* (between 0 and 1) specifies the fraction of the window 
that is cosine-tapered; *P*=0 corresponds to "rectangle" and *P*=1 to "hann". 

- partial_tukey(*N*) and punchout_tukey(*N*) are largely obsoleted by the 
more time-effective subdivide_tukey(*N*), see next item. They generate *N* 
functions each spanning a part of each block. Optional arguments are an 
overlap *OV* (\<1, may be negative), for example partial_tukey(2/2e-1); 
and then a taper parameter *P*, for example partial_tukey(2/2e-1/5e-1).

- subdivide_tukey(*N*) is a more efficient reimplementation of partial_tukey 
and punchout_tukey taken together, combining the windows they would 
generate up to the specified *N*. Specifying subdivide_tukey(3) entails a 
tukey, a partial_tukey(2), a partial_tukey(3) and a punchout_tukey(3); 
specifying subdivide_tukey(5) will on top of that add a partial_tukey(4), 
a punchout_tukey(4), a partial_tukey(5) and a punchout_tukey(5) - but all 
with tapering chosen to facilitate the re-use of computation. Thus the *P* 
parameter (defaulting to 5e-1) is applied for the smallest used window:
For example, subdivide_tukey(2/5e-1) results in the same taper as that of
tukey(25e-2) and subdivide_tukey(5) in the same taper as of tukey(1e-1). 

#### Example (for illustration - impractically long!):
`-A "flattop;gauss(1e-1);tukey(3e-2);punchout_tukey(3/-9e-4/54e-2);subdivide_tukey(5/7e-1);hanning`

The encoder will for each subframe try all the following in order,
estimate the size, and pick the best which is then used to encode:

- First taper the subframe with the flattop function;
- Start anew at a gauss (STDEV of ten percent of the subframe);
- Start anew at a tukey window that tapers 15% of each end (total 30%);
- Then each of 3 windows generated by punchout_tukey: each generated by
  deleting ("punching out") a third of the subframe, with a small
  negative overlap, tapering off 27% off each end (total 54e-2);
- Finally a sequence of windows produced by subdivide_tukey: like the
  two previous ones, it applies cosine-tapering, but steeper tapers
  at the ends (the base case tapers 7e-1 divided by 5 i.e. 14 percent
  of the subframe), and successively trying subdivisions: the first
  and last half of the subframe, then the first/middle/last third etc,
  up to generating five windows, one for each fifth of the subframe.
- Wrong and discarded: "hanning" (a not-uncommon mixup of "hamming" and "hann").

# SEE ALSO

**metaflac(1)**

**flac** and **metaflac** are maintained at https://github.com/xiph/flac  
Format specification: RFC 9639, https://datatracker.ietf.org/doc/rfc9639  


# AUTHOR

This manual page was initially written by Matt Zimmerman
\<mdz@debian.org\> for the Debian GNU/Linux system. It has been 
maintained by the Xiph.org Foundation.
