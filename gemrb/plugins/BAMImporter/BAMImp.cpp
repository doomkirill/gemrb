/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BAMImporter/BAMImp.cpp,v 1.33 2004/12/12 22:27:26 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "BAMImp.h"
#include "../Core/Interface.h"
#include "../Core/Compressor.h"
#include "../Core/FileStream.h"

BAMImp::BAMImp(void)
{
	str = NULL;
	autoFree = false;
	frames = NULL;
	cycles = NULL;
	FramesCount = 0;
	CyclesCount = 0;
}

BAMImp::~BAMImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	if (frames) {
		delete[] frames;
	}
	if (cycles) {
		delete[] cycles;
	}
}

bool BAMImp::Open(DataStream* stream, bool autoFree)
{
	unsigned int i;

	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	if (frames) {
		delete[] frames;
	}
	if (cycles) {
		delete[] cycles;
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "BAMCV1  ", 8 ) == 0) {
		//printf("Compressed file found...\n");
		//Check if Decompressed file has already been Cached
		char cpath[_MAX_PATH];
		strcpy( cpath, core->CachePath );
		strcat( cpath, stream->filename );
		FILE* exist_in_cache = fopen( cpath, "rb" );
		if (exist_in_cache) {
			//File was previously cached, using local copy
			if (autoFree) {
				delete( str );
			}
			fclose( exist_in_cache );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		} else {
			//No file found in Cache, Decompressing and storing for further use
			str->Seek( 4, GEM_CURRENT_POS );

			if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID )) {
				printf( "No Compression Manager Available.\nCannot Load Compressed Bam File.\n" );
				return false;
			}
			FILE* newfile = fopen( cpath, "wb" );
			Compressor* comp = ( Compressor* )
				core->GetInterface( IE_COMPRESSION_CLASS_ID );
			comp->Decompress( newfile, str );
			core->FreeInterface( comp );
			fclose( newfile );
			if (autoFree)
				delete( str );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		}
	}
	if (strncmp( Signature, "BAM V1  ", 8 ) != 0) {
		return false;
	}
	str->ReadWord( &FramesCount );
	str->Read( &CyclesCount, 1 );
	str->Read( &CompressedColorIndex, 1 );
	str->ReadDword( &FramesOffset );
	str->ReadDword( &PaletteOffset );
	str->ReadDword( &FLTOffset );
	str->Seek( FramesOffset, GEM_STREAM_START );
	frames = new FrameEntry[FramesCount];
	for (i = 0; i < FramesCount; i++) {
		str->ReadWord( &frames[i].Width );
		str->ReadWord( &frames[i].Height );
		str->ReadWord( &frames[i].XPos );
		str->ReadWord( &frames[i].YPos );
		str->ReadDword( &frames[i].FrameData );
	}
	cycles = new CycleEntry[CyclesCount];
	for (i = 0; i < CyclesCount; i++) {
		str->ReadWord( &cycles[i].FramesCount );
		str->ReadWord( &cycles[i].FirstFrame );
	}
	str->Seek( PaletteOffset, GEM_STREAM_START );
	//no idea if we have to switch this
	for (i = 0; i < 256; i++) {
		RevColor rc;
		str->Read( &rc, 4 );
		Palette[i].r = rc.r;
		Palette[i].g = rc.g;
		Palette[i].b = rc.b;
		Palette[i].a = rc.a;
	}
	return true;
}

int BAMImp::GetCycleSize(unsigned char Cycle)
{
	if(Cycle >= CyclesCount ) {
		return -1;
	}
	return cycles[Cycle].FramesCount;
}

Sprite2D* BAMImp::GetFrameFromCycle(unsigned char Cycle, unsigned short frame)
{
	if(Cycle >= CyclesCount ) {
		printf("[BAMImp] Invalid Cycle %d\n", (int) Cycle);
		return NULL;
	}
	if(cycles[Cycle].FramesCount<=frame) {
		printf("[BAMImp] Invalid Frame %d in Cycle %d\n",(int) frame, (int) Cycle);
		return NULL;
	}
	str->Seek( FLTOffset + ( cycles[Cycle].FirstFrame * 2 ) + ( frame * 2 ),
			GEM_STREAM_START );
	ieWord findex;
	str->ReadWord( &findex );
	return GetFrame( findex );
}

Animation* BAMImp::GetAnimation(unsigned char Cycle, int x, int y,
	unsigned char mode)
{
	unsigned int i;

	if(Cycle >=CyclesCount ) {
		return NULL;
	}
	if(!cycles[Cycle].FramesCount) {
		return NULL;
	}
	str->Seek( FLTOffset + ( cycles[Cycle].FirstFrame * 2 ), GEM_STREAM_START );
	unsigned short * findex = ( unsigned short * )
		malloc( cycles[Cycle].FramesCount * sizeof(unsigned short) );
	for (i = 0; i < cycles[Cycle].FramesCount; i++) {
		str->ReadWord( &findex[i] );
	}
	Animation* anim = new Animation( findex, cycles[Cycle].FramesCount );
	anim->x = x;
	anim->y = y;
	for (i = 0; i < cycles[Cycle].FramesCount; i++) {
		anim->AddFrame( GetFrame( findex[i], mode ), findex[i] );
	}
	free( findex );
	return anim;
}

Sprite2D* BAMImp::GetFrame(unsigned short findex, unsigned char mode)
{
	if (findex >= FramesCount) {
		findex = cycles[0].FirstFrame;
	}
	void* pixels = GetFramePixels(findex);
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8(
		frames[findex].Width, frames[findex].Height, 8,
		pixels, Palette, true, 0 );
	//don't free pixels, createsprite stores it

	spr->XPos = frames[findex].XPos;
	spr->YPos = frames[findex].YPos;
	if (mode == IE_SHADED) {
		core->GetVideoDriver()->ConvertToVideoFormat( spr );
		core->GetVideoDriver()->CalculateAlpha( spr );
	}
	return spr;
}

void* BAMImp::GetFramePixels(unsigned short findex)
{
	if (findex >= FramesCount) {
		findex = cycles[0].FirstFrame;
	}
	str->Seek( ( frames[findex].FrameData & 0x7FFFFFFF ), GEM_STREAM_START );
	unsigned long pixelcount = frames[findex].Height * frames[findex].Width;
	void* pixels = malloc( pixelcount );
	bool RLECompressed = ( ( frames[findex].FrameData & 0x80000000 ) == 0 );
	if (RLECompressed) {
		//if RLE Compressed
		unsigned long RLESize;
		RLESize = ( unsigned long )
			ceil( frames[findex].Width * frames[findex].Height * 1.5 );
		//without partial reads, we should be careful
		unsigned long remains = str->Remains();
		if (RLESize > remains) {
			RLESize = remains;
		}
		void* inpix = malloc( RLESize );
		unsigned char * p = ( unsigned char * ) inpix;
		unsigned char * Buffer = ( unsigned char * ) pixels;
		if (str->Read( inpix, RLESize ) == GEM_ERROR) {
			free( inpix );
			return NULL;
		}
		unsigned int i = 0;
		while (i < pixelcount) {
			if (*p == CompressedColorIndex) {
				p++;
				// FIXME: Czech HOW has apparently broken frame
				//   #141 in REALMS.BAM. Maybe we should put
				//   this condition to #ifdef BROKEN_xx ?
				// Or maybe rather put correct REALMS.BAM
				//   into override/ dir?
				if (i + ( *p ) + 1 > pixelcount) {
					memset( &Buffer[i], CompressedColorIndex, pixelcount - i );
					printf ("Broken frame %d in %s\n", findex, str->filename);
				} else {
					memset( &Buffer[i], CompressedColorIndex, ( *p ) + 1 );
				}
				i += *p;
			} else 
				Buffer[i] = *p;
			p++;
			i++;
		}
		free( inpix );
	} else {
		str->Read( pixels, pixelcount );
	}
	return pixels;
}

AnimationFactory* BAMImp::GetAnimationFactory(const char* ResRef,
	unsigned char mode)
{
	unsigned int i;

	AnimationFactory* af = new AnimationFactory( ResRef );
	int count = 0;
	for (i = 0; i < CyclesCount; i++) {
		if (cycles[i].FirstFrame + cycles[i].FramesCount > count) {
			count = cycles[i].FirstFrame + cycles[i].FramesCount;
		}
	}
	ieWord * FLT = ( ieWord * ) calloc( count, sizeof(ieWord) );
	str->Seek( FLTOffset, GEM_STREAM_START );
	str->Read( FLT, count * sizeof(ieWord) );
	if( DataStream::IsEndianSwitch() ) {
		//msvc likes it as char *
		swab( (char*) FLT, (char*) FLT, count * sizeof(ieWord) );
	}
	std::vector< unsigned short> indices;
	for (i = 0; i < CyclesCount; i++) {
		unsigned int ff = cycles[i].FirstFrame,
		lf = ff + cycles[i].FramesCount;
		for (unsigned int f = ff; f < lf; f++) {
			if (FLT[f] == 0xffff)
				continue;
			bool found = false;
			for (unsigned int k = 0; k < indices.size(); k++) {
				if (indices[k] == FLT[f]) {
					found = true;
					break;
				}
			}
			if (!found) {
				if (( frames[FLT[f]].Width != 1 ) &&
					( frames[FLT[f]].Height != 1 ))
					af->AddFrame( GetFrame( FLT[f], mode ), FLT[f] );
				indices.push_back( FLT[f] );
			}
		}
		af->AddCycle( cycles[i] );
	}
	af->LoadFLT( FLT, count );
	free( FLT );
	return af;
}
/** This function will load the Animation as a Font */
Font* BAMImp::GetFont()
{
	unsigned int i;

	int w = 0, h = 0;
	unsigned int Count;

	// Numeric fonts have all frames in single cycle
	if (CyclesCount > 1) {
		Count = CyclesCount;
	} else {
		Count = FramesCount;
	}

	for (i = 0; i < Count; i++) {
		unsigned int index;
		if (CyclesCount > 1) {
			index = cycles[i].FirstFrame;
			if (index >= FramesCount)
				continue;
		} else {
			index = i;
		}

		//printf("[index = %d, w = %d, h = %d]\n", index, frames[index].Width, frames[index].Height);
		if (frames[index].Width > w)
			w = frames[index].Width;
		if (frames[index].Height > h)
			h = frames[index].Height;
	}

	Font* fnt = new Font( w*( int ) Count, h, Palette, true, 0 );
	for (i = 0; i < Count; i++) {
		unsigned int index;
		if (CyclesCount > 1) {
			index = cycles[i].FirstFrame;
			if (index >= FramesCount) {
				fnt->AddChar( NULL, 0, 0, 0, 0 );
				continue;
			}
		} else {
			index = i;
		}

		void* pixels = GetFramePixels( index );
		if( !pixels) {
			fnt->AddChar( NULL, 0, 0, 0, 0 );
			continue;
		}
		fnt->AddChar( pixels, frames[index].Width,
				frames[index].Height,
				frames[index].XPos,
				frames[index].YPos );
		free( pixels );
	}
	return fnt;
}
/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
If the Global Animation Palette is NULL, returns NULL. */
Sprite2D* BAMImp::GetPalette()
{
	unsigned char * pixels = ( unsigned char * ) malloc( 256 );
	unsigned char * p = pixels;
	for (int i = 0; i < 256; i++) {
		*p++ = ( unsigned char ) i;
	}
	return core->GetVideoDriver()->CreateSprite8( 16, 16, 8, pixels, Palette, false );
}

void BAMImp::SetupColors(int *Colors)
{
        Color* MetalPal = core->GetPalette( Colors[0], 12 );
        Color* MinorPal = core->GetPalette( Colors[1], 12 );
        Color* MajorPal = core->GetPalette( Colors[2], 12 );
        Color* SkinPal = core->GetPalette( Colors[3], 12 );
        Color* LeatherPal = core->GetPalette( Colors[4], 12 );
        Color* ArmorPal = core->GetPalette( Colors[5], 12 );
        Color* HairPal = core->GetPalette( Colors[6], 12 );
        memcpy( &Palette[0x04], MetalPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x10], MinorPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x1C], MajorPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x28], SkinPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x34], LeatherPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x40], ArmorPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x4C], HairPal, 12 * sizeof( Color ) );
        memcpy( &Palette[0x58], &MinorPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x60], &MajorPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x68], &MinorPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x70], &MetalPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x78], &LeatherPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x80], &LeatherPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0x88], &MinorPal[1], 8 * sizeof( Color ) );

        int i; //moved here to be compatible with msvc6.0

        for (i = 0x90; i < 0xA8; i += 0x08)
                memcpy( &Palette[i], &LeatherPal[1], 8 * sizeof( Color ) );
        memcpy( &Palette[0xB0], &SkinPal[1], 8 * sizeof( Color ) );
        for (i = 0xB8; i < 0xFF; i += 0x08)
                memcpy( &Palette[i], &LeatherPal[1], 8 * sizeof( Color ) );

        free( MetalPal );
        free( MinorPal );
        free( MajorPal );
        free( SkinPal );
        free( LeatherPal );
        free( ArmorPal );
        free( HairPal );
}

Sprite2D* BAMImp::GetPaperdollImage(int *Colors)
{
	if (FramesCount<2) {
		return NULL;
	}
	if(Colors) {
		SetupColors(Colors);
	}

        void* pixels = GetFramePixels(0);
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8(frames[0].Width, frames[0].Height, 8, pixels, Palette, true, 0 );
	spr->XPos = frames[0].XPos;
	spr->YPos = frames[0].YPos;
	//don't free pixels, createsprite stores it
/*
        pixels = GetFramePixels(1);
	spr = core->GetVideoDriver()->CreateSprite8(frames[1].Width, frames[1].Height, 8, pixels, Palette, true, 0 );

	spr->XPos = frames[1].XPos;
	spr->YPos = frames[1].YPos;
*/
	return spr;
}
