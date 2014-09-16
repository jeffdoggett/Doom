/* Acorn RiscOS specific declarations */
/* Doom Port By J.A.Doggett. */


#ifndef ACORN_H
#define ACORN_H

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>

#define	F_OK		0	/* test for existence of file */
#define	X_OK		0x01	/* test for execute or search permission */
#define	W_OK		0x02	/* test for write permission */
#define	R_OK		0x04	/* test for read permission */



void set_riscos_filetype (const char * file, unsigned int type);

int access(const char * filename, int mode);
int mkdir (const char * directory_name, mode_t perms);
int strcasecmp (const char * s1, const char * s2);
int strncasecmp (const char * s1, const char * s2, unsigned int max);
char * strdup (const char * string);
void usleep (unsigned int time);
int RiscOs_expand_path (const char * path_name);

#endif
