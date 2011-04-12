/* mp4tags -- tool to set iTunes-compatible metadata tags
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Contributed to MPEG4IP
 * by Christopher League <league@contrapunctus.net>
 */

#include "util/impl.h"

using namespace mp4v2::util;

char *mkconcat(char **, int);
char *trimwhitespace(char *str);
int ratingIndexFromString( char *ratingString);
uint8_t getContentRatingFromString(char *contentRatingString);
uint16_t genreIndexFromString(const char* genreString);
uint8_t getMediaKindFromString(const char* mediaKindString);

enum rating_type {
    MPAA_NR = 1,
    MPAA_G,
    MPAA_PG,
    MPAA_PG_13,
    MPAA_R,
    MPAA_NC_17,
    MPAA_UNRATED,
    US_TV_Y     = 9,
    US_TV_Y7,
    US_TV_G,
    US_TV_PG,
    US_TV_14,
    US_TV_MA,
    US_TV_UNRATED,
    UK_MOVIE_NR     = 17,
    UK_MOVIE_U,
    UK_MOVIE_Uc,
    UK_MOVIE_PG,
    UK_MOVIE_12,
    UK_MOVIE_12A,
    UK_MOVIE_15,
    UK_MOVIE_18,
    UK_MOVIE_R18,
    UK_MOVIE_E,
    UK_MOVIE_UNRATED,
    UK_TV_CAUTION  = 29,
    DE_MOVIE_FSK_0 = 31,
    DE_MOVIE_FSK_6,
    DE_MOVIE_FSK_12,
    DE_MOVIE_FSK_16,
    DE_MOVIE_FSK_18,
    AU_MOVIE_G_0 = 37,
    AU_MOVIE_PG,
    AU_MOVIE_M,
    AU_MOVIE_MA_15,
    AU_MOVIE_R18,
    AU_TV_P = 43,
    AU_TV_C,
    AU_TV_G,
    AU_TV_PG,
    AU_TV_M,
    AU_TV_MA15,
    AU_TV_AV15,
    AU_TV_R18,
    FR_MOVIE_TOUT_PUBLIC = 52,
    FR_MOVIE_10,
    FR_MOVIE_12,
    FR_MOVIE_16,
    FR_MOVIE_18,
    FR_MOVIE_UNRATED,
    FR_TV_10 = 59,
    FR_TV_12,
    FR_TV_16,
    FR_TV_18,
    R_UNKNOWN   = 64,
};

typedef struct mediaKind_t
{
    uint8_t stik;
    char *english_name;
} mediaKind_t;


// stiks stikArray[] = {
//  { "Movie", 0 },
//  { "Normal", 1 },
//  { "Audiobook", 2 },
//  { "Whacked Bookmark", 5 },
//  { "Music Video", 6 },
//  { "Short Film", 9 },
//  { "TV Show", 10 },
//  { "Booklet", 11 }
// };
static const mediaKind_t mediaKind_strings[] = {
    {1, "Music"},
    {2, "Audiobook"},
    {6, "Music Video"},
    {9, "Movie"},
    {10, "TV Show"},
    {11, "Booklet"},
    {14, "Ringtone"},  
    {0, NULL},
};

typedef struct contentRating_t
{
    uint8_t rtng;
    char *english_name;
} contentRating_t;

static const contentRating_t contentRating_strings[] = {
    {0, "None"},
    {2, "Clean"},
    {4, "Explicit"},
    {0, NULL},
};

typedef struct iTMF_rating_t
{
    const char * rating;
    const char * english_name;
} iTMF_rating_t;

static const iTMF_rating_t rating_strings[] = {
    {"--", "-- United States"},
    {"mpaa|NR|000|", "Not Rated"},          // 1
    {"mpaa|G|100|", "G"},
    {"mpaa|PG|200|", "PG"},
    {"mpaa|PG-13|300|", "PG-13"},
    {"mpaa|R|400|", "R" },
    {"mpaa|NC-17|500|", "NC-17"},
    {"mpaa|Unrated|???|", "Unrated"},
    {"--", ""},
    {"us-tv|TV-Y|100|", "TV-Y"},            // 9
    {"us-tv|TV-Y7|200|", "TV-Y7"},
    {"us-tv|TV-G|300|", "TV-G"},
    {"us-tv|TV-PG|400|", "TV-PG"},
    {"us-tv|TV-14|500|", "TV-14"},
    {"us-tv|TV-MA|600|", "TV-MA"},
    {"us-tv|Unrated|???|", "Unrated"},
    {"--", "-- United Kingdom"},
    {"uk-movie|NR|000|", "Not Rated"},      // 17
    {"uk-movie|U|100|", "U"},
    {"uk-movie|Uc|150|", "Uc"},
    {"uk-movie|PG|200|", "PG"},
    {"uk-movie|12|300|", "12"},
    {"uk-movie|12A|325|", "12A"},
    {"uk-movie|15|350|", "15"},
    {"uk-movie|18|400|", "18"},
    {"uk-movie|R18|600|", "R18"},
    {"uk-movie|E|0|", "Exempt" },
    {"uk-movie|Unrated|???|", "Unrated"},
    {"--", ""},
    {"uk-tv|Caution|500|", "Caution"},      // 29
    {"--", "-- Germany"},
    {"de-movie|ab 0 Jahren|75|", "ab 0 Jahren"},		// 31
    {"de-movie|ab 6 Jahren|100|", "ab 6 Jahren"},
    {"de-movie|ab 12 Jahren|200|", "ab 12 Jahren"},
    {"de-movie|ab 16 Jahren|500|", "ab 16 Jahren"},
    {"de-movie|ab 18 Jahren|600|", "ab 18 Jahren"},
    {"--", ""},
    {"de-tv|ab 0 Jahren|75|", "ab 0 Jahren"},		// 37
    {"de-tv|ab 6 Jahren|100|", "ab 6 Jahren"},
    {"de-tv|ab 12 Jahren|200|", "ab 12 Jahren"},
    {"de-tv|ab 16 Jahren|500|", "ab 16 Jahren"},
    {"de-tv|ab 18 Jahren|600|", "ab 18 Jahren"},
    {"--", "-- Australia"},
    {"au-movie|G|100|", "G"},               // 43
    {"au-movie|PG|200|", "PG"},
    {"au-movie|M|350|", "M"},
    {"au-movie|MA15+|375|", "MA 15+"},
    {"au-movie|R18+|400|", "R18+"},
    {"--", ""},
    {"au-tv|P|100|", "P"},                  // 49
    {"au-tv|C|200|", "C"},
    {"au-tv|G|300|", "G"},
    {"au-tv|PG|400|", "PG"},
    {"au-tv|M|500|", "M"},
    {"au-tv|MA15+|550|", "MA 15+"},
    {"au-tv|AV15+|575|", "AV 15+"},
    {"au-tv|R18+|600|", "R18+"},
    {"--", "-- France"},
    {"fr-movie|Tout Public|100|", "Tout Public"},     // 58
    {"fr-movie|-10|100|", "-10"},
    {"fr-movie|-12|300|", "-12"},
    {"fr-movie|-16|375|", "-16"},
    {"fr-movie|-18|400|", "-18"},
    {"fr-movie|Unrated|???|", "Unrated"},
    {"--", ""},
    {"fr-tv|-10|100|", "-10"},              // 65
    {"fr-tv|-12|200|", "-12"},
    {"fr-tv|-16|500|", "-16"},
    {"fr-tv|-18|600|", "-18"},
    {"--", ""},
    {"--", "Unknown"},                      // 70
    {NULL, NULL},
};

typedef struct genreType_t
{
    uint8_t index;
    const char *short_name;
    const char *english_name;
} genreType_t;

static const genreType_t genreType_strings[] = {
    {1,   "blues",             "Blues" },
    {2,   "classicrock",       "Classic Rock" },
    {3,   "country",           "Country" },
    {4,   "dance",             "Dance" },
    {5,   "disco",             "Disco" },
    {6,   "funk",              "Funk" },
    {7,   "grunge",            "Grunge" },
    {8,   "hiphop",            "Hop-Hop" },
    {9,   "jazz",              "Jazz" },
    {10,  "metal",             "Metal" },
    {11,  "newage",            "New Age" },
    {12,  "oldies",            "Oldies" },
    {13,  "other",             "Other" },
    {14,  "pop",               "Pop" },
    {15,  "rand_b",            "R&B" },
    {16,  "rap",               "Rap" },
    {17,  "reggae",            "Reggae" },
    {18,  "rock",              "Rock" },
    {19,  "techno",            "Techno" },
    {20,  "industrial",        "Industrial" },
    {21,  "alternative",       "Alternative" },
    {22,  "ska",               "Ska" },
    {23,  "deathmetal",        "Death Metal" },
    {24,  "pranks",            "Pranks" },
    {25,  "soundtrack",        "Soundtrack" },
    {26,  "eurotechno",        "Euro-Techno" },
    {27,  "ambient",           "Ambient" },
    {28,  "triphop",           "Trip-Hop" },
    {29,  "vocal",             "Vocal" },
    {30,  "jazzfunk",          "Jazz+Funk" },
    {31,  "fusion",            "Fusion" },
    {32,  "trance",            "Trance" },
    {33,  "classical",         "Classical" },
    {34,  "instrumental",      "Instrumental" },
    {35,  "acid",              "Acid" },
    {36,  "house",             "House" },
    {37,  "game",              "Game" },
    {38,  "soundclip",         "Sound Clip" },
    {39,  "gospel",            "Gospel" },
    {40,  "noise",             "Noise" },
    {41,  "alternrock",        "AlternRock" },
    {42,  "bass",              "Bass" },
    {43,  "soul",              "Soul" },
    {44,  "punk",              "Punk" },
    {45,  "space",             "Space" },
    {46,  "meditative",        "Meditative" },
    {47,  "instrumentalpop",   "Instrumental Pop" },
    {48,  "instrumentalrock",  "Instrumental Rock" },
    {49,  "ethnic",            "Ethnic" },
    {50,  "gothic",            "Gothic" },
    {51,  "darkwave",          "Darkwave" },
    {52,  "technoindustrial",  "Techno-Industrial" },
    {53,  "electronic",        "Electronic" },
    {54,  "popfolk",           "Pop-Folk" },
    {55,  "eurodance",         "Eurodance" },
    {56,  "dream",             "Dream" },
    {57,  "southernrock",      "Southern Rock" },
    {58,  "comedy",            "Comedy" },
    {59,  "cult",              "Cult" },
    {60,  "gangsta",           "Gangsta" },
    {61,  "top40",             "Top 40" },
    {62,  "christianrap",      "Christian Rap" },
    {63,  "popfunk",           "Pop/Funk" },
    {64,  "jungle",            "Jungle" },
    {65,  "nativeamerican",    "Native American" },
    {66,  "cabaret",           "Cabaret" },
    {67,  "newwave",           "New Wave" },
    {68,  "psychedelic",       "Psychedelic" },
    {69,  "rave",              "Rave" },
    {70,  "showtunes",         "Showtunes" },
    {71,  "trailer",           "Trailer" },
    {72,  "lofi",              "Lo-Fi" },
    {73,  "tribal",            "Tribal" },
    {74,  "acidpunk",          "Acid Punk" },
    {75,  "acidjazz",          "Acid Jazz" },
    {76,  "polka",             "Polka" },
    {77,  "retro",             "Retro" },
    {78,  "musical",           "Musical" },
    {79,  "rockand_roll",      "Rock & Roll" },
    
    {80,  "hardrock",          "Hard Rock" },
    {81,  "folk",              "Folk" },
    {82,  "folkrock",          "Folk-Rock" },
    {83,  "nationalfolk",      "National Folk" },
    {84,  "swing",             "Swing" },
    {85,  "fastfusion",        "Fast Fusion" },
    {86,  "bebob",             "Bebob" },
    {87,  "latin",             "Latin" },
    {88,  "revival",           "Revival" },
    {89,  "celtic",            "Celtic" },
    {90,  "bluegrass",         "Bluegrass" },
    {91,  "avantgarde",        "Avantgarde" },
    {92,  "gothicrock",        "Gothic Rock" },
    {93,  "progressiverock",   "Progresive Rock" },
    {94,  "psychedelicrock",   "Psychedelic Rock" },
    {95,  "symphonicrock",     "SYMPHONIC_ROCK" },
    {96,  "slowrock",          "Slow Rock" },
    {97,  "bigband",           "Big Band" },
    {98,  "chorus",            "Chorus" },
    {99,  "easylistening",     "Easy Listening" },
    {100, "acoustic",          "Acoustic" },
    {101, "humour",            "Humor" },
    {102, "speech",            "Speech" },
    {103, "chanson",           "Chason" },
    {104, "opera",             "Opera" },
    {105, "chambermusic",      "Chamber Music" },
    {106, "sonata",            "Sonata" },
    {107, "symphony",          "Symphony" },
    {108, "bootybass",         "Booty Bass" },
    {109, "primus",            "Primus" },
    {110, "porngroove",        "Porn Groove" },
    {111, "satire",            "Satire" },
    {112, "slowjam",           "Slow Jam" },
    {113, "club",              "Club" },
    {114, "tango",             "Tango" },
    {115, "samba",             "Samba" },
    {116, "folklore",          "Folklore" },
    {117, "ballad",            "Ballad" },
    {118, "powerballad",       "Power Ballad" },
    {119, "rhythmicsoul",      "Rhythmic Soul" },
    {120, "freestyle",         "Freestyle" },
    {121, "duet",              "Duet" },
    {122, "punkrock",          "Punk Rock" },
    {123, "drumsolo",          "Drum Solo" },
    {124, "acapella",          "A capella" },
    {125, "eurohouse",         "Euro-House" },
    {126, "dancehall",         "Dance Hall" },
    {255, "none",              "none" },
    
    {0, "undefined" } // must be last
};

///////////////////////////////////////////////////////////////////////////////

/* One-letter options -- if you want to rearrange these, change them
   here, immediately below in OPT_STRING, and in the help text. */
#define OPT_HELP         0x01ff
#define OPT_VERSION      0x02ff
#define OPT_REMOVE       'r'
#define OPT_ALBUM        'A'
#define OPT_ARTIST       'a'
#define OPT_TEMPO        'b'
#define OPT_COMMENT      'c'
#define OPT_COPYRIGHT    'C'
#define OPT_DISK         'd'
#define OPT_DISKS        'D'
#define OPT_ENCODEDBY    'e'
#define OPT_TOOL         'E'
#define OPT_GENRE        'g'
#define OPT_GROUPING     'G'
#define OPT_HD           'H'
#define OPT_MEDIA_TYPE   'i'
#define OPT_CONTENTID    'I'
#define OPT_LONGDESC     'l'
#define OPT_GENREID      'j'
#define OPT_LYRICS       'L'
#define OPT_DESCRIPTION  'm'
#define OPT_TVEPISODE    'M'
#define OPT_TVSEASON     'n'
#define OPT_TVNETWORK    'N'
#define OPT_TVEPISODEID  'o'
#define OPT_PLAYLISTID   'p'
#define OPT_PICTURE      'P'
#define OPT_PODCAST      'B'
#define OPT_ALBUM_ARTIST 'R'
#define OPT_NAME         's'
#define OPT_TVSHOW       'S'
#define OPT_TRACK        't'
#define OPT_TRACKS       'T'
#define OPT_XID          'x'
#define OPT_COMPOSER     'w'
#define OPT_RELEASEDATE  'y'
#define OPT_ARTISTID     'z'
#define OPT_COMPOSERID   'Z'

#define OPT_CAST         'W'
#define OPT_DIRECTOR     'F'
#define OPT_CODIRECTOR   'J'
#define OPT_PRODUCERS    'K'
#define OPT_SWRITERS     'O'
#define OPT_COPYWARNING  'Q'
#define OPT_STUDIO       'U'
#define OPT_RATING       'Y'
#define OPT_RANNOTATION  'V'
#define OPT_CRATING      'X'


#define OPT_STRING  "r:A:a:b:c:C:d:D:e:E:g:G:H:i:I:j:l:L:m:M:n:N:o:p:P:B:R:s:S:t:T:x:w:y:z:Z:W:F:J:K:O:Q:U:Y:V:X:"

#define ELEMENT_OF(x,i) x[int(i)]

static const char* const help_text =
    "OPTION... FILE...\n"
    "Adds or modifies iTunes-compatible tags on MP4 files.\n"
    "\n"
    "      -help            Display this help text and exit\n"
    "      -version         Display version information and exit\n"
    "  -A, -album       STR  Set the album title\n"   // --album     set album tag
    "  -a, -artist      STR  Set the artist information\n"   // --artist    set artist tag
    "  -b, -tempo       NUM  Set the tempo (beats per minute)\n"  // --tempo     set tempo tag
    "  -c, -comment     STR  Set a general comment\n"  // --comments    set comments tag
    "  -C, -copyright   STR  Set the copyright information\n"  // --copyright     set copyright tag
    "  -d, -disk        NUM  Set the disk number\n"
    "  -D, -disks       NUM  Set the number of disks\n"  // --disk_n    set disk number tag
    "  -e, -encodedby   STR  Set the name of the person or company who encoded the file\n"  // --encoded_by  set encoded by tag
    "  -E, -tool        STR  Set the software used for encoding\n"  // --encoding_tool   set encoding tool tag
    "  -g, -genre       STR  Set the genre name\n"    // --genre     set genre tag
    "  -G, -grouping    STR  Set the grouping name\n"    // --grouping    set grouping tag
    "  -H, -hdvideo     NUM  Set the HD flag (1\\0)\n" // --is_hd_video     set the hd video tag [yes/no]
    "  -i, -type        STR  Set the Media Type(\"Movie\", \"TV Show\", \"Music Video\", ...)\n" // --media_kind  set the media kind tag ("Normal", "Movie", "TV Show", "Audiobook", "Whacked Bookmark", "Music Video", "Short Film", "Booklets")
    "  -I, -contentid   NUM  Set the content ID\n" // --cnid    set cnid tag
    "  -j, -genreid     NUM  Set the genre ID\n"
    "  -l, -longdesc    NUM  Set the long description\n"  // --long_description  set long description tag
    "  -L, -lyrics      NUM  Set the lyrics\n"  // --lyrics    set lyrics tag
    "  -m, -description STR  Set the short description\n"  // --description   set description tag
    "  -M, -episode     NUM  Set the episode number\n"  // --tv_episode_n  set tv episode number tag
    "  -n, -season      NUM  Set the season number\n"  // --tv_season     set tv season tag
    "  -N, -network     STR  Set the TV network\n"  // --tv_network  set tv network tag
    "  -o, -episodeid   STR  Set the TV episode ID\n"   // --tv_episode_id   set tv episode id tag
    "  -p, -playlistid  NUM  Set the playlist ID\n"
    "  -P, -picture     PTH  Set the picture as a .png\n" // --artwork   set the artwork tag
    "  -B, -podcast     NUM  Set the podcast flag.\n"
    "  -R, -albumartist STR  Set the album artist\n" // --album_artist  set album artist tag
    "  -s, -song        STR  Set the title of the song, movie, tv show,...\n"  // --name    set name tag
    "  -S  -show        STR  Set the TV show\n"  // --tv_show     set tv show tag
    "  -t, -track       NUM  Set the track number\n"    // --track_n     set track number tag
    "  -T, -tracks      NUM  Set the number of tracks\n" 
    "  -x, -xid         STR  Set the globally-unique xid (vendor:scheme:id)\n"
    "  -w, -writer      STR  Set the composer information\n" // // --composer set composer tag
    "  -y, -year        NUM  Set the release date\n"  // --release_date  set release date tag
    "  -z, -artistid    NUM  Set the artist ID\n"  
    "  -Z, -composerid  NUM  Set the composer ID\n"
    "  -W, -cast        STR  Set the cast|actors tag (AppleTV)\n"  // iTunMOVI
    "  -F, -director    STR  Set the director tag (AppleTV)\n"  // iTunMOVI
    "  -J, -codirector  STR  Set the codirector tag (AppleTV)\n"  // iTunMOVI
    "  -K, -producers   STR  Set the producers tag (AppleTV)\n"  // iTunMOVI
    "  -O, -swriters    STR  Set the screen writers tag (AppleTV)\n"  // iTunMOVI
    "  -Q, -copywarning STR  Add copy warning (AppleTV)\n"  // iTunMOVI
    "  -U, -studio      STR  Add film studio (AppleTV)\n"  // iTunMOVI
    "  -Y, -rating      STR  Add film ratings (AppleTV)\n"  // --rati\nng    set rating tag (MPAA: "Not Rated",  "G",  "PG",  "PG-13",  "R" ,  "NC-17",  "Unrated", US-TV: "TV-Y",  "TV-Y7",  "TV-G",  "TV-PG",  "TV-14",  "TV-MA",  "Unrated")
    "  -V  -rannotation STR  Add rating annotation to the ratings, ie rated r for violence\n"
    "  -X  -crating     STR  Add content rating tag. \"Inoffensive\", \"Clean\", \"Explicit\"\n" // --content_rating  set content rating tag ("Inoffensive", "Clean", "Explicit")
    "  -r, -remove      STR  Remove tags by code (e.g. \"-r cs\"\n"
    "                        removes the comment and song tags)";
  
  // --is_gapless    set gapless tag [yes/no]
  

extern "C" int
    main( int argc, char** argv )
{
    const prog::Option long_options[] = {
        { "help",        prog::Option::NO_ARG,       0, OPT_HELP         },
        { "version",     prog::Option::NO_ARG,       0, OPT_VERSION      },
        { "album",       prog::Option::REQUIRED_ARG, 0, OPT_ALBUM        },
        { "artist",      prog::Option::REQUIRED_ARG, 0, OPT_ARTIST       },
        { "comment",     prog::Option::REQUIRED_ARG, 0, OPT_COMMENT      },
        { "copyright",   prog::Option::REQUIRED_ARG, 0, OPT_COPYRIGHT    },
        { "disk",        prog::Option::REQUIRED_ARG, 0, OPT_DISK         },
        { "disks",       prog::Option::REQUIRED_ARG, 0, OPT_DISKS        },
        { "encodedby",   prog::Option::REQUIRED_ARG, 0, OPT_ENCODEDBY    },
        { "tool",        prog::Option::REQUIRED_ARG, 0, OPT_TOOL         },
        { "genre",       prog::Option::REQUIRED_ARG, 0, OPT_GENRE        },
        { "grouping",    prog::Option::REQUIRED_ARG, 0, OPT_GROUPING     },
        { "hdvideo",     prog::Option::REQUIRED_ARG, 0, OPT_HD           },
        { "type",        prog::Option::REQUIRED_ARG, 0, OPT_MEDIA_TYPE   },
        { "contentid",   prog::Option::REQUIRED_ARG, 0, OPT_CONTENTID    },
        { "longdesc",    prog::Option::REQUIRED_ARG, 0, OPT_LONGDESC     },
        { "genreid",     prog::Option::REQUIRED_ARG, 0, OPT_GENREID      },
        { "lyrics",      prog::Option::REQUIRED_ARG, 0, OPT_LYRICS       },
        { "description", prog::Option::REQUIRED_ARG, 0, OPT_DESCRIPTION  },
        { "episode",     prog::Option::REQUIRED_ARG, 0, OPT_TVEPISODE    },
        { "season",      prog::Option::REQUIRED_ARG, 0, OPT_TVSEASON     },
        { "network",     prog::Option::REQUIRED_ARG, 0, OPT_TVNETWORK    },
        { "episodeid",   prog::Option::REQUIRED_ARG, 0, OPT_TVEPISODEID  },
        { "playlistid",  prog::Option::REQUIRED_ARG, 0, OPT_PLAYLISTID   },
        { "picture",     prog::Option::REQUIRED_ARG, 0, OPT_PICTURE      },
        { "podcast",     prog::Option::REQUIRED_ARG, 0, OPT_PODCAST      },
        { "song",        prog::Option::REQUIRED_ARG, 0, OPT_NAME         },
        { "show",        prog::Option::REQUIRED_ARG, 0, OPT_TVSHOW       },
        { "tempo",       prog::Option::REQUIRED_ARG, 0, OPT_TEMPO        },
        { "track",       prog::Option::REQUIRED_ARG, 0, OPT_TRACK        },
        { "tracks",      prog::Option::REQUIRED_ARG, 0, OPT_TRACKS       },
        { "xid",         prog::Option::REQUIRED_ARG, 0, OPT_XID          },
        { "writer",      prog::Option::REQUIRED_ARG, 0, OPT_COMPOSER     },
        { "year",        prog::Option::REQUIRED_ARG, 0, OPT_RELEASEDATE  },
        { "artistid",    prog::Option::REQUIRED_ARG, 0, OPT_ARTISTID     },
        { "composerid",  prog::Option::REQUIRED_ARG, 0, OPT_COMPOSERID   },
        { "remove",      prog::Option::REQUIRED_ARG, 0, OPT_REMOVE       },
        { "albumartist", prog::Option::REQUIRED_ARG, 0, OPT_ALBUM_ARTIST },
        { "cast",        prog::Option::REQUIRED_ARG, 0, OPT_CAST         },
        { "director",    prog::Option::REQUIRED_ARG, 0, OPT_DIRECTOR     },
        { "codirector",  prog::Option::REQUIRED_ARG, 0, OPT_CODIRECTOR   },
        { "producers",   prog::Option::REQUIRED_ARG, 0, OPT_PRODUCERS    },
        { "swriters",    prog::Option::REQUIRED_ARG, 0, OPT_SWRITERS     },
        { "copywarning", prog::Option::REQUIRED_ARG, 0, OPT_COPYWARNING  },
        { "studio",      prog::Option::REQUIRED_ARG, 0, OPT_STUDIO       },
        { "rating",      prog::Option::REQUIRED_ARG, 0, OPT_RATING       },
        { "rannotation", prog::Option::REQUIRED_ARG, 0, OPT_RANNOTATION  },
        { "crating",     prog::Option::REQUIRED_ARG, 0, OPT_CRATING      },
        { NULL, prog::Option::NO_ARG, 0, 0 }
    };

    /* Sparse arrays of tag data: some space is wasted, but it's more
       convenient to say tags[OPT_SONG] than to enumerate all the
       metadata types (again) as a struct. */
    const char *tags[UCHAR_MAX];
    uint64_t nums[UCHAR_MAX];

    memset( tags, 0, sizeof( tags ) );
    memset( nums, 0, sizeof( nums ) );

    /* Any modifications requested? */
    int mods = 0;

    /* Option-processing loop. */
    int c = prog::getOptionSingle( argc, argv, OPT_STRING, long_options, NULL );
    while ( c != -1 ) {
        int r = 2;
        switch ( c ) {
                /* getopt() returns '?' if there was an error.  It already
                   printed the error message, so just return. */
            case '?':
                return 1;

                /* Help and version requests handled here. */
            case OPT_HELP:
                fprintf( stderr, "usage %s %s\n", argv[0], help_text );
                return 0;
            case OPT_VERSION:
                fprintf( stderr, "%s - %s\n", argv[0], MP4V2_PROJECT_name_formal );
                return 0;

                /* Integer arguments: convert them using sscanf(). */
            case OPT_TEMPO:
            case OPT_DISK:
            case OPT_DISKS:
            case OPT_HD:
            case OPT_CONTENTID:
            case OPT_GENREID:
            case OPT_TVEPISODE:
            case OPT_TVSEASON:
            case OPT_PLAYLISTID:
            case OPT_TRACK:
            case OPT_TRACKS:
            case OPT_ARTISTID:
            case OPT_COMPOSERID:
            case OPT_PODCAST:
                if ( c == OPT_PLAYLISTID ) {
                    r = sscanf( prog::optarg, "%llu", &nums[c] );
                } else {
                    unsigned int n;
                    r = sscanf( prog::optarg, "%u", &n );
                    if ( r >= 1 )
                    {
                        nums[c] = static_cast<uint64_t>( n );
                    }
                }
                if ( r < 1 ) {
                    fprintf( stderr, "%s: option requires integer argument -- %c\n",
                             argv[0], c );
                    return 2;
                }
                /* Break not, lest ye be broken.  :) */
                /* All arguments: all valid options end up here, and we just
                   stuff the string pointer into the tags[] array. */
            default:
                tags[c] = prog::optarg;
                mods++;
        } /* end switch */

        c = prog::getOptionSingle( argc, argv, OPT_STRING, long_options, NULL );
    } /* end while */

    /* Check that we have at least one non-option argument */
    if ( ( argc - prog::optind ) < 1 ) {
        fprintf( stderr,
                 "%s: You must specify at least one MP4 file.\n",
                 argv[0] );
        fprintf( stderr, "usage %s %s\n", argv[0], help_text );
        return 3;
    }

    /* Check that we have at least one requested modification.  Probably
       it's useful instead to print the metadata if no modifications are
       requested? */
    if ( !mods ) {
        fprintf( stderr,
                 "%s: You must specify at least one tag modification.\n",
                 argv[0] );
        fprintf( stderr, "usage %s %s\n", argv[0], help_text );
        return 4;
    }

    /* Loop through the non-option arguments, and modify the tags as
       requested. */
    while ( prog::optind < argc ) {
        char *mp4 = argv[prog::optind++];

        MP4FileHandle h = MP4Modify( mp4 );
        if ( h == MP4_INVALID_FILE_HANDLE ) {
            fprintf( stderr, "Could not open '%s'... aborting\n", mp4 );
            return 5;
        }
        /* Read out the existing metadata */
        const MP4Tags* mdata = MP4TagsAlloc();
        MP4TagsFetch( mdata, h );

        /* Remove any tags. ELEMENT_OF(x,i) x[int(i)]*/
        if ( ELEMENT_OF(tags,OPT_REMOVE) ) {
            for ( const char *p = ELEMENT_OF(tags,OPT_REMOVE); *p; p++ ) {
                switch ( *p ) {
                    case OPT_CRATING:
                    case OPT_RANNOTATION: 
                    {
                      MP4ItmfItemList* list = MP4ItmfGetItemsByMeaning(h, "com.apple.iTunes", "iTunEXTC");
                      if (list) {
                          uint32_t i;
                          for (i = 0; i < list->size; i++) {
                              MP4ItmfItem* item = &list->elements[i];
                              MP4ItmfRemoveItem(h, item);
                          }
                      }
                      break;
                    }
                    case OPT_CAST:
                    case OPT_DIRECTOR:
                    case OPT_CODIRECTOR:
                    case OPT_PRODUCERS:
                    case OPT_SWRITERS:
                    case OPT_COPYWARNING:
                    case OPT_STUDIO:
                    {
                      MP4ItmfItemList* list = MP4ItmfGetItemsByMeaning(h, "com.apple.iTunes", "iTunMOVI");
                      if (list) {
                          uint32_t i;
                          for (i = 0; i < list->size; i++) {
                              MP4ItmfItem* item = &list->elements[i];
                              MP4ItmfRemoveItem(h, item);
                          }
                      }
                      break;
                    }
                    case OPT_ALBUM:
                        MP4TagsSetAlbum( mdata, NULL );
                        break;
                    case OPT_ARTIST:
                        MP4TagsSetArtist( mdata, NULL );
                        break;
                    case OPT_TEMPO:
                        MP4TagsSetTempo( mdata, NULL );
                        break;
                    case OPT_COMMENT:
                        MP4TagsSetComments( mdata, NULL );
                        break;
                    case OPT_COPYRIGHT:
                        MP4TagsSetCopyright( mdata, NULL );
                        break;
                    case OPT_DISK:
                        MP4TagsSetDisk( mdata, NULL );
                        break;
                    case OPT_DISKS:
                        MP4TagsSetDisk( mdata, NULL );
                        break;
                    case OPT_ENCODEDBY:
                        MP4TagsSetEncodedBy( mdata, NULL );
                        break;
                    case OPT_TOOL:
                        MP4TagsSetEncodingTool( mdata, NULL );
                        break;
                    case OPT_GENRE:
                        MP4TagsSetGenre( mdata, NULL );
                        MP4TagsSetGenreType( mdata, NULL);
                        break;
                    case OPT_GROUPING:
                        MP4TagsSetGrouping( mdata, NULL );
                        break;
                    case OPT_HD:
                        MP4TagsSetHDVideo( mdata, NULL );
                        break;
                    case OPT_MEDIA_TYPE:
                        MP4TagsSetMediaType( mdata, NULL );
                        break;
                    case OPT_CONTENTID:
                        MP4TagsSetContentID( mdata, NULL );
                        break;
                    case OPT_LONGDESC:
                        MP4TagsSetLongDescription( mdata, NULL );
                        break;
                    case OPT_GENREID:
                        MP4TagsSetGenreID( mdata, NULL );
                        break;
                    case OPT_LYRICS:
                        MP4TagsSetLyrics( mdata, NULL );
                        break;
                    case OPT_DESCRIPTION:
                        MP4TagsSetDescription( mdata, NULL );
                        break;
                    case OPT_TVEPISODE:
                        MP4TagsSetTVEpisode( mdata, NULL );
                        break;
                    case OPT_TVSEASON:
                        MP4TagsSetTVSeason( mdata, NULL );
                        break;
                    case OPT_TVNETWORK:
                        MP4TagsSetTVNetwork( mdata, NULL );
                        break;
                    case OPT_TVEPISODEID:
                        MP4TagsSetTVEpisodeID( mdata, NULL );
                        break;
                    case OPT_PLAYLISTID:
                        MP4TagsSetPlaylistID( mdata, NULL );
                        break;
                    case OPT_PICTURE:
                        if( mdata->artworkCount )
                            MP4TagsRemoveArtwork( mdata, 0 );
                        break;
                    case OPT_ALBUM_ARTIST:
                        MP4TagsSetAlbumArtist( mdata, NULL );
                        break ;
                    case OPT_NAME:
                        MP4TagsSetName( mdata, NULL );
                        break;
                    case OPT_TVSHOW:
                        MP4TagsSetTVShow( mdata, NULL );
                        break;
                    case OPT_TRACK:
                        MP4TagsSetTrack( mdata, NULL );
                        break;
                    case OPT_TRACKS:
                        MP4TagsSetTrack( mdata, NULL );
                        break;
                    case OPT_XID:
                        MP4TagsSetXID( mdata, NULL );
                        break;
                    case OPT_COMPOSER:
                        MP4TagsSetComposer( mdata, NULL );
                        break;
                    case OPT_RELEASEDATE:
                        MP4TagsSetReleaseDate( mdata, NULL );
                        break;
                    case OPT_ARTISTID:
                        MP4TagsSetArtistID( mdata, NULL );
                        break;
                    case OPT_COMPOSERID:
                        MP4TagsSetComposerID( mdata, NULL );
                        break;
                    case OPT_PODCAST:
                        MP4TagsSetPodcast(mdata, NULL);
                        break;
                }
            }
        }

        /* Track/disk numbers need to be set all at once, but we'd like to
           allow users to just specify -T 12 to indicate that all existing
           track numbers are out of 12.  This means we need to look up the
           current info if it is not being set. */

        if ( ELEMENT_OF(tags,OPT_TRACK) || ELEMENT_OF(tags,OPT_TRACKS) ) {
            MP4TagTrack tt;
            tt.index = 0;
            tt.total = 0;

            if( mdata->track ) {
                tt.index = mdata->track->index;
                tt.total = mdata->track->total;
            }

            if( ELEMENT_OF(tags,OPT_TRACK) )
                tt.index = static_cast<uint16_t>( ELEMENT_OF(nums,OPT_TRACK) );
            if( ELEMENT_OF(tags,OPT_TRACKS) )
                tt.total = static_cast<uint16_t>( ELEMENT_OF(nums,OPT_TRACKS) );

            MP4TagsSetTrack( mdata, &tt );
        }

        if ( ELEMENT_OF(tags,OPT_DISK) || ELEMENT_OF(tags,OPT_DISKS) ) {
            MP4TagDisk td;
            td.index = 0;
            td.total = 0;

            if( mdata->disk ) {
                td.index = mdata->disk->index;
                td.total = mdata->disk->total;
            }

            if( ELEMENT_OF(tags,OPT_DISK) )
                td.index = static_cast<uint16_t>( ELEMENT_OF(nums,OPT_DISK) );
            if( ELEMENT_OF(tags,OPT_DISKS) )
                td.total = static_cast<uint16_t>( ELEMENT_OF(nums,OPT_DISKS) );

            MP4TagsSetDisk( mdata, &td );
        }
        
        /* Set the other relevant attributes */
        for ( int i = 0;  i < UCHAR_MAX;  i++ ) {
            if ( tags[i] ) {
                switch ( i ) {
                    case OPT_ALBUM:
                        MP4TagsSetAlbum( mdata, tags[i] );
                        break;
                    case OPT_ARTIST:
                        MP4TagsSetArtist( mdata, tags[i] );
                        break;
                    case OPT_TEMPO:
                    {
                        uint16_t value = static_cast<uint16_t>( nums[i] );
                        MP4TagsSetTempo( mdata, &value );
                        break;
                    }
                    case OPT_COMMENT:
                        MP4TagsSetComments( mdata, tags[i] );
                        break;
                    case OPT_COPYRIGHT:
                        MP4TagsSetCopyright( mdata, tags[i] );
                        break;
                    case OPT_ENCODEDBY:
                        MP4TagsSetEncodedBy( mdata, tags[i] );
                        break;
                    case OPT_TOOL:
                        MP4TagsSetEncodingTool( mdata, tags[i] );
                        break;
                    case OPT_GENRE:
                    {
                      
                      uint16_t genreType = genreIndexFromString( tags[i] );
                      if (genreType) {
                          MP4TagsSetGenre(mdata, NULL);
                          MP4TagsSetGenreType(mdata, &genreType);
                      }
                      else {
                          MP4TagsSetGenreType(mdata, NULL);
                          MP4TagsSetGenre(mdata, tags[i]);
                      }
                      
                      break;
                    }
                    case OPT_GROUPING:
                        MP4TagsSetGrouping( mdata, tags[i] );
                        break;
                    case OPT_HD:
                    {
                        uint8_t value = static_cast<uint8_t>( nums[i] );
                        MP4TagsSetHDVideo( mdata, &value );
                        break;
                    }
                    case OPT_MEDIA_TYPE:
                    {
                        uint8_t st = getMediaKindFromString( tags[i] );
                        //printf("genre: %s, type: %i", tags[i], st);
                        if (st != 0) {
                            MP4TagsSetMediaType( mdata, &st);
                        }                        
                        break;
                    }
                    case OPT_CONTENTID:
                    {
                        uint32_t value = static_cast<uint32_t>( nums[i] );
                        MP4TagsSetContentID( mdata, &value );
                        break;
                    }
                    case OPT_LONGDESC:
                        MP4TagsSetLongDescription( mdata, tags[i] );
                        break;
                    case OPT_GENREID:
                    {
                        uint32_t value = static_cast<uint32_t>( nums[i] );
                        MP4TagsSetGenreID( mdata, &value );
                        break;
                    }
                    case OPT_LYRICS:
                        MP4TagsSetLyrics( mdata, tags[i] );
                        break;
                    case OPT_DESCRIPTION:
                        MP4TagsSetDescription( mdata, tags[i] );
                        break;
                    case OPT_TVEPISODE:
                    {
                        uint32_t value = static_cast<uint32_t>( nums[i] );
                        MP4TagsSetTVEpisode( mdata, &value );
                        break;
                    }
                    case OPT_TVSEASON:
                    {
                        uint32_t value = static_cast<uint32_t>( nums[i] );
                        MP4TagsSetTVSeason( mdata, &value );
                        break;
                    }
                    case OPT_TVNETWORK:
                        MP4TagsSetTVNetwork( mdata, tags[i] );
                        break;
                    case OPT_TVEPISODEID:
                        MP4TagsSetTVEpisodeID( mdata, tags[i] );
                        break;
                    case OPT_PLAYLISTID:
                    {
                        uint64_t value = static_cast<uint64_t>( nums[i] );
                        MP4TagsSetPlaylistID( mdata, &value );
                        break;
                    }
                    case OPT_PICTURE:
                    {
                        File in( tags[i], File::MODE_READ );
                        if( !in.open() ) {
                            MP4TagArtwork art;
                            art.size = (uint32_t)in.size;
                            art.data = malloc( art.size );
                            art.type = MP4_ART_UNDEFINED;

                            File::Size nin;
                            if( !in.read( art.data, art.size, nin ) && nin == art.size ) {
                                if( mdata->artworkCount )
                                    MP4TagsRemoveArtwork( mdata, 0 );
                                MP4TagsAddArtwork( mdata, &art ); 
                            }

                            free( art.data );
                            in.close();
                        }
                        else {
                            fprintf( stderr, "Art file %s not found\n", tags[i] );
                        }
                        break;
                    }
                    case OPT_ALBUM_ARTIST:
                        MP4TagsSetAlbumArtist( mdata, tags[i] );
                        break;
                    case OPT_NAME:
                        MP4TagsSetName( mdata, tags[i] );
                        break;
                    case OPT_TVSHOW:
                        MP4TagsSetTVShow( mdata, tags[i] );
                        break;
                    case OPT_XID:
                        MP4TagsSetXID( mdata, tags[i] );
                        break;
                    case OPT_COMPOSER:
                        MP4TagsSetComposer( mdata, tags[i] );
                        break;
                    case OPT_RELEASEDATE:
                        MP4TagsSetReleaseDate( mdata, tags[i] );
                        break;
                    case OPT_ARTISTID:
                    {
                        uint32_t value = static_cast<uint32_t>( nums[i] );
                        MP4TagsSetArtistID( mdata, &value );
                        break;
                    }
                    case OPT_COMPOSERID:
                    {
                        uint32_t value = static_cast<uint32_t>( nums[i] );
                        MP4TagsSetComposerID( mdata, &value );
                        break;
                    }
                    case OPT_PODCAST:
                    {
                        uint8_t value = static_cast<uint8_t>( nums[i] );
                        MP4TagsSetPodcast(mdata, &value);
                        break;
                    }
                    case OPT_CRATING:
                    {    
                        const char *content_rating = tags[i];
                        char *aux = (char*)malloc(strlen(content_rating) + 1);
                        strcpy(aux, content_rating);
                        const uint8_t rating_index = getContentRatingFromString(aux);
                        
                        if(rating_index < 100) {
                          MP4TagsSetContentRating( mdata, &rating_index);
                        }
                        break;
                    }
                }
            }
        }
        /* Write out all tag modifications, free and close */
        MP4TagsStore( mdata, h );
        MP4TagsFree( mdata );
        
        if ( ELEMENT_OF(tags,OPT_RATING)) {
          char **lista;
          int i = 0;
          lista = (char **) malloc (sizeof (char *));
          char *result = NULL;
          
          MP4ItmfItemList* list = MP4ItmfGetItemsByMeaning(h, "com.apple.iTunes", "iTunEXTC");
          if (list) {
              uint32_t i;
              for (i = 0; i < list->size; i++) {
                  MP4ItmfItem* item = &list->elements[i];
                  MP4ItmfRemoveItem(h, item);
              }
          }
          MP4ItmfItemListFree(list);

          MP4ItmfItem* newItem = MP4ItmfItemAlloc( "----", 1 );
          newItem->mean = strdup( "com.apple.iTunes" );
          newItem->name = strdup( "iTunEXTC" );

          MP4ItmfData* data = &newItem->dataList.elements[0];
          
          const char* ratingString = NULL;
          const char *rating = ELEMENT_OF(tags, OPT_RATING);

          char *aux = (char*)malloc(strlen(rating) + 1);
          strcpy(aux, rating);
          int rating_index = ratingIndexFromString(aux);
          if(rating_index >= 0) {
            ratingString = rating_strings[rating_index].rating;
            char *ratingStringCopy = NULL;
            ratingStringCopy = (char*)malloc(strlen(ratingString) + 1);
            strcpy(ratingStringCopy, ratingString);
            lista[i++] = ratingStringCopy;
            
            if ( ELEMENT_OF(tags,OPT_RANNOTATION)) {
              const char *annotation = ELEMENT_OF(tags,OPT_RANNOTATION);
              char *annotationCopy = (char*)malloc(strlen(annotation) + 1);
              strcpy(annotationCopy, annotation);
              annotationCopy = trimwhitespace(annotationCopy);
              if(strlen(annotationCopy) > 0 ) {
                lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
                lista[i++] = annotationCopy;
              }
            }
            
            result = mkconcat(lista, i);
            result = trimwhitespace(result);
            
            data->typeCode = MP4_ITMF_BT_UTF8;
            data->valueSize = strlen(result);
            data->value = (uint8_t*)malloc( data->valueSize );
            memcpy( data->value, result, data->valueSize );

            MP4ItmfAddItem(h, newItem);
          }
        }
        // else {
        //             MP4ItmfItemList* list = MP4ItmfGetItemsByMeaning(h, "com.apple.iTunes", "iTunEXTC");
        //             if (list) {
        //                 uint32_t i;
        //                 for (i = 0; i < list->size; i++) {
        //                     MP4ItmfItem* item = &list->elements[i];
        //                     MP4ItmfRemoveItem(h, item);
        //                 }
        //             }
        //         }
        
        
        if ( ELEMENT_OF(tags,OPT_CAST) || ELEMENT_OF(tags, OPT_DIRECTOR) ||
             ELEMENT_OF(tags,OPT_CODIRECTOR) || ELEMENT_OF(tags, OPT_PRODUCERS) ||
             ELEMENT_OF(tags,OPT_SWRITERS) || ELEMENT_OF(tags, OPT_COPYWARNING) ||
             ELEMENT_OF(tags,OPT_STUDIO) ) {
           
          char **lista;
          char *token;
          int i = 0;
          lista = (char **) malloc (sizeof (char *));
          char *result = NULL;

          char* plist = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"><plist version=\"1.0\"><dict>";
          lista[i++] = plist;
                  	
        	
          if( ELEMENT_OF(tags,OPT_COPYWARNING) ) {
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "<key>copy-warning</key><string>";

            const char *copy_warning = ELEMENT_OF(tags,OPT_COPYWARNING);
            char *aux = (char*)malloc(strlen(copy_warning) + 1);
            strcpy(aux, copy_warning);
            
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = trimwhitespace(aux);

            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "</string>";
          }
          
          if( ELEMENT_OF(tags,OPT_STUDIO) ) {
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "<key>studio</key><string>";

            const char *studio = ELEMENT_OF(tags,OPT_STUDIO);
            char *aux = (char*)malloc(strlen(studio) + 1);
            strcpy(aux, studio);
            
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = trimwhitespace(aux);

            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "</string>";
          }  
                      
          if( ELEMENT_OF(tags,OPT_CAST) ) {
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "<key>cast</key><array>";

            const char *cast = ELEMENT_OF(tags,OPT_CAST);
            char *aux = (char*)malloc(strlen(cast) + 1);
            strcpy(aux, cast);
   
            token = strtok(aux, ",");

            while(token != NULL) {
              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "<dict><key>name</key><string>";

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = trimwhitespace(token);

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "</string></dict>";

              token = strtok(NULL, ",");
            }

            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "</array>";

            token = NULL;
          }
          
          if( ELEMENT_OF(tags,OPT_DIRECTOR) ) {
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "<key>directors</key><array>";

            const char *directors = ELEMENT_OF(tags,OPT_DIRECTOR);
            char *aux = (char*)malloc(strlen(directors) + 1);
            strcpy(aux, directors);
   
            token = strtok(aux, ",");

            while(token != NULL) {
              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "<dict><key>name</key><string>";

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = trimwhitespace(token);

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "</string></dict>";

              token = strtok(NULL, ",");
            }

            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "</array>";

            token = NULL;
          }
          
          if( ELEMENT_OF(tags,OPT_CODIRECTOR) ) {
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "<key>codirectors</key><array>";

            const char *codirectors = ELEMENT_OF(tags,OPT_CODIRECTOR);
            char *aux = (char*)malloc(strlen(codirectors) + 1);
            strcpy(aux, codirectors);
   
            token = strtok(aux, ",");

            while(token != NULL) {
              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "<dict><key>name</key><string>";

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = trimwhitespace(token);

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "</string></dict>";

              token = strtok(NULL, ",");
            }

            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "</array>";

            token = NULL;
          }
          
          if( ELEMENT_OF(tags,OPT_PRODUCERS) ) {
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "<key>producers</key><array>";

            const char *producers = ELEMENT_OF(tags,OPT_PRODUCERS);
            char *aux = (char*)malloc(strlen(producers) + 1);
            strcpy(aux, producers);
   
            token = strtok(aux, ",");

            while(token != NULL) {
              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "<dict><key>name</key><string>";

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = trimwhitespace(token);

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "</string></dict>";

              token = strtok(NULL, ",");
            }

            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "</array>";

            token = NULL;
          }
          
          if( ELEMENT_OF(tags,OPT_SWRITERS) ) {
            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "<key>screenwriters</key><array>";

            const char *screenwriters = ELEMENT_OF(tags,OPT_SWRITERS);
            char *aux = (char*)malloc(strlen(screenwriters) + 1);
            strcpy(aux, screenwriters);
   
            token = strtok(aux, ",");

            while(token != NULL) {
              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "<dict><key>name</key><string>";

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = trimwhitespace(token);

              lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
              lista[i++] = "</string></dict>";

              token = strtok(NULL, ",");
            }

            lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
            lista[i++] = "</array>";

            token = NULL;
          }
          
          lista = (char **)realloc(lista, sizeof(char*) * (i + 1));
          lista[i++] = "</dict></plist>";
          result = mkconcat(lista, i);

          if(result != NULL) {
            MP4ItmfItemList* list = MP4ItmfGetItemsByMeaning(h, "com.apple.iTunes", "iTunMOVI");
            
            if (list) {
              uint32_t i;
              for (i = 0; i < list->size; i++) {
                MP4ItmfItem* item = &list->elements[i];
                MP4ItmfRemoveItem(h, item);
              }
            }
            
            MP4ItmfItemListFree(list);

            MP4ItmfItem* newItem = MP4ItmfItemAlloc( "----", 1 );
            newItem->mean = strdup( "com.apple.iTunes" );
            newItem->name = strdup( "iTunMOVI" );

            MP4ItmfData* data = &newItem->dataList.elements[0];
            data->typeCode = MP4_ITMF_BT_UTF8;
            data->valueSize = strlen(result);
            data->value = (uint8_t*)malloc( data->valueSize );
            memcpy( data->value, result, data->valueSize );

            MP4ItmfAddItem(h, newItem);
            free(result);
          }
        } 
        // else {
        //     MP4ItmfItemList* list = MP4ItmfGetItemsByMeaning(h, "com.apple.iTunes", "iTunMOVI");
        //     if (list) {
        //         uint32_t i;
        //         for (i = 0; i < list->size; i++) {
        //             MP4ItmfItem* item = &list->elements[i];
        //             MP4ItmfRemoveItem(h, item);
        //         }
        //     }
        // }
        
        
        MP4Close( h );
    } /* end while optind < argc */
    return 0;
}

char *mkconcat(char **list, int max) {
 char *result = NULL;
 int i = 0, len = 0;

 /* calc. total size needed ... */
 for(i = 0; i < max; i++)
  len += (strlen(list[i]) /*+ 1*/);
  
  //printf("len: %i, max: %i\n", len, max);

 /* alloc sufficient mem ... */ 
 result = (char*)malloc(len * sizeof(char) + 1);
 if(result == NULL) {
  fprintf(stderr, "Error - mkconcat -> malloc()\n");
  return NULL;
 }

 /* concatenate strings */
 for(i = 0; i < max; i++) {
   if(i == 0)
     strcpy(result, list[i]);
   else
     strcat(result, list[i]);
 }
 //printf("result: %s\n", result);
 return result;
}

char *trimwhitespace(char *str) {
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

int ratingIndexFromString( char *ratingString) {
    int ratingIndex = -1;
    iTMF_rating_t *ratingList;
    int k = 0;
    for ( ratingList = (iTMF_rating_t*) rating_strings; ratingList->english_name; ratingList++, k++ ) {
        if(strcmp(ratingString, ratingList->english_name) == 0) {
            ratingIndex = k;
            break;
          }
    }
    return ratingIndex;
}

uint8_t getContentRatingFromString(char *contentRatingString) {
    contentRating_t *contentRatingList;
    for ( contentRatingList = (contentRating_t*) contentRating_strings; contentRatingList->english_name; contentRatingList++) {
        if ( strcmp(contentRatingString, contentRatingList->english_name) == 0) {
          return contentRatingList->rtng;     
        }
    }
    return 100;
}

uint16_t genreIndexFromString(const char* genreString) {
    uint16_t genreIndex = 0;
    genreType_t *genreList;
    int k = 0;
    for ( genreList = (genreType_t*) genreType_strings; genreList->english_name; genreList++, k++ ) {
        if( strcmp(genreString, genreList->english_name) == 0)
            genreIndex = k + 1;
    }
    return genreIndex;
}

uint8_t getMediaKindFromString(const char* mediaKindString) {
    uint8_t mediaKind = 0;
    mediaKind_t *mediaKindList;
    for (mediaKindList = (mediaKind_t*) mediaKind_strings; mediaKindList->english_name; mediaKindList++) {
        if( strcmp(mediaKindString, mediaKindList->english_name) == 0) {
            mediaKind = mediaKindList->stik;
        }
    }
    return mediaKind;
}