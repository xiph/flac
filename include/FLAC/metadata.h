/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef FLAC__METADATA_H
#define FLAC__METADATA_H

#include "format.h"

/******************************************************************************
	(For an example of how all these routines are used, see the source
	code for the unit tests in src/test_libFLAC/metadata_*.c, or metaflac
	in src/metaflac/)
******************************************************************************/

/******************************************************************************
	This module provides read and write access to FLAC file metadata at
	three increasing levels of complexity:

	level 0:
	read-only access to the STREAMINFO block.

	level 1:
	read-write access to all metadata blocks.  This level is write-
	efficient in most cases (more on this later), and uses less memory
	than level 2.

	level 2:
	read-write access to all metadata blocks.  This level is write-
	efficient in all cases, but uses more memory since all metadata for
	the whole file is read into memory and manipulated before writing
	out again.

	What do we mean by efficient?  When writing metadata back to a FLAC
	file it is possible to grow or shrink the metadata such that the entire
	file must be rewritten.  However, if the size remains the same during
	changes or PADDING blocks are utilized, only the metadata needs to be
	overwritten, which is much faster.

	Efficient means the whole file is rewritten at most one time, and only
	when necessary.  Level 1 is not efficient only in the case that you
	cause more than one metadata block to grow or shrink beyond what can
	be accomodated by padding.  In this case you should probably use level
	2, which allows you to edit all the metadata for a file in memory and
	write it out all at once.

	All levels know how to skip over and not disturb an ID3v2 tag at the
	front of the file.
******************************************************************************/

/******************************************************************************
	After the three interfaces, methods for creating and manipulating
	the various metadata blocks are defined.  Unless you will be using
	level 1 or 2 to modify existing metadata you will not need these.
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
 * level 0
 * ---------------------------------------------------------------------
 * Only one routine to read the STREAMINFO.  Skips any ID3v2 tag at the
 * head of the file.  Useful for file-based player plugins.
 *
 * Provide the address of a FLAC__StreamMetadata_StreamInfo object to
 * fill.
 */

FLAC__bool FLAC__metadata_get_streaminfo(const char *filename, FLAC__StreamMetadata_StreamInfo *streaminfo);


/***********************************************************************
 * level 1
 * ---------------------------------------------------------------------
 * The general usage of this interface is:
 *
 * Create an iterator using FLAC__metadata_simple_iterator_new()
 * Attach it to a file using FLAC__metadata_simple_iterator_init() and check
 *    the exit code.  Call FLAC__metadata_simple_iterator_is_writable() to
 *    see if the file is writable, or read-only access is allowed.
 * Use _next() and _prev() to move around the blocks.  This is does not
 *    read the actual blocks themselves.  _next() is relatively fast.
 *    _prev() is slower since it needs to search forward from the front
 *    of the file.
 * Use _get_block_type() or _get_block() to access the actual data at
 *    the current iterator position.  The returned object is yours to
 *    modify and free.
 * Use _set_block() to write a modified block back.  You must have write
 *    permission to the original file.  Make sure to read the whole
 *    comment to _set_block() below.
 * Use _insert_block_after() to add new blocks.  Use the object creation
 *    functions from the end of this header file to generate new objects.
 * Use _delete_block() to remove the block currently referred to by the
 *    iterator, or replace it with padding.
 * Destroy the iterator with FLAC__metadata_simple_iterator_delete()
 *    when finished.
 *
 * NOTE: The FLAC file remains open the whole time between _init() and
 *       _delete(), so make sure you are not altering the file during
 *       this time.
 *
 * NOTE: Do not modify the is_last, length, or type fields of returned
 *       FLAC__MetadataType objects.  These are managed automatically.
 *
 * NOTE: If any of the modification functions (_set_block, _delete_block,
 *       _insert_block_after, etc) return false, you should delete the
 *       iterator as it may no longer be valid.
 */

/*
 * opaque structure definition
 */
struct FLAC__Metadata_SimpleIterator;
typedef struct FLAC__Metadata_SimpleIterator FLAC__Metadata_SimpleIterator;

typedef enum {
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK = 0,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ILLEGAL_INPUT,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_ERROR_OPENING_FILE,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_WRITABLE,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_BAD_METADATA,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_READ_ERROR,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_SEEK_ERROR,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_WRITE_ERROR,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_RENAME_ERROR,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_UNLINK_ERROR,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR,
	FLAC__METADATA_SIMPLE_ITERATOR_STATUS_INTERNAL_ERROR
} FLAC__Metadata_SimpleIteratorStatus;
extern const char * const FLAC__Metadata_SimpleIteratorStatusString[];

/*
 * Constructor/destructor
 */
FLAC__Metadata_SimpleIterator *FLAC__metadata_simple_iterator_new();
void FLAC__metadata_simple_iterator_delete(FLAC__Metadata_SimpleIterator *iterator);

/*
 * Get the current status of the iterator.  Call this after a function
 * returns false to get the reason for the error.  Also resets the status
 * to FLAC__METADATA_SIMPLE_ITERATOR_STATUS_OK
 */
FLAC__Metadata_SimpleIteratorStatus FLAC__metadata_simple_iterator_status(FLAC__Metadata_SimpleIterator *iterator);

/*
 * Initialize the iterator to point to the first metadata block in the
 * given FLAC file.  If 'preserve_file_stats' is true, the owner and
 * modification time will be preserved even if the FLAC file is written.
 */
FLAC__bool FLAC__metadata_simple_iterator_init(FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool preserve_file_stats);

/*
 * Returns true if the FLAC file is writable.  If false, calls to
 * FLAC__metadata_simple_iterator_set_block() and
 * FLAC__metadata_simple_iterator_insert_block_after() will fail.
 */
FLAC__bool FLAC__metadata_simple_iterator_is_writable(const FLAC__Metadata_SimpleIterator *iterator);

/*
 * These move the iterator forwards or backwards, returning false if
 * already at the end.
 */
FLAC__bool FLAC__metadata_simple_iterator_next(FLAC__Metadata_SimpleIterator *iterator);
FLAC__bool FLAC__metadata_simple_iterator_prev(FLAC__Metadata_SimpleIterator *iterator);

/*
 * Get the type of the metadata block at the current position.  This
 * avoids reading the actual block data which can save time for large
 * blocks.
 */
FLAC__MetadataType FLAC__metadata_simple_iterator_get_block_type(const FLAC__Metadata_SimpleIterator *iterator);

/*
 * Get the metadata block at the current position.  You can modify the
 * block but must use FLAC__metadata_simple_iterator_set_block() to
 * write it back to the FLAC file.
 *
 * You must call FLAC__metadata_object_delete() on the returned object
 * when you are finished with it.
 */
FLAC__StreamMetadata *FLAC__metadata_simple_iterator_get_block(FLAC__Metadata_SimpleIterator *iterator);

/*
 * Write a block back to the FLAC file.  This function tries to be
 * as efficient as possible; how the block is actually written is
 * shown by the following:
 *
 * Existing block is a STREAMINFO block and the new block is a
 * STREAMINFO block: the new block is written in place.  Make sure
 * you know what you're doing when changing the values of a
 * STREAMINFO block.
 *
 * Existing block is a STREAMINFO block and the new block is a
 * not a STREAMINFO block: this is an error since the first block
 * must be a STREAMINFO block.  Returns false without altering the
 * file.
 *
 * Existing block is not a STREAMINFO block and the new block is a
 * STREAMINFO block: this is an error since there may be only one
 * STREAMINFO block.  Returns false without altering the file.
 *
 * Existing block and new block are the same length: the existing
 * block will be replaced by the new block, written in place.
 *
 * Existing block is longer than new block: if use_padding is true,
 * the existing block will be overwritten in place with the new
 * block followed by a PADDING block, if possible, to make the total
 * size the same as the existing block.  Remember that a padding
 * block requires at least four bytes so if the difference in size
 * between the new block and existing block is less than that, the
 * entire file will have to be rewritten, using the new block's
 * exact size.  If use_padding is false, the entire file will be
 * rewritten, replacing the existing block by the new block.
 *
 * Existing block is shorter than new block: if use_padding is true,
 * the function will try and expand the new block into the following
 * PADDING block, if it exists and doing so won't shrink the PADDING
 * block to less than 4 bytes.  If there is no following PADDING
 * block, or it will shrink to less than 4 bytes, or use_padding is
 * false, the entire file is rewritten, replacing the existing block
 * with the new block.  Note that in this case any following PADDING
 * block is preserved as is.
 *
 * After writing the block, the iterator will remain in the same
 * place, i.e. pointing to the new block.
 */
FLAC__bool FLAC__metadata_simple_iterator_set_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding);

/*
 * This is similar to FLAC__metadata_simple_iterator_set_block()
 * except that instead of writing over an existing block, it appends
 * a block after the existing block.  'use_padding' is again used to
 * tell the function to try an expand into following padding in an
 * attempt to avoid rewriting the entire file.
 *
 * This function will fail and return false if given a STREAMINFO
 * block.
 *
 * After writing the block, the iterator will be pointing to the
 * new block.
 */
FLAC__bool FLAC__metadata_simple_iterator_insert_block_after(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding);

/*
 * Deletes the block at the current position.  This will cause the
 * entire FLAC file to be rewritten, unless 'use_padding' is true,
 * in which case the block will be replaced by an equal-sized PADDING
 * block.  The iterator will be left pointing to the block before the
 * one just deleted.
 *
 * You may not delete the STREAMINFO block.
 */
FLAC__bool FLAC__metadata_simple_iterator_delete_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__bool use_padding);


/***********************************************************************
 * level 2
 * ---------------------------------------------------------------------
 * The general usage of this interface is:
 *
 * Create a new chain using FLAC__metadata_chain_new().  A chain is a
 *    linked list of metadata blocks.
 * Read all metadata into the the chain from a FLAC file using
 *    FLAC__metadata_chain_read() and check the status.
 * Optionally, consolidate the padding using
 *    FLAC__metadata_chain_merge_padding() or
 *    FLAC__metadata_chain_sort_padding().
 * Create a new iterator using FLAC__metadata_iterator_new()
 * Initialize the iterator to point to the first element in the chain
 *    using FLAC__metadata_iterator_init()
 * Traverse the chain using FLAC__metadata_iterator_next/prev().
 * Get a block for reading or modification using
 *    FLAC__metadata_iterator_get_block().  The pointer to the object
 *    inside the chain is returned, so the block is yours to modify.
 *    Changes will be reflected in the FLAC file when you write the
 *    chain.  You can also add and delete blocks (see functions below).
 * When done, write out the chain using FLAC__metadata_chain_write().
 *    Make sure to read the whole comment to the function below.
 * Delete the chain using FLAC__metadata_chain_delete().
 *
 * NOTE: Even though the FLAC file is not open while the chain is being
 *       manipulated, you must not alter the file externally during
 *       this time.  The chain assumes the FLAC file will not change
 *       between the time of FLAC__metadata_chain_read() and
 *       FLAC__metadata_chain_write().
 *
 * NOTE: Do not modify the is_last, length, or type fields of returned
 *       FLAC__MetadataType objects.  These are managed automatically.
 *
 * NOTE: The metadata objects returned by _get_bloca()k are owned by the
 *       chain; do not FLAC__metadata_object_delete() them.  In the
 *       same way, blocks passed to _set_block() become owned by the
 *       chain and will be deleted when the chain is deleted.
 */

/*
 * opaque structure definitions
 */
struct FLAC__Metadata_Chain;
typedef struct FLAC__Metadata_Chain FLAC__Metadata_Chain;
struct FLAC__Metadata_Iterator;
typedef struct FLAC__Metadata_Iterator FLAC__Metadata_Iterator;

typedef enum {
	FLAC__METADATA_CHAIN_STATUS_OK = 0,
	FLAC__METADATA_CHAIN_STATUS_ILLEGAL_INPUT,
	FLAC__METADATA_CHAIN_STATUS_ERROR_OPENING_FILE,
	FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE,
	FLAC__METADATA_CHAIN_STATUS_NOT_WRITABLE,
	FLAC__METADATA_CHAIN_STATUS_BAD_METADATA,
	FLAC__METADATA_CHAIN_STATUS_READ_ERROR,
	FLAC__METADATA_CHAIN_STATUS_SEEK_ERROR,
	FLAC__METADATA_CHAIN_STATUS_WRITE_ERROR,
	FLAC__METADATA_CHAIN_STATUS_RENAME_ERROR,
	FLAC__METADATA_CHAIN_STATUS_UNLINK_ERROR,
	FLAC__METADATA_CHAIN_STATUS_MEMORY_ALLOCATION_ERROR,
	FLAC__METADATA_CHAIN_STATUS_INTERNAL_ERROR
} FLAC__Metadata_ChainStatus;
extern const char * const FLAC__Metadata_ChainStatusString[];

/*********** FLAC__Metadata_Chain ***********/

/*
 * Constructor/destructor
 */
FLAC__Metadata_Chain *FLAC__metadata_chain_new();
void FLAC__metadata_chain_delete(FLAC__Metadata_Chain *chain);

/*
 * Get the current status of the chain.  Call this after a function
 * returns false to get the reason for the error.  Also resets the status
 * to FLAC__METADATA_CHAIN_STATUS_OK
 */
FLAC__Metadata_ChainStatus FLAC__metadata_chain_status(FLAC__Metadata_Chain *chain);

/*
 * Read all metadata into the chain
 */
FLAC__bool FLAC__metadata_chain_read(FLAC__Metadata_Chain *chain, const char *filename);

/*
 * Write all metadata out to the FLAC file.  This function tries to be as
 * efficient as possible; how the metadata is actually written is shown by
 * the following:
 *
 * If the current chain is the same size as the existing metadata, the new
 * data is written in place.
 *
 * If the current chain is longer than the existing metadata, and
 * 'use_padding' is true, and the last block is a PADDING block of
 * sufficient length, the function will truncate the final padding block
 * so that the overall size of the metadata is the same as the existing
 * metadata, and then just rewrite the metadata.  Otherwise, if not all of
 * the above conditions are met, the entire FLAC file must be rewritten.
 * If you want to use padding this way it is a good idea to call
 * FLAC__metadata_chain_sort_padding() first so that you have the maximum
 * amount of padding to work with, unless you need to preserve ordering
 * of the PADDING blocks for some reason.
 *
 * If the current chain is shorter than the existing metadata, and
 * use_padding is true, and the final block is a PADDING block, the padding
 * is extended to make the overall size the same as the existing data.  If
 * use_padding is true and the last block is not a PADDING block, a new
 * PADDING block is added to the end of the new data to make it the same
 * size as the existing data (if possible, see the note to
 * FLAC__metadata_simple_iterator_set_block() about the four byte limit)
 * and the new data is written in place.  If none of the above apply or
 * use_padding is false, the entire FLAC file is rewritten.
 *
 * If 'preserve_file_stats' is true, the owner and modification time will
 * be preserved even if the FLAC file is written.
 */
FLAC__bool FLAC__metadata_chain_write(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats);

/*
 * This function will merge adjacent PADDING blocks into a single block.
 *
 * NOTE: this function does not write to the FLAC file, it only
 * modifies the chain.
 *
 * NOTE: Any iterator on the current chain will become invalid after this
 * call.  You should delete the iterator and get a new one.
 */
void FLAC__metadata_chain_merge_padding(FLAC__Metadata_Chain *chain);

/*
 * This function will move all PADDING blocks to the end on the metadata,
 * then merge them into a single block.
 *
 * NOTE: this function does not write to the FLAC file, it only
 * modifies the chain.
 *
 * NOTE: Any iterator on the current chain will become invalid after this
 * call.  You should delete the iterator and get a new one.
 */
void FLAC__metadata_chain_sort_padding(FLAC__Metadata_Chain *chain);


/*********** FLAC__Metadata_Iterator ***********/

/*
 * Constructor/destructor
 */
FLAC__Metadata_Iterator *FLAC__metadata_iterator_new();
void FLAC__metadata_iterator_delete(FLAC__Metadata_Iterator *iterator);

/*
 * Initialize the iterator to point to the first metadata block in the
 * given chain.
 */
void FLAC__metadata_iterator_init(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Chain *chain);

/*
 * These move the iterator forwards or backwards, returning false if
 * already at the end.
 */
FLAC__bool FLAC__metadata_iterator_next(FLAC__Metadata_Iterator *iterator);
FLAC__bool FLAC__metadata_iterator_prev(FLAC__Metadata_Iterator *iterator);

/*
 * Get the type of the metadata block at the current position.
 */
FLAC__MetadataType FLAC__metadata_iterator_get_block_type(const FLAC__Metadata_Iterator *iterator);

/*
 * Get the metadata block at the current position.  You can modify
 * the block in place but must write the chain before the changes
 * are reflected to the FLAC file.  You do not need to call
 * FLAC__metadata_iterator_set_block() to reflect the changes;
 * the pointer returned by FLAC__metadata_iterator_get_block()
 * points directly into the chain.
 *
 * Do not call FLAC__metadata_object_delete() on the returned object;
 * to delete a block use FLAC__metadata_iterator_delete_block().
 */
FLAC__StreamMetadata *FLAC__metadata_iterator_get_block(FLAC__Metadata_Iterator *iterator);

/*
 * Set the metadata block at the current position, replacing the existing
 * block.  The new block passed in becomes owned by the chain and will be
 * deleted when the chain is deleted.
 */
FLAC__bool FLAC__metadata_iterator_set_block(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);

/*
 * Removes the current block from the chain.  If replace_with_padding is
 * true, the block will instead be replaced with a padding block of equal
 * size.  You can not delete the STREAMINFO block.  The iterator will be
 * left pointing to the block before the one just 'deleted', even if
 * 'replace_with_padding' is true.
 */
FLAC__bool FLAC__metadata_iterator_delete_block(FLAC__Metadata_Iterator *iterator, FLAC__bool replace_with_padding);

/*
 * Insert a new block before or after the current block.  You cannot
 * insert a block before the first STREAMINFO block.  You cannot
 * insert a STREAMINFO block as there can be only one, the one that
 * already exists at the head when you read in a chain.  The iterator
 * will be left pointing to the new block.
 */
FLAC__bool FLAC__metadata_iterator_insert_block_before(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);
FLAC__bool FLAC__metadata_iterator_insert_block_after(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);


/******************************************************************************
	The following are methods for manipulating the defined block types.
	Since many are variable length we have to be careful about the memory
	management.  We decree that all pointers to data in the object are
	owned by the object and memory-managed by the object.

	Use the _new and _delete functions to create all instances.  When
	using the _set_ functions to set pointers to data, set 'copy' to true
	to have the function make it's own copy of the data, or to false to
	give the object ownership of your data.  In the latter case your pointer
	must be freeable by free() and will be free()d when the object is
	FLAC__metadata_object_delete()d.  It is legal to pass a null pointer
	as the data pointer to a _set_ function as long as then length argument
	is 0 and the copy argument is 'false'.

	The _new and _clone function will return NULL in the case of a memory
	allocation error, otherwise a new object.  The _set_ functions return
	false in the case of a memory allocation error.

	We don't have the convenience of C++ here, so note that the library
	relies on you to keep the types straight.  In other words, if you pass,
	for example, a FLAC__StreamMetadata* that represents a STREAMINFO block
	to FLAC__metadata_object_application_set_data(), you will get an
	assertion failure.

	There is no need to recalculate the length field on metadata blocks
	you have modified.  They will be calculated automatically before they
	are written back to a file.
******************************************************************************/


/******************************************************************
 * Common to all the types derived from FLAC__StreamMetadata:
 */
FLAC__StreamMetadata *FLAC__metadata_object_new(FLAC__MetadataType type);
FLAC__StreamMetadata *FLAC__metadata_object_clone(const FLAC__StreamMetadata *object);
void FLAC__metadata_object_delete(FLAC__StreamMetadata *object);
/* Does a deep comparison of the block data */
FLAC__bool FLAC__metadata_object_is_equal(const FLAC__StreamMetadata *block1, const FLAC__StreamMetadata *block2);

/******************************************************************
 * FLAC__StreamMetadata_Application
 * ----------------------------------------------------------------
 * Note: 'length' is in bytes.
 */
FLAC__bool FLAC__metadata_object_application_set_data(FLAC__StreamMetadata *object, FLAC__byte *data, unsigned length, FLAC__bool copy);

/******************************************************************
 * FLAC__StreamMetadata_SeekPoint
 * ----------------------------------------------------------------
 * @@@@  You can
 * use the _resize function to alter it.  If the size shrinks,
 * elements will truncated; if it grows, new placeholder points
 * will be added to the end.
 */

/******************************************************************
 * FLAC__StreamMetadata_SeekTable
 */
FLAC__bool FLAC__metadata_object_seektable_resize_points(FLAC__StreamMetadata *object, unsigned new_num_points);
void FLAC__metadata_object_seektable_set_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point);
FLAC__bool FLAC__metadata_object_seektable_insert_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point);
FLAC__bool FLAC__metadata_object_seektable_delete_point(FLAC__StreamMetadata *object, unsigned point_num);

/******************************************************************
 * FLAC__StreamMetadata_VorbisComment
 */
FLAC__bool FLAC__metadata_object_vorbiscomment_set_vendor_string(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
FLAC__bool FLAC__metadata_object_vorbiscomment_resize_comments(FLAC__StreamMetadata *object, unsigned new_num_comments);
FLAC__bool FLAC__metadata_object_vorbiscomment_set_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
FLAC__bool FLAC__metadata_object_vorbiscomment_insert_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
FLAC__bool FLAC__metadata_object_vorbiscomment_delete_comment(FLAC__StreamMetadata *object, unsigned comment_num);
FLAC__bool FLAC__metadata_object_seektable_is_legal(const FLAC__StreamMetadata *object);

#ifdef __cplusplus
}
#endif

#endif
