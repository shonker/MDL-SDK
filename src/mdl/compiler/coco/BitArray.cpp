/*-------------------------------------------------------------------------
Compiler Generator Coco/R,
Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
extended by M. Loeberbauer & A. Woess, Univ. of Linz
ported to C++ by Csaba Balazs, University of Szeged
with improvements by Pat Terry, Rhodes University

This program is free software; you can redistribute it and/or modify it 
under the terms of the GNU General Public License as published by the 
Free Software Foundation; either version 2, or (at your option) any 
later version.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
for more details.

You should have received a copy of the GNU General Public License along 
with this program; if not, write to the Free Software Foundation, Inc., 
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

As an exception, it is allowed to write an extension of Coco/R that is
used as a plugin in non-free software.

If not otherwise stated, any source code generated by Coco/R (other than 
Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/

#include <utility>
#include <cstring>
#include "BitArray.h"

namespace Coco {

BitArray::BitArray(size_t length, bool defaultValue)
{
	Count = length;
	Data = new unsigned char[ (length+7)>>3 ];
	if (defaultValue) {
		std::memset(Data, 0xFF, (length + 7) >> 3);
	} else {
		std::memset(Data, 0x00, (length + 7) >> 3);
	}
}

BitArray::BitArray(BitArray const &copy) {
	Count  = copy.Count;
	Data = new unsigned char[ (copy.Count+7)>>3 ];
	std::memcpy(Data, copy.Data, (copy.Count+7)>>3);
}

BitArray::BitArray(BitArray &&tmp) noexcept
{
	std::swap(Count, tmp.Count);
	std::swap(Data, tmp.Data);
}

BitArray::~BitArray()
{
	delete [] Data;
	Data = NULL;
}

size_t BitArray::getCount() const {
	return Count;
}

bool BitArray::Get(size_t index) const
{
	return (Data[(index>>3)] & (1<<(index&7))) != 0;
}

void BitArray::Set(size_t index, bool value)
{
	if (value){
		Data[(index>>3)] |= (1 << (index&7));
	} else {
		unsigned char mask = 0xFF;
		mask ^= (1 << (index&7));
		Data[(index>>3)] &= mask;
	}
}

void BitArray::SetAll(bool value)
{
	if (value) {
		std::memset(Data, 0xFF, (Count + 7) >> 3);
	} else {
		std::memset(Data, 0x00, (Count + 7) >> 3);
	}
}


void BitArray::Not()
{
	for (size_t i = 0; i < (Count+7)>>3; ++i) {
		Data[i] ^= 0xFF;
	}
}

void BitArray::And(BitArray const &value)
{
	size_t n = value.Count;
	if (Count < n) {
		n = Count;
	}
	for (size_t i = 0; i < (n+7)>>3; ++i) {
		Data[i] &= value.Data[i];
	}
}

void BitArray::AndNot(BitArray const &value)
{
	size_t n = value.Count;
	if (Count < n) {
		n = Count;
	}
	for (size_t i = 0; i < (n + 7) >> 3; ++i) {
		Data[i] &= ~value.Data[i];
	}
}

void BitArray::Or(BitArray const &value)
{
	size_t n = value.Count;
	if (Count < n) {
		n = Count;
	}
	for (size_t i = 0; i < (n+7)>>3; ++i) {
		Data[i] |= value.Data[i];
	}
}

void BitArray::Xor(BitArray const &value)
{
	size_t n = value.Count;
	if (Count < n) {
		n = Count;
	}
	for (size_t i = 0; i < (n+7)>>3; ++i) {
		Data[i] ^= value.Data[i];
	}
}

BitArray *BitArray::Clone() const
{
	return new BitArray(*this);
}

bool BitArray::Equal(BitArray const &right) const
{
	if (Count != right.Count) {
		return false;
	}
	for(size_t index = 0; index < Count; index++) {
		if (Get(index) != right.Get(index)) {
			return false;
		}
	}
	return true;
}

bool BitArray::Overlaps(BitArray const &right) const
{
	for (size_t index = 0; index < Count; ++index) {
		if (Get(index) && right.Get(index)) {
			return true;
		}
	}
	return false;
}

BitArray const &BitArray::operator=(BitArray const &right)
{
	if ( &right != this ) {         // avoid self assignment
		delete [] Data;              // prevents memory leak
		Count  = right.Count;
		Data = new unsigned char[ (Count+7)>>3 ];
		std::memcpy(Data, right.Data, (Count+7)>>3);
	}
	return *this;   // enables cascaded assignments
}

BitArray const &BitArray::operator=(BitArray &&tmp)
{
	std::swap(Count, tmp.Count);
	std::swap(Data, tmp.Data);
	return *this;
}

} // namespace
