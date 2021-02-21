/*  dh_stuff.h */
/*  Support for dehacker scripts */

#ifndef __DHSTUFF_H__
#define __DHSTUFF_H__

void DH_parse_hacker_file_f (const char * filename, FILE * fin, unsigned int filetop_pos);
void DH_parse_language_file_f (FILE * fin, size_t filesize);
void DH_replace_file_extension (char * newname, const char * oldname, const char * n_ext);
void DH_parse_hacker_file (const char * filename);
void DH_parse_hacker_wad_file (const char * wadname, boolean do_it);
unsigned int dh_instr (const char * text, const char * search_string);
unsigned int dh_inchar (const char * text, char search_char);
unsigned int dh_rinchar (const char * text, char search_char);
unsigned int dh_qty_match (const char * s1, const char * s2);
char * dh_split_lines (char * ptr, char * top);
char * dh_next_line (char * ptr, char * top);
void dh_fgets (char * a_line, unsigned int max_length, FILE * fin);
void Load_Mapinfo (void);
void Change_To_Mapinfo (FILE * fin);
void Change_To_UMapinfo (FILE * fin);
void DH_pistol_start (void);
void DH_remove_duplicate_mapinfos (void);
void Load_Lockdefs (void);

#if 0
int dh_strcmp (const char * s1, const char * s2);
#else
#define dh_strcmp(s1,s2) strncasecmp(s1, s2, strlen(s2))
#endif

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
