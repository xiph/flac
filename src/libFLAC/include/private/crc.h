/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002,2003  Josh Coalson
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

#ifndef FLAC__PRIVATE__CRC_H
#define FLAC__PRIVATE__CRC_H

#include "FLAC/ordinals.h"

/* 8 bit CRC generator, MSB shifted first
** polynomial = x^8 + x^2 + x^1 + x^0
** init = 0
*/
extern FLAC__byte const FLAC__crc8_table[256];
#define FLAC__CRC8_UPDATE(data, crc) (crc) = FLAC__crc8_table[(crc) ^ (data)];
void FLAC__crc8_update(const FLAC__byte data, FLAC__uint8 *crc);
void FLAC__crc8_update_block(const FLAC__byte *data, unsigned len, FLAC__uint8 *crc);
FLAC__uint8 FLAC__crc8(const FLAC__byte *data, unsigned len);

/* 16 bit CRC generator, MSB shifted first
** polynomial = x^16 + x^15 + x^2 + x^0
** init = 0
*/
extern FLAC__uint16 FLAC__crc16_table[256];
#define FLAC__CRC16_UPDATE(data, crc) (crc) = ((crc)<<8) ^ FLAC__crc16_table[((crc)>>8) ^ (data)];
void FLAC__crc16_update(const FLAC__byte data, FLAC__uint16 *crc);
void FLAC__crc16_update_block(const FLAC__byte *data, unsigned len, FLAC__uint16 *crc);
FLAC__uint16 FLAC__crc16(const FLAC__byte *data, unsigned len);

#endif
