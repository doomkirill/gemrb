#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ie_types.h"

#define VERSION_GEMRB "0.2.3"

#define GEMRB_STRING "GemRB v" VERSION_GEMRB

#ifndef GLOBALS_ONLY_DEFS

#include "stdlib.h"
#include "stdio.h"
#include "errors.h"
#include "win32def.h"
#include "SClassID.h"
#include "../plugins/Core/Class_ID.h"
#include "../plugins/Core/ClassDesc.h"
#include "RGBAColor.h"
#include "../plugins/Core/Region.h"
#include "../plugins/Core/Sprite2D.h"
#include "../plugins/Core/VideoMode.h"
#include "../plugins/Core/VideoModes.h"
#include "../plugins/Core/DataStream.h"
#include "../plugins/Core/AnimStructures.h"

#endif //GLOBALS_ONLY_DEFS

//Global Variables

#ifdef WIN32
#define PathDelimiter '\\'
#define SPathDelimiter "\\"
#else
#define PathDelimiter '/'
#define SPathDelimiter "/"
#endif

#define ExtractFileFromPath(file, full_path) strcpy (file, ((strrchr (full_path, PathDelimiter)==NULL) ? ((strchr (full_path, ':')==NULL) ? full_path : (strchr(full_path, ':') +1) ) : (strrchr(full_path, PathDelimiter) +1)))

#define IE_NORMAL 0
#define IE_SHADED 1

#define IE_STR_STRREFON 1
#define IE_STR_SOUND	2
#define IE_STR_SPEECH   4

//IDS Importer Defines
#define IDS_VALUE_NOT_LOCATED -65535 // GetValue returns this if text is not found in arrays ... this needs to be a unique number that does not exist in the value[] array
#define GEM_ENCRYPTION_KEY "\x88\xa8\x8f\xba\x8a\xd3\xb9\xf5\xed\xb1\xcf\xea\xaa\xe4\xb5\xfb\xeb\x82\xf9\x90\xca\xc9\xb5\xe7\xdc\x8e\xb7\xac\xee\xf7\xe0\xca\x8e\xea\xca\x80\xce\xc5\xad\xb7\xc4\xd0\x84\x93\xd5\xf0\xeb\xc8\xb4\x9d\xcc\xaf\xa5\x95\xba\x99\x87\xd2\x9d\xe3\x91\xba\x90\xca"

/////feature flags
#define  GF_HAS_KAPUTZ  			0 //pst
#define  GF_ALL_STRINGS_TAGGED			1 //bg1, pst, iwd1
#define  GF_HAS_SONGLIST		2 //bg2
#define  GF_TEAM_MOVEMENT		3 //pst
#define  GF_UPPER_BUTTON_TEXT		4 //bg2
#define  GF_LOWER_LABEL_TEXT		5 //bg2
#define  GF_HAS_PARTY_INI   			6 //iwd2
#define  GF_SOUNDFOLDERS			7 //iwd2
#define  GF_IGNORE_BUTTON_FRAMES		8 // all?
#define  GF_ONE_BYTE_ANIMID			9 // pst
#define  GF_HAS_DPLAYER				10 // not pst
#define  GF_HAS_EXPTABLE			11 // iwd, iwd2
#define  GF_HAS_BEASTS_INI   			12 //pst; also for quests.ini
#define  GF_HAS_DESC_ICON   			13 //bg
#define  GF_HAS_PICK_SOUND   			14 //pst
#define  GF_IWD_MAP_DIMENSIONS			15 //iwd, iwd2
#define  GF_AUTOMAP_INI				16 //pst
#define  GF_SMALL_FOG                           17 //bg1, pst

/////AI global defines
#define AI_UPDATE_TIME	30

/////globally used functions
#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class Scriptable;

/* this function will work with pl/cz special characters */

extern unsigned char pl_uppercase[256];
extern unsigned char pl_lowercase[256];

GEM_EXPORT void strnuprcpy(char* d, const char *s, int l);
GEM_EXPORT void strnlwrcpy(char* d, const char *s, int l);
GEM_EXPORT unsigned char GetOrient(Point &s, Point &d);
GEM_EXPORT unsigned int Distance(Point pos, Point pos2);
GEM_EXPORT unsigned int Distance(Point pos, Scriptable *b);
GEM_EXPORT unsigned int Distance(Scriptable *a, Scriptable *b);
GEM_EXPORT bool dir_exists(const char* path);
GEM_EXPORT int strlench(const char* string, char ch);
#ifndef HAVE_STRNDUP
GEM_EXPORT char* strndup(const char* s, int l);
#endif

#ifdef WIN32
#define ResolveFilePath(p)
#else
char* FindInDir(char* Dir, char* Filename);
void ResolveFilePath(char* FilePath);
char* strupr(char* string);
char* strlwr(char* string);
#endif

#ifdef WIN32
#define GetTime(store) store = GetTickCount()
#else
#include <sys/time.h>
#define GetTime(store) \
	{ \
		struct timeval tv; \
		gettimeofday(&tv, NULL); \
		store = (tv.tv_usec/1000) + (tv.tv_sec*1000); \
	}
#endif

struct ActorBlock;

#endif //GLOBALS_H

