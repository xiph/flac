/* test_libFLAC++ - Unit tester for libFLAC++
 * Copyright (C) 2002  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

extern "C" {
#include "file_utils.h"
}
#include "FLAC/assert.h"
#include "FLAC++/decoder.h"
#include "FLAC++/metadata.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcpy()/memset() */

/******************************************************************************
	The general strategy of these tests (for interface levels 1 and 2) is
	to create a dummy FLAC file with a known set of initial metadata
	blocks, then keep a mirror locally of what we expect the metadata to be
	after each operation.  Then testing becomes a simple matter of running
	a FLAC::Decoder::File over the dummy file after each operation, comparing
	the decoded metadata to what's in our local copy.  If there are any
	differences in the metadata, or the actual audio data is corrupted, we
	will catch it while decoding.
******************************************************************************/

class OurFileDecoder: public FLAC::Decoder::File {
public:
	inline OurFileDecoder(bool ignore_metadata): ignore_metadata_(ignore_metadata), error_occurred_(false) { }

	bool ignore_metadata_;;
	bool error_occurred_;
protected:
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);
};

struct OurMetadata {
	FLAC::Metadata::Prototype *blocks[64];
	unsigned num_blocks;
};

static const char *flacfile_ = "metadata.flac";

/* our copy of the metadata in flacfile_ */
static OurMetadata our_metadata_;

/* the current block number that corresponds to the position of the iterator we are testing */
static unsigned mc_our_block_number_ = 0;

static bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static bool die_c_(const char *msg, FLAC::Metadata::Chain::Status status)
{
	printf("ERROR: %s\n", msg);
	printf("       status=%u (%s)\n", (unsigned)((::FLAC__Metadata_ChainStatus)status), status.as_cstring());
	return false;
}

static bool die_ss_(const char *msg, FLAC::Metadata::SimpleIterator &iterator)
{
	const FLAC::Metadata::SimpleIterator::Status status = iterator.status();
	printf("ERROR: %s\n", msg);
	printf("       status=%u (%s)\n", (unsigned)((::FLAC__Metadata_SimpleIteratorStatus)status), status.as_cstring());
	return false;
}

static void *malloc_or_die_(size_t size)
{
	void *x = malloc(size);
	if(0 == x) {
		fprintf(stderr, "ERROR: out of memory allocating %u bytes\n", (unsigned)size);
		exit(1);
	}
	return x;
}

/* functions for working with our metadata copy */

static bool replace_in_our_metadata_(FLAC::Metadata::Prototype *block, unsigned position, bool copy)
{
	unsigned i;
	FLAC::Metadata::Prototype *obj = block;
	FLAC__ASSERT(position < our_metadata_.num_blocks);
	if(copy) {
		if(0 == (obj = FLAC::Metadata::clone(block)))
			return die_("during FLAC::Metadata::clone()");
	}
	delete our_metadata_.blocks[position];
	our_metadata_.blocks[position] = obj;

	/* set the is_last flags */
	for(i = 0; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i]->set_is_last(false);
	our_metadata_.blocks[i]->set_is_last(true);

	return true;
}

static bool insert_to_our_metadata_(FLAC::Metadata::Prototype *block, unsigned position, bool copy)
{
	unsigned i;
	FLAC::Metadata::Prototype *obj = block;
	if(copy) {
		if(0 == (obj = FLAC::Metadata::clone(block)))
			return die_("during FLAC::Metadata::clone()");
	}
	if(position > our_metadata_.num_blocks) {
		position = our_metadata_.num_blocks;
	}
	else {
		for(i = our_metadata_.num_blocks; i > position; i--)
			our_metadata_.blocks[i] = our_metadata_.blocks[i-1];
	}
	our_metadata_.blocks[position] = obj;
	our_metadata_.num_blocks++;

	/* set the is_last flags */
	for(i = 0; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i]->set_is_last(false);
	our_metadata_.blocks[i]->set_is_last(true);

	return true;
}

static void delete_from_our_metadata_(unsigned position)
{
	unsigned i;
	FLAC__ASSERT(position < our_metadata_.num_blocks);
	delete our_metadata_.blocks[position];
	for(i = position; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i] = our_metadata_.blocks[i+1];
	our_metadata_.num_blocks--;

	/* set the is_last flags */
	if(our_metadata_.num_blocks > 0) {
		for(i = 0; i < our_metadata_.num_blocks - 1; i++)
			our_metadata_.blocks[i]->set_is_last(false);
		our_metadata_.blocks[i]->set_is_last(true);
	}
}

void add_to_padding_length_(unsigned index, int delta)
{
	FLAC::Metadata::Padding *padding = dynamic_cast<FLAC::Metadata::Padding *>(our_metadata_.blocks[index]);
	FLAC__ASSERT(0 != padding);
	padding->set_length((unsigned)((int)padding->get_length() + delta));
}

/* function for comparing our metadata to a FLAC::Metadata::Chain */

static bool compare_chain_(FLAC::Metadata::Chain &chain, unsigned current_position, FLAC::Metadata::Prototype *current_block)
{
	unsigned i;
	FLAC::Metadata::Iterator iterator;
	FLAC::Metadata::Prototype *block;
	bool next_ok = true;

	printf("\tcomparing chain... ");
	fflush(stdout);

	if(!iterator.is_valid())
		return die_("allocating memory for iterator");

	iterator.init(chain);

	i = 0;
	do {
		printf("%u... ", i);
		fflush(stdout);

		if(0 == (block = iterator.get_block()))
			return die_("getting block from iterator");

		if(*block != *our_metadata_.blocks[i])
			return die_("metadata block mismatch");

		i++;
		next_ok = iterator.next();
	} while(i < our_metadata_.num_blocks && next_ok);

	if(next_ok)
		return die_("chain has more blocks than expected");

	if(i < our_metadata_.num_blocks)
		return die_("short block count in chain");

	if(0 != current_block) {
		printf("CURRENT_POSITION... ");
		fflush(stdout);

		if(*current_block != *our_metadata_.blocks[current_position])
			return die_("metadata block mismatch");
	}

	printf("PASSED\n");

	return true;
}

::FLAC__StreamDecoderWriteStatus OurFileDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
	(void)buffer;

	if(
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER && frame->header.number.frame_number == 0) ||
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER && frame->header.number.sample_number == 0)
	) {
		printf("content... ");
		fflush(stdout);
	}

	return ::FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void OurFileDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	/* don't bother checking if we've already hit an error */
	if(error_occurred_)
		return;

	printf("%d... ", mc_our_block_number_);
	fflush(stdout);

	if(!ignore_metadata_) {
		if(mc_our_block_number_ >= our_metadata_.num_blocks) {
			(void)die_("got more metadata blocks than expected");
			error_occurred_ = true;
		}
		else {
			if(*our_metadata_.blocks[mc_our_block_number_] != metadata) {
				(void)die_("metadata block mismatch");
				error_occurred_ = true;
			}
		}
	}

	mc_our_block_number_++;
}

void OurFileDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	error_occurred_ = true;
	printf("ERROR: got error callback, status = %s (%u)\n", FLAC__StreamDecoderErrorStatusString[status], (unsigned)status);
}

static bool generate_file_()
{
	::FLAC__StreamMetadata streaminfo, vorbiscomment, padding;
	::FLAC__StreamMetadata *metadata[1];

	printf("generating FLAC file for test\n");

	while(our_metadata_.num_blocks > 0)
		delete_from_our_metadata_(0);

	streaminfo.is_last = false;
	streaminfo.type = ::FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo.length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo.data.stream_info.min_blocksize = 576;
	streaminfo.data.stream_info.max_blocksize = 576;
	streaminfo.data.stream_info.min_framesize = 0;
	streaminfo.data.stream_info.max_framesize = 0;
	streaminfo.data.stream_info.sample_rate = 44100;
	streaminfo.data.stream_info.channels = 1;
	streaminfo.data.stream_info.bits_per_sample = 8;
	streaminfo.data.stream_info.total_samples = 0;
	memset(streaminfo.data.stream_info.md5sum, 0, 16);

	{
		const unsigned vendor_string_length = (unsigned)strlen((const char*)FLAC__VENDOR_STRING);
		vorbiscomment.is_last = false;
		vorbiscomment.type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
		vorbiscomment.length = (4 + vendor_string_length) + 4;
		vorbiscomment.data.vorbis_comment.vendor_string.length = vendor_string_length;
		vorbiscomment.data.vorbis_comment.vendor_string.entry = (FLAC__byte*)malloc_or_die_(vendor_string_length);
		memcpy(vorbiscomment.data.vorbis_comment.vendor_string.entry, FLAC__VENDOR_STRING, vendor_string_length);
		vorbiscomment.data.vorbis_comment.num_comments = 0;
		vorbiscomment.data.vorbis_comment.comments = 0;
	}

	padding.is_last = true;
	padding.type = ::FLAC__METADATA_TYPE_PADDING;
	padding.length = 1234;

	metadata[0] = &padding;

	FLAC::Metadata::StreamInfo s(&streaminfo);
	FLAC::Metadata::VorbisComment v(&vorbiscomment);
	FLAC::Metadata::Padding p(&padding);
	if(
		!insert_to_our_metadata_(&s, 0, /*copy=*/true) ||
		!insert_to_our_metadata_(&v, 1, /*copy=*/true) ||
		!insert_to_our_metadata_(&p, 2, /*copy=*/true)
	)
		return die_("priming our metadata");

	if(!file_utils__generate_flacfile(flacfile_, 0, 512 * 1024, &streaminfo, metadata, 1))
		return die_("creating the encoded file"); 

	free(vorbiscomment.data.vorbis_comment.vendor_string.entry);

	return true;
}

static bool test_file_(const char *filename, bool ignore_metadata)
{
	OurFileDecoder decoder(ignore_metadata);

	FLAC__ASSERT(0 != filename);

	mc_our_block_number_ = 0;
	decoder.error_occurred_ = false;

	printf("\ttesting '%s'... ", filename);
	fflush(stdout);

	if(!decoder.is_valid())
		return die_("couldn't allocate decoder instance");

	decoder.set_md5_checking(true);
	decoder.set_filename(filename);
	decoder.set_metadata_respond_all();
	if(decoder.init() != ::FLAC__FILE_DECODER_OK) {
		decoder.finish();
		return die_("initializing decoder\n");
	}
	if(!decoder.process_until_end_of_file()) {
		decoder.finish();
		return die_("decoding file\n");
	}

	decoder.finish();

	if(decoder.error_occurred_)
		return false;

	if(mc_our_block_number_ != our_metadata_.num_blocks)
		return die_("short metadata block count");

	printf("PASSED\n");
	return true;
}

static bool change_stats_(const char *filename, bool read_only)
{
	if(!file_utils__change_stats(filename, read_only))
        return die_("during file_utils__change_stats()");

	return true;
}

static bool remove_file_(const char *filename)
{
	while(our_metadata_.num_blocks > 0)
		delete_from_our_metadata_(0);

	if(!file_utils__remove_file(filename))
		return die_("removing file");

	return true;
}

static bool test_level_0_()
{
	FLAC::Metadata::StreamInfo streaminfo;

	printf("\n\n++++++ testing level 0 interface\n");

	if(!generate_file_())
		return false;

	if(!test_file_(flacfile_, /*ignore_metadata=*/true))
		return false;

	if(!FLAC::Metadata::get_streaminfo(flacfile_, streaminfo))
		return die_("during FLAC::Metadata::get_streaminfo()");

	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(streaminfo.get_channels() != 1)
		return die_("mismatch in streaminfo.get_channels()");
	if(streaminfo.get_bits_per_sample() != 8)
		return die_("mismatch in streaminfo.get_bits_per_sample()");
	if(streaminfo.get_sample_rate() != 44100)
		return die_("mismatch in streaminfo.get_sample_rate()");
	if(streaminfo.get_min_blocksize() != 576)
		return die_("mismatch in streaminfo.get_min_blocksize()");
	if(streaminfo.get_max_blocksize() != 576)
		return die_("mismatch in streaminfo.get_max_blocksize()");

	if(!remove_file_(flacfile_))
		return false;

	return true;
}

static bool test_level_1_()
{
	FLAC::Metadata::Prototype *block;
	FLAC::Metadata::StreamInfo *streaminfo;
	FLAC::Metadata::Padding *padding;
	FLAC::Metadata::Application *app;
	FLAC__byte data[1000];
	unsigned our_current_position = 0;

	printf("\n\n++++++ testing level 1 interface\n");

	/************************************************************/
	{
	printf("simple iterator on read-only file\n");

	if(!generate_file_())
		return false;

	if(!change_stats_(flacfile_, /*read_only=*/true))
		return false;

	if(!test_file_(flacfile_, /*ignore_metadata=*/true))
		return false;

	FLAC::Metadata::SimpleIterator iterator;

	if(!iterator.is_valid())
		return die_("iterator.is_valid() returned false");

	if(!iterator.init(flacfile_, false))
		return die_("iterator.init() returned false");

	printf("is writable = %u\n", (unsigned)iterator.is_writable());
	if(iterator.is_writable())
		return die_("iterator claims file is writable when tester thinks it should not be; are you running as root?\n");

	printf("iterate forwards\n");

	if(iterator.get_block_type() != ::FLAC__METADATA_TYPE_STREAMINFO)
		return die_("expected STREAMINFO type from iterator.get_block_type()");
	if(0 == (block = iterator.get_block()))
		return die_("getting block 0");
	if(block->get_type() != ::FLAC__METADATA_TYPE_STREAMINFO)
		return die_("expected STREAMINFO type");
	if(block->get_is_last())
		return die_("expected is_last to be false");
	if(block->get_length() != FLAC__STREAM_METADATA_STREAMINFO_LENGTH)
		return die_("bad STREAMINFO length");
	/* check to see if some basic data matches (c.f. generate_file_()) */
	streaminfo = dynamic_cast<FLAC::Metadata::StreamInfo *>(block);
	FLAC__ASSERT(0 != streaminfo);
	if(streaminfo->get_channels() != 1)
		return die_("mismatch in channels");
	if(streaminfo->get_bits_per_sample() != 8)
		return die_("mismatch in bits_per_sample");
	if(streaminfo->get_sample_rate() != 44100)
		return die_("mismatch in sample_rate");
	if(streaminfo->get_min_blocksize() != 576)
		return die_("mismatch in min_blocksize");
	if(streaminfo->get_max_blocksize() != 576)
		return die_("mismatch in max_blocksize");

	if(!iterator.next())
		return die_("forward iterator ended early");
	our_current_position++;

	if(!iterator.next())
		return die_("forward iterator ended early");
	our_current_position++;

	if(iterator.get_block_type() != ::FLAC__METADATA_TYPE_PADDING)
		return die_("expected PADDING type from iterator.get_block_type()");
	if(0 == (block = iterator.get_block()))
		return die_("getting block 1");
	if(block->get_type() != ::FLAC__METADATA_TYPE_PADDING)
		return die_("expected PADDING type");
	if(!block->get_is_last())
		return die_("expected is_last to be true");
	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(block->get_length() != 1234)
		return die_("bad PADDING length");

	if(iterator.next())
		return die_("forward iterator returned true but should have returned false");

	printf("iterate backwards\n");
	if(!iterator.prev())
		return die_("reverse iterator ended early");
	if(!iterator.prev())
		return die_("reverse iterator ended early");
	if(iterator.prev())
		return die_("reverse iterator returned true but should have returned false");

	printf("testing iterator.set_block() on read-only file...\n");

	if(!iterator.set_block(streaminfo, false))
		printf("PASSED.  iterator.set_block() returned false like it should\n");
	else
		return die_("iterator.set_block() returned true but shouldn't have");
	}

	/************************************************************/
	{
	printf("simple iterator on writable file\n");

	if(!change_stats_(flacfile_, /*read-only=*/false))
		return false;

	printf("creating APPLICATION block\n");

	if(0 == (app = new FLAC::Metadata::Application()))
		return die_("new FLAC::Metadata::Application()");
	app->set_id((const unsigned char *)"duh");

	printf("creating PADDING block\n");

	if(0 == (padding = new FLAC::Metadata::Padding()))
		return die_("new FLAC::Metadata::Padding()");
	padding->set_length(20);

	FLAC::Metadata::SimpleIterator iterator;

	if(!iterator.is_valid())
		return die_("iterator.is_valid() returned false");

	if(!iterator.init(flacfile_, /*preserve_file_stats=*/false))
		return die_("iterator.init() returned false");
	our_current_position = 0;

	printf("is writable = %u\n", (unsigned)iterator.is_writable());

	printf("[S]VP\ttry to write over STREAMINFO block...\n");
	if(!iterator.set_block(app, false))
		printf("\titerator.set_block() returned false like it should\n");
	else
		return die_("iterator.set_block() returned true but shouldn't have");

	printf("[S]VP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\tinsert PADDING after, don't expand into padding\n");
	padding->set_length(25);
	if(!iterator.insert_block_after(padding, false))
		return die_ss_("iterator.insert_block_after(padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	printf("SVP[P]\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SV[P]P\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]PP\tinsert PADDING after, don't expand into padding\n");
	padding->set_length(30);
	if(!iterator.insert_block_after(padding, false))
		return die_ss_("iterator.insert_block_after(padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;
	
	printf("SV[P]PP\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]PPP\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]VPPP\tdelete (STREAMINFO block), must fail\n");
	if(iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false) should have returned false", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("[S]VPPP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]PPP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]PP\tdelete (middle block), replace with padding\n");
	if(!iterator.delete_block(true))
		return die_ss_("iterator.delete_block(true)", iterator);
	our_current_position--;

	printf("S[V]PPP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]PP\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("S[V]PP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVP[P]\tdelete (last block), replace with padding\n");
	if(!iterator.delete_block(true))
		return die_ss_("iterator.delete_block(false)", iterator);
	our_current_position--;

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[P]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVP[P]\tdelete (last block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[P]\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]P\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]VP\tset STREAMINFO (change sample rate)\n");
	FLAC__ASSERT(our_current_position == 0);
	block = iterator.get_block();
	streaminfo = dynamic_cast<FLAC::Metadata::StreamInfo *>(block);
	FLAC__ASSERT(0 != streaminfo);
	streaminfo->set_sample_rate(32000);
	if(!replace_in_our_metadata_(block, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(block, false))
		return die_ss_("iterator.set_block(block, false)", iterator);
	delete block;

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("[S]VP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]P\tinsert APPLICATION after, expand into padding of exceeding size\n");
	app->set_id((const unsigned char *)"euh"); /* twiddle the id so that our comparison doesn't miss transposition */
	if(!iterator.insert_block_after(app, true))
		return die_ss_("iterator.insert_block_after(app, true)", iterator);
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return false;
	add_to_padding_length_(our_current_position+1, -((int)(FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8) + (int)app->get_length()));

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVA[P]\tset APPLICATION, expand into padding of exceeding size\n");
	app->set_id((const unsigned char *)"fuh"); /* twiddle the id */
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);
	if(!insert_to_our_metadata_(app, our_current_position, /*copy=*/true))
		return false;
	add_to_padding_length_(our_current_position+1, -((int)(FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8) + (int)app->get_length()));

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVA[A]P\tset APPLICATION (grow), don't expand into padding\n");
	app->set_id((const unsigned char *)"guh"); /* twiddle the id */
	if(!app->set_data(data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app, false))
		return die_ss_("iterator.set_block(app, false)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVA[A]P\tset APPLICATION (shrink), don't fill in with padding\n");
	app->set_id((const unsigned char *)"huh"); /* twiddle the id */
	if(!app->set_data(data, 12, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app, false))
		return die_ss_("iterator.set_block(app, false)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVA[A]P\tset APPLICATION (grow), expand into padding of exceeding size\n");
	app->set_id((const unsigned char *)"iuh"); /* twiddle the id */
	if(!app->set_data(data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	add_to_padding_length_(our_current_position+1, -((int)sizeof(data) - 12));
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVA[A]P\tset APPLICATION (shrink), fill in with padding\n");
	app->set_id((const unsigned char *)"juh"); /* twiddle the id */
	if(!app->set_data(data, 23, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!insert_to_our_metadata_(padding, our_current_position+1, /*copy=*/true))
		return die_("copying object");
	dynamic_cast<FLAC::Metadata::Padding *>(our_metadata_.blocks[our_current_position+1])->set_length(sizeof(data) - 23 - FLAC__STREAM_METADATA_HEADER_LENGTH);
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVA[A]PP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVAA[P]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVAAP[P]\tset PADDING (shrink), don't fill in with padding\n");
	padding->set_length(5);
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(padding, false))
		return die_ss_("iterator.set_block(padding, false)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVAAP[P]\tset APPLICATION (grow)\n");
	app->set_id((const unsigned char *)"kuh"); /* twiddle the id */
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app, false))
		return die_ss_("iterator.set_block(app, false)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVAAP[A]\tset PADDING (equal)\n");
	padding->set_length(27);
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(padding, false))
		return die_ss_("iterator.set_block(padding, false)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVAAP[P]\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SVAA[P]P\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVA[A]P\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVA[P]\tinsert PADDING after\n");
	padding->set_length(5);
	if(!iterator.insert_block_after(padding, false))
		return die_ss_("iterator.insert_block_after(padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVAP[P]\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SVA[P]P\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SV[A]PP\tset APPLICATION (grow), try to expand into padding which is too small\n");
	if(!app->set_data(data, 32, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]PP\tset APPLICATION (grow), try to expand into padding which is 'close' but still too small\n");
	if(!app->set_data(data, 60, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]PP\tset APPLICATION (grow), expand into padding which will leave 0-length pad\n");
	if(!app->set_data(data, 87, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	dynamic_cast<FLAC::Metadata::Padding *>(our_metadata_.blocks[our_current_position+1])->set_length(0);
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]PP\tset APPLICATION (grow), expand into padding which is exactly consumed\n");
	if(!app->set_data(data, 91, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]P\tset APPLICATION (grow), expand into padding which is exactly consumed\n");
	if(!app->set_data(data, 100, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	our_metadata_.blocks[our_current_position]->set_is_last(true);
	if(!iterator.set_block(app, true))
		return die_ss_("iterator.set_block(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]\tset PADDING (equal size)\n");
	padding->set_length(app->get_length());
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(padding, true))
		return die_ss_("iterator.set_block(padding, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[P]\tinsert PADDING after\n");
	if(!iterator.insert_block_after(padding, false))
		return die_ss_("iterator.insert_block_after(padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVP[P]\tinsert PADDING after\n");
	padding->set_length(5);
	if(!iterator.insert_block_after(padding, false))
		return die_ss_("iterator.insert_block_after(padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVPP[P]\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SVP[P]P\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SV[P]PP\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]PPP\tinsert APPLICATION after, try to expand into padding which is too small\n");
	if(!app->set_data(data, 101, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.insert_block_after(app, true))
		return die_ss_("iterator.insert_block_after(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]PPP\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("S[V]PPP\tinsert APPLICATION after, try to expand into padding which is 'close' but still too small\n");
	if(!app->set_data(data, 97, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.insert_block_after(app, true))
		return die_ss_("iterator.insert_block_after(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]PPP\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("S[V]PPP\tinsert APPLICATION after, expand into padding which is exactly consumed\n");
	if(!app->set_data(data, 100, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!iterator.insert_block_after(app, true))
		return die_ss_("iterator.insert_block_after(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]PP\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("S[V]PP\tinsert APPLICATION after, expand into padding which will leave 0-length pad\n");
	if(!app->set_data(data, 96, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	dynamic_cast<FLAC::Metadata::Padding *>(our_metadata_.blocks[our_current_position+1])->set_length(0);
	if(!iterator.insert_block_after(app, true))
		return die_ss_("iterator.insert_block_after(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]PP\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("S[V]PP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]P\tdelete (middle block), don't replace with padding\n");
	if(!iterator.delete_block(false))
		return die_ss_("iterator.delete_block(false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("S[V]P\tinsert APPLICATION after, expand into padding which is exactly consumed\n");
	if(!app->set_data(data, 1, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!iterator.insert_block_after(app, true))
		return die_ss_("iterator.insert_block_after(app, true)", iterator);

	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;
	}

	delete app;
	delete padding;

	if(!remove_file_(flacfile_))
		return false;

	return true;
}

static bool test_level_2_()
{
	FLAC::Metadata::Prototype *block;
	FLAC::Metadata::StreamInfo *streaminfo;
	FLAC::Metadata::Application *app;
	FLAC::Metadata::Padding *padding;
	FLAC__byte data[2000];
	unsigned our_current_position;

	printf("\n\n++++++ testing level 2 interface\n");

	printf("generate read-only file\n");

	if(!generate_file_())
		return false;

	if(!change_stats_(flacfile_, /*read_only=*/true))
		return false;

	printf("create chain\n");
	FLAC::Metadata::Chain chain;
	if(!chain.is_valid())
		return die_("allocating memory for chain");

	printf("read chain\n");

	if(!chain.read(flacfile_))
		return die_c_("reading chain", chain.status());

	printf("[S]VP\ttest initial metadata\n");

	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("switch file to read-write\n");

	if(!change_stats_(flacfile_, /*read-only=*/false))
		return false;

	printf("create iterator\n");
	{
	FLAC::Metadata::Iterator iterator;
	if(!iterator.is_valid())
		return die_("allocating memory for iterator");

	our_current_position = 0;

	iterator.init(chain);

	if(0 == (block = iterator.get_block()))
		return die_("getting block from iterator");

	FLAC__ASSERT(block->get_type() == FLAC__METADATA_TYPE_STREAMINFO);

	printf("[S]VP\tmodify STREAMINFO, write\n");

	streaminfo = dynamic_cast<FLAC::Metadata::StreamInfo *>(block);
	FLAC__ASSERT(0 != streaminfo);
	streaminfo->set_sample_rate(32000);
	if(!replace_in_our_metadata_(block, our_current_position, /*copy=*/true))
		return die_("copying object");

	if(!chain.write(/*use_padding=*/false, /*preserve_file_stats=*/true))
		return die_c_("during chain.write(false, true)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("[S]VP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\treplace PADDING with identical-size APPLICATION\n");
	if(0 == (block = iterator.get_block()))
		return die_("getting block from iterator");
	if(0 == (app = new FLAC::Metadata::Application()))
		return die_("new FLAC::Metadata::Application()");
	app->set_id((const unsigned char *)"duh");
	if(!app->set_data(data, block->get_length()-(FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(false, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]\tshrink APPLICATION, don't use padding\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 26, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(false, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]\tgrow APPLICATION, don't use padding\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 28, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(false, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]\tgrow APPLICATION, use padding, but last block is not padding\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 36, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(false, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]\tshrink APPLICATION, use padding, last block is not padding, but delta is too small for new PADDING block\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 33, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]\tshrink APPLICATION, use padding, last block is not padding, delta is enough for new PADDING block\n");
	if(0 == (padding = new FLAC::Metadata::Padding()))
		return die_("creating PADDING block");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 29, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	padding->set_length(0);
	if(!insert_to_our_metadata_(padding, our_current_position+1, /*copy=*/false))
		return die_("internal error");
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]P\tshrink APPLICATION, use padding, last block is padding\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 16, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	dynamic_cast<FLAC::Metadata::Padding *>(our_metadata_.blocks[our_current_position+1])->set_length(13);
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]P\tgrow APPLICATION, use padding, last block is padding, but delta is too small\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 50, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]P\tgrow APPLICATION, use padding, last block is padding of exceeding size\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 56, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	add_to_padding_length_(our_current_position+1, -(56 - 50));
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]P\tgrow APPLICATION, use padding, last block is padding of exact size\n");
	if(0 == (app = dynamic_cast<FLAC::Metadata::Application *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("copying object");
	if(!app->set_data(data, 67, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!iterator.set_block(app))
		return die_c_("iterator.set_block(app)", chain.status());

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV[A]\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]A\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]VA\tinsert PADDING before STREAMINFO (should fail)\n");
	if(0 == (padding = new FLAC::Metadata::Padding()))
		return die_("creating PADDING block");
	padding->set_length(30);
	if(!iterator.insert_block_before(padding))
		printf("\titerator.insert_block_before() returned false like it should\n");
	else
		return die_("iterator.insert_block_before() should have returned false");

	printf("[S]VA\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]A\tinsert PADDING after\n");
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!iterator.insert_block_after(padding))
		return die_("iterator.insert_block_after(padding)");

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("SV[P]A\tinsert PADDING before\n");
	if(0 == (padding = dynamic_cast<FLAC::Metadata::Padding *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("creating PADDING block");
	padding->set_length(17);
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!iterator.insert_block_before(padding))
		return die_("iterator.insert_block_before(padding)");

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("SV[P]PA\tinsert PADDING before\n");
	if(0 == (padding = dynamic_cast<FLAC::Metadata::Padding *>(FLAC::Metadata::clone(our_metadata_.blocks[our_current_position]))))
		return die_("creating PADDING block");
	padding->set_length(0);
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!iterator.insert_block_before(padding))
		return die_("iterator.insert_block_before(padding)");

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("SV[P]PPA\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVP[P]PA\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVPP[P]A\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVPPP[A]\tinsert PADDING after\n");
	if(0 == (padding = dynamic_cast<FLAC::Metadata::Padding *>(FLAC::Metadata::clone(our_metadata_.blocks[2]))))
		return die_("creating PADDING block");
	padding->set_length(57);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!iterator.insert_block_after(padding))
		return die_("iterator.insert_block_after(padding)");

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("SVPPPA[P]\tinsert PADDING before\n");
	if(0 == (padding = dynamic_cast<FLAC::Metadata::Padding *>(FLAC::Metadata::clone(our_metadata_.blocks[2]))))
		return die_("creating PADDING block");
	padding->set_length(99);
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!iterator.insert_block_before(padding))
		return die_("iterator.insert_block_before(padding)");

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	}
	our_current_position = 0;

	printf("SVPPPAPP\tmerge padding\n");
	chain.merge_padding();
	add_to_padding_length_(2, FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[3]->get_length());
	add_to_padding_length_(2, FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[4]->get_length());
	add_to_padding_length_(6, FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[7]->get_length());
	delete_from_our_metadata_(7);
	delete_from_our_metadata_(4);
	delete_from_our_metadata_(3);

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SVPAP\tsort padding\n");
	chain.sort_padding();
	add_to_padding_length_(4, FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[2]->get_length());
	delete_from_our_metadata_(2);

	if(!chain.write(/*use_padding=*/true, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(true, false)", chain.status());
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("create iterator\n");
	{
	FLAC::Metadata::Iterator iterator;
	if(!iterator.is_valid())
		return die_("allocating memory for iterator");

	our_current_position = 0;

	iterator.init(chain);

	printf("[S]VAP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]AP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[A]P\tdelete middle block, replace with padding\n");
	if(0 == (padding = new FLAC::Metadata::Padding()))
		return die_("creating PADDING block");
	padding->set_length(71);
	if(!replace_in_our_metadata_(padding, our_current_position--, /*copy=*/false))
		return die_("copying object");
	if(!iterator.delete_block(/*replace_with_padding=*/true))
		return die_c_("iterator.delete_block(true)", chain.status());

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("S[V]PP\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]P\tdelete middle block, don't replace with padding\n");
	delete_from_our_metadata_(our_current_position--);
	if(!iterator.delete_block(/*replace_with_padding=*/false))
		return die_c_("iterator.delete_block(false)", chain.status());

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("S[V]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\tdelete last block, replace with padding\n");
	if(0 == (padding = new FLAC::Metadata::Padding()))
		return die_("creating PADDING block");
	padding->set_length(219);
	if(!replace_in_our_metadata_(padding, our_current_position--, /*copy=*/false))
		return die_("copying object");
	if(!iterator.delete_block(/*replace_with_padding=*/true))
		return die_c_("iterator.delete_block(true)", chain.status());

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("S[V]P\tnext\n");
	if(!iterator.next())
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\tdelete last block, don't replace with padding\n");
	delete_from_our_metadata_(our_current_position--);
	if(!iterator.delete_block(/*replace_with_padding=*/false))
		return die_c_("iterator.delete_block(false)", chain.status());

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	printf("S[V]\tprev\n");
	if(!iterator.prev())
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]V\tdelete STREAMINFO block, should fail\n");
	if(iterator.delete_block(/*replace_with_padding=*/false))
		return die_("iterator.delete_block() on STREAMINFO should have failed but didn't");

	if(!compare_chain_(chain, our_current_position, iterator.get_block()))
		return false;

	}
	our_current_position = 0;

	printf("SV\tmerge padding\n");
	chain.merge_padding();

	if(!chain.write(/*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(false, false)", chain.status());
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	printf("SV\tsort padding\n");
	chain.sort_padding();

	if(!chain.write(/*use_padding=*/false, /*preserve_file_stats=*/false))
		return die_c_("during chain.write(false, false)", chain.status());
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(flacfile_, /*ignore_metadata=*/false))
		return false;

	if(!remove_file_(flacfile_))
		return false;

	return true;
}

bool test_metadata_file_manipulation()
{
	printf("\n+++ libFLAC++ unit test: metadata manipulation\n\n");

	our_metadata_.num_blocks = 0;

	if(!test_level_0_())
		return false;

	if(!test_level_1_())
		return false;

	if(!test_level_2_())
		return false;

	return true;
}
