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

#include "FLAC/assert.h"
#include "FLAC++/metadata.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

static ::FLAC__StreamMetadata streaminfo_, padding_, seektable_, application_, vorbiscomment_;

static bool die_(const char *msg)
{
	printf("FAILED, %s\n", msg);
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

static void init_metadata_blocks_()
{
	streaminfo_.is_last = false;
	streaminfo_.type = ::FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo_.length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo_.data.stream_info.min_blocksize = 576;
	streaminfo_.data.stream_info.max_blocksize = 576;
	streaminfo_.data.stream_info.min_framesize = 0;
	streaminfo_.data.stream_info.max_framesize = 0;
	streaminfo_.data.stream_info.sample_rate = 44100;
	streaminfo_.data.stream_info.channels = 1;
	streaminfo_.data.stream_info.bits_per_sample = 8;
	streaminfo_.data.stream_info.total_samples = 0;
	memset(streaminfo_.data.stream_info.md5sum, 0, 16);

	padding_.is_last = false;
	padding_.type = ::FLAC__METADATA_TYPE_PADDING;
	padding_.length = 1234;

	seektable_.is_last = false;
	seektable_.type = ::FLAC__METADATA_TYPE_SEEKTABLE;
	seektable_.data.seek_table.num_points = 2;
	seektable_.length = seektable_.data.seek_table.num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	seektable_.data.seek_table.points = (::FLAC__StreamMetadata_SeekPoint*)malloc_or_die_(seektable_.data.seek_table.num_points * sizeof(::FLAC__StreamMetadata_SeekPoint));
	seektable_.data.seek_table.points[0].sample_number = 0;
	seektable_.data.seek_table.points[0].stream_offset = 0;
	seektable_.data.seek_table.points[0].frame_samples = streaminfo_.data.stream_info.min_blocksize;
	seektable_.data.seek_table.points[1].sample_number = ::FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seektable_.data.seek_table.points[1].stream_offset = 1000;
	seektable_.data.seek_table.points[1].frame_samples = streaminfo_.data.stream_info.min_blocksize;

	application_.is_last = false;
	application_.type = ::FLAC__METADATA_TYPE_APPLICATION;
	application_.length = 8;
	memcpy(application_.data.application.id, "\xfe\xdc\xba\x98", 4);
	application_.data.application.data = (FLAC__byte*)malloc_or_die_(4);
	memcpy(application_.data.application.data, "\xf0\xe1\xd2\xc3", 4);

	vorbiscomment_.is_last = true;
	vorbiscomment_.type = ::FLAC__METADATA_TYPE_VORBIS_COMMENT;
	vorbiscomment_.length = (4 + 5) + 4 + (4 + 12) + (4 + 12);
	vorbiscomment_.data.vorbis_comment.vendor_string.length = 5;
	vorbiscomment_.data.vorbis_comment.vendor_string.entry = (FLAC__byte*)malloc_or_die_(5);
	memcpy(vorbiscomment_.data.vorbis_comment.vendor_string.entry, "name0", 5);
	vorbiscomment_.data.vorbis_comment.num_comments = 2;
	vorbiscomment_.data.vorbis_comment.comments = (::FLAC__StreamMetadata_VorbisComment_Entry*)malloc_or_die_(vorbiscomment_.data.vorbis_comment.num_comments * sizeof(::FLAC__StreamMetadata_VorbisComment_Entry));
	vorbiscomment_.data.vorbis_comment.comments[0].length = 12;
	vorbiscomment_.data.vorbis_comment.comments[0].entry = (FLAC__byte*)malloc_or_die_(12);
	memcpy(vorbiscomment_.data.vorbis_comment.comments[0].entry, "name2=value2", 12);
	vorbiscomment_.data.vorbis_comment.comments[1].length = 12;
	vorbiscomment_.data.vorbis_comment.comments[1].entry = (FLAC__byte*)malloc_or_die_(12);
	memcpy(vorbiscomment_.data.vorbis_comment.comments[1].entry, "name3=value3", 12);
}

static void free_metadata_blocks_()
{
	free(seektable_.data.seek_table.points);
	free(application_.data.application.data);
	free(vorbiscomment_.data.vorbis_comment.vendor_string.entry);
	free(vorbiscomment_.data.vorbis_comment.comments[0].entry);
	free(vorbiscomment_.data.vorbis_comment.comments);
}

bool test_metadata_object_streaminfo()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::StreamInfo\n");

	printf("testing StreamInfo::StreamInfo()... ");
	FLAC::Metadata::StreamInfo block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing StreamInfo::StreamInfo(const StreamInfo &)... +\n");
	printf("        StreamInfo::operator!=(const StreamInfo &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing StreamInfo::~StreamInfo()... ");
	}
	printf("OK\n");

	printf("testing StreamInfo::StreamInfo(const ::FLAC__StreamMetadata &)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(streaminfo_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::StreamInfo(const ::FLAC__StreamMetadata *)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(&streaminfo_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::operator=(const StreamInfo &)... +\n");
	printf("        StreamInfo::operator==(const StreamInfo &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        StreamInfo::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy = streaminfo_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == streaminfo_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        StreamInfo::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy = &streaminfo_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == streaminfo_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::set_min_blocksize()... ");
	block.set_min_blocksize(streaminfo_.data.stream_info.min_blocksize);
	printf("OK\n");

	printf("testing StreamInfo::set_max_blocksize()... ");
	block.set_max_blocksize(streaminfo_.data.stream_info.max_blocksize);
	printf("OK\n");

	printf("testing StreamInfo::set_min_framesize()... ");
	block.set_min_framesize(streaminfo_.data.stream_info.min_framesize);
	printf("OK\n");

	printf("testing StreamInfo::set_max_framesize()... ");
	block.set_max_framesize(streaminfo_.data.stream_info.max_framesize);
	printf("OK\n");

	printf("testing StreamInfo::set_sample_rate()... ");
	block.set_sample_rate(streaminfo_.data.stream_info.sample_rate);
	printf("OK\n");

	printf("testing StreamInfo::set_channels()... ");
	block.set_channels(streaminfo_.data.stream_info.channels);
	printf("OK\n");

	printf("testing StreamInfo::set_bits_per_sample()... ");
	block.set_bits_per_sample(streaminfo_.data.stream_info.bits_per_sample);
	printf("OK\n");

	printf("testing StreamInfo::set_total_samples()... ");
	block.set_total_samples(streaminfo_.data.stream_info.total_samples);
	printf("OK\n");

	printf("testing StreamInfo::set_md5sum()... ");
	block.set_md5sum(streaminfo_.data.stream_info.md5sum);
	printf("OK\n");

	printf("testing StreamInfo::get_min_blocksize()... ");
	if(block.get_min_blocksize() != streaminfo_.data.stream_info.min_blocksize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_max_blocksize()... ");
	if(block.get_max_blocksize() != streaminfo_.data.stream_info.max_blocksize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_min_framesize()... ");
	if(block.get_min_framesize() != streaminfo_.data.stream_info.min_framesize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_max_framesize()... ");
	if(block.get_max_framesize() != streaminfo_.data.stream_info.max_framesize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_sample_rate()... ");
	if(block.get_sample_rate() != streaminfo_.data.stream_info.sample_rate)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_channels()... ");
	if(block.get_channels() != streaminfo_.data.stream_info.channels)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_bits_per_sample()... ");
	if(block.get_bits_per_sample() != streaminfo_.data.stream_info.bits_per_sample)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_total_samples()... ");
	if(block.get_total_samples() != streaminfo_.data.stream_info.total_samples)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_md5sum()... ");
	if(0 != memcmp(block.get_md5sum(), streaminfo_.data.stream_info.md5sum, 16))
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::StreamInfo *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::StreamInfo *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_padding()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::Padding\n");

	printf("testing Padding::Padding()... ");
	FLAC::Metadata::Padding block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = 0;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing Padding::Padding(const Padding &)... +\n");
	printf("        Padding::operator!=(const Padding &)... ");
	{
		FLAC::Metadata::Padding blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing Padding::~Padding()... ");
	}
	printf("OK\n");

	printf("testing Padding::Padding(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Padding blockcopy(padding_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::Padding(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Padding blockcopy(&padding_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::operator=(const Padding &)... +\n");
	printf("        Padding::operator==(const Padding &)... ");
	{
		FLAC::Metadata::Padding blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Padding::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Padding blockcopy = padding_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == padding_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Padding::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Padding blockcopy = &padding_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == padding_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::set_length()... ");
	block.set_length(padding_.length);
	printf("OK\n");

	printf("testing Prototype::get_length()... ");
	if(block.get_length() != padding_.length)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::Padding *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::Padding *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_application()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::Application\n");

	printf("testing Application::Application()... ");
	FLAC::Metadata::Application block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing Application::Application(const Application &)... +\n");
	printf("        Application::operator!=(const Application &)... ");
	{
		FLAC::Metadata::Application blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing Application::~Application()... ");
	}
	printf("OK\n");

	printf("testing Application::Application(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Application blockcopy(application_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::Application(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Application blockcopy(&application_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::operator=(const Application &)... +\n");
	printf("        Application::operator==(const Application &)... ");
	{
		FLAC::Metadata::Application blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Application::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Application blockcopy = application_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == application_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Application::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Application blockcopy = &application_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == application_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::set_id()... ");
	block.set_id(application_.data.application.id);
	printf("OK\n");

	printf("testing Application::set_data()... ");
	block.set_data(application_.data.application.data, application_.length - sizeof(application_.data.application.id), /*copy=*/true);
	printf("OK\n");

	printf("testing Application::get_id()... ");
	if(0 != memcmp(block.get_id(), application_.data.application.id, sizeof(application_.data.application.id)))
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing Application::get_data()... ");
	if(0 != memcmp(block.get_data(), application_.data.application.data, application_.length - sizeof(application_.data.application.id)))
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::Application *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::Application *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_seektable()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::SeekTable\n");

	printf("testing SeekTable::SeekTable()... ");
	FLAC::Metadata::SeekTable block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = 0;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing SeekTable::SeekTable(const SeekTable &)... +\n");
	printf("        SeekTable::operator!=(const SeekTable &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing SeekTable::~SeekTable()... ");
	}
	printf("OK\n");

	printf("testing SeekTable::SeekTable(const ::FLAC__StreamMetadata &)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy(seektable_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::SeekTable(const ::FLAC__StreamMetadata *)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::SeekTable blockcopy(&seektable_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::operator=(const SeekTable &)... +\n");
	printf("        SeekTable::operator==(const SeekTable &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        SeekTable::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy = seektable_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == seektable_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        SeekTable::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::SeekTable blockcopy = &seektable_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == seektable_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::insert_point() x 3... ");
	if(!block.insert_point(0, seektable_.data.seek_table.points[1]))
		return die_("returned false");
	if(!block.insert_point(0, seektable_.data.seek_table.points[1]))
		return die_("returned false");
	if(!block.insert_point(1, seektable_.data.seek_table.points[0]))
		return die_("returned false");
	printf("OK\n");

	printf("testing SeekTable::is_legal()... ");
	if(block.is_legal())
		return die_("returned true");
	printf("OK\n");

	printf("testing SeekTable::set_point()... ");
	block.set_point(0, seektable_.data.seek_table.points[0]);
	printf("OK\n");

	printf("testing SeekTable::delete_point()... ");
	if(!block.delete_point(0))
		return die_("returned false");
	printf("OK\n");

	printf("testing SeekTable::is_legal()... ");
	if(!block.is_legal())
		return die_("returned false");
	printf("OK\n");

	printf("testing SeekTable::get_num_points()... ");
	if(block.get_num_points() != seektable_.data.seek_table.num_points)
		return die_("number mismatch");
	printf("OK\n");

	printf("testing SeekTable::operator!=(const ::FLAC__StreamMetadata &)... ");
	if(block != seektable_)
		return die_("data mismatch");
	printf("OK\n");

	printf("testing SeekTable::get_point()... ");
	if(
		block.get_point(1).sample_number != seektable_.data.seek_table.points[1].sample_number ||
		block.get_point(1).stream_offset != seektable_.data.seek_table.points[1].stream_offset ||
		block.get_point(1).frame_samples != seektable_.data.seek_table.points[1].frame_samples
	)
		return die_("point mismatch");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::SeekTable *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::SeekTable *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_vorbiscomment()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::VorbisComment::Entry\n");

	printf("testing Entry::Entry()... ");
	{
		FLAC::Metadata::VorbisComment::Entry entry1;
		if(!entry1.is_valid())
			return die_("!is_valid()");
		printf("OK\n");

		printf("testing Entry::~Entry()... ");
	}
	printf("OK\n");

	printf("testing Entry::Entry(const char *field, unsigned field_length)... ");
	FLAC::Metadata::VorbisComment::Entry entry2("name2=value2", strlen("name2=value2"));
	if(!entry2.is_valid())
		return die_("!is_valid()");
	printf("OK\n");

	printf("testing Entry::Entry(const char *field_name, const char *field_value, unsigned field_value_length)... ");
	FLAC::Metadata::VorbisComment::Entry entry3("name3", "value3", strlen("value3"));
	if(!entry3.is_valid())
		return die_("!is_valid()");
	printf("OK\n");

	printf("testing Entry::Entry(const Entry &entry)... ");
	{
		FLAC::Metadata::VorbisComment::Entry entry2copy(entry2);
		if(!entry2copy.is_valid())
			return die_("!is_valid()");
		printf("OK\n");

		printf("testing Entry::~Entry()... ");
	}
	printf("OK\n");

	printf("testing Entry::operator=(const Entry &entry)... ");
	FLAC::Metadata::VorbisComment::Entry entry1 = entry2;
	if(!entry2.is_valid())
		return die_("!is_valid()");
	printf("OK\n");

	printf("testing Entry::get_field_length()... ");
	if(entry1.get_field_length() != strlen("name2=value2"))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_name_length()... ");
	if(entry1.get_field_name_length() != strlen("name2"))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_value_length()... ");
	if(entry1.get_field_value_length() != strlen("value2"))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_entry()... ");
	{
		::FLAC__StreamMetadata_VorbisComment_Entry entry = entry1.get_entry();
		if(entry.length != strlen("name2=value2"))
			return die_("entry length mismatch");
		if(0 != memcmp(entry.entry, "name2=value2", entry.length))
			return die_("entry value mismatch");
	}
	printf("OK\n");

	printf("testing Entry::get_field()... ");
	if(0 != memcmp(entry1.get_field(), "name2=value2", strlen("name2=value2")))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_name()... ");
	if(0 != memcmp(entry1.get_field_name(), "name2", strlen("name2")))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_value()... ");
	if(0 != memcmp(entry1.get_field_value(), "value2", strlen("value2")))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::set_field_name()... ");
	if(!entry1.set_field_name("name1"))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_name(), "name1", strlen("name1")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name1=value2", strlen("name1=value2")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("testing Entry::set_field_value()... ");
	if(!entry1.set_field_value("value1", strlen("value1")))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_value(), "value1", strlen("value1")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name1=value1", strlen("name1=value1")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("testing Entry::set_field()... ");
	if(!entry1.set_field("name0=value0", strlen("name0=value0")))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_name(), "name0", strlen("name0")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field_value(), "value0", strlen("value0")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name0=value0", strlen("name0=value0")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("PASSED\n\n");


	printf("testing class FLAC::Metadata::VorbisComment\n");

	printf("testing VorbisComment::VorbisComment()... ");
	FLAC::Metadata::VorbisComment block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN + FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN) / 8;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing VorbisComment::VorbisComment(const VorbisComment &)... +\n");
	printf("        VorbisComment::operator!=(const VorbisComment &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing VorbisComment::~VorbisComment()... ");
	}
	printf("OK\n");

	printf("testing VorbisComment::VorbisComment(const ::FLAC__StreamMetadata &)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy(vorbiscomment_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::VorbisComment(const ::FLAC__StreamMetadata *)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy(&vorbiscomment_);
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::operator=(const VorbisComment &)... +\n");
	printf("        VorbisComment::operator==(const VorbisComment &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        VorbisComment::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy = vorbiscomment_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == vorbiscomment_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        VorbisComment::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy = &vorbiscomment_;
		if(!blockcopy.is_valid())
			return die_("!block.is_valid()");
		if(!(blockcopy == vorbiscomment_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::get_num_comments()... ");
	if(block.get_num_comments() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing VorbisComment::set_vendor_string()... ");
	if(!block.set_vendor_string(entry1))
		return die_("returned false");
	printf("OK\n");

	printf("testing VorbisComment::get_vendor_string()... ");
	if(block.get_vendor_string().get_field_name_length() != vorbiscomment_.data.vorbis_comment.vendor_string.length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_vendor_string().get_field_name(), vorbiscomment_.data.vorbis_comment.vendor_string.entry, vorbiscomment_.data.vorbis_comment.vendor_string.length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::insert_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.insert_comment(0, entry3))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[1].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[1].entry, vorbiscomment_.data.vorbis_comment.comments[1].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::insert_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.insert_comment(0, entry3))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[1].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[1].entry, vorbiscomment_.data.vorbis_comment.comments[1].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::insert_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.insert_comment(1, entry2))
		return die_("returned false");
	if(block.get_comment(1).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(1).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::set_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.set_comment(0, entry2))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::delete_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.delete_comment(0))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length[0] mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value[0] mismatch");
	if(block.get_comment(1).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[1].length)
		return die_("length[1] mismatch");
	if(0 != memcmp(block.get_comment(1).get_field(), vorbiscomment_.data.vorbis_comment.comments[1].entry, vorbiscomment_.data.vorbis_comment.comments[1].length))
		return die_("value[0] mismatch");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::VorbisComment *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::VorbisComment *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object()
{
	printf("\n+++ libFLAC++ unit test: metadata objects\n\n");

	init_metadata_blocks_();

	if(!test_metadata_object_streaminfo())
		return false;

	if(!test_metadata_object_padding())
		return false;

	if(!test_metadata_object_application())
		return false;

	if(!test_metadata_object_seektable())
		return false;

	if(!test_metadata_object_vorbiscomment())
		return false;

	free_metadata_blocks_();

	return true;
}
