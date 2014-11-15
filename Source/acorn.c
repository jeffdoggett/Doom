/* Acorn UNIX-like drivers */

/* Unixlib doesn't quite do what I want, so some functions
   are recreated here.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <kernel.h>
#include <string.h>
#include <dirent.h>
#include "d_main.h"
#include "acorn.h"
#define OS_ReadMonotonicTime	0x42
#define OS_ReadMemMapInfo	0x51

/* ---------------------------------------------------------------------------- */

/* Do not remove this - Doom doesn't seem to like the one in unixlib */

int gettimeofday (struct timeval *tv, struct timezone *tzp)
{
  _kernel_swi_regs regs;

  _kernel_swi (OS_ReadMonotonicTime, &regs, &regs);
  tv -> tv_sec  = (long) regs.r[0] / 100;
  tv -> tv_usec = ((long) regs.r[0] % 100) * 10000;
  return (0);
}

/* ---------------------------------------------------------------------------- */

void usleep (unsigned int time)
{
  unsigned int ft;
  _kernel_swi_regs regs;

  _kernel_swi (OS_ReadMonotonicTime, &regs, &regs);
  ft = regs.r[0];

  do
  {
    _kernel_swi (OS_ReadMonotonicTime, &regs, &regs);
  } while ((regs.r[0] - ft) < (time / 10000));
}

/* ---------------------------------------------------------------------------- */
#ifndef F_OK
#define	F_OK		0	/* test for existence of file */
#define	X_OK		0x01	/* test for execute or search permission */
#define	W_OK		0x02	/* test for write permission */
#define	R_OK		0x04	/* test for read permission */
#endif

int access (const char * filename, int mode)
{
  _kernel_osfile_block block;
  int rc;

  // printf ("Access called - %s\n", filename);

  rc = _kernel_osfile (17, filename, &block);
  if (rc == 0)  return (-1);

  /* block.end contains the file attributes */
  if ((mode & R_OK)
   && ((block.end & 0x01) == 0))
    return (-1);

  if ((mode & W_OK)
   && ((block.end & 0x02) == 0))
    return (-1);

  // printf ("Access returned OK\n");
  return (0);
}

/* ---------------------------------------------------------------------------- */

void set_riscos_filetype (const char * file, unsigned int type)
{
  _kernel_osfile_block osfile;

  osfile.load = type;
  _kernel_osfile (18,file,&osfile);
}

/* ---------------------------------------------------------------------------- */

int mkdir (const char * directory_name, mode_t perms)
{
  _kernel_osfile_block osfile;
  int type;

  /* ----- Get file catalogue info ----- */
  type = _kernel_osfile (17,directory_name,&osfile);

  switch (type)
  {
    case 0:  /* Does not exist */
#ifdef DEBUG
      fprintf (stderr, "Making directory '%s'\n", directory_name);
#endif
      osfile.load = 0;
      osfile.exec = 0;
      osfile.start = 0;
      osfile.end = 0;
      type = _kernel_osfile (8,directory_name,&osfile);
      break;

    case 1:  /* File exists */
      fprintf (stderr, "Cannot create directory '%s' because it exists as a file.\n", directory_name);

  } /* of switch */

  return (0);
}

/* --------------------------------------------------------------------------------- */
#if 0
void make_savegame_dir (const char * path_name)
{
  char * dir_name;

  dir_name = malloc (strlen (path_name) + 1);
  if (dir_name)
  {
    dirname (dir_name, path_name);
    mkdir (dir_name, 0755);
    free (dir_name);
  }
}
#endif
/* --------------------------------------------------------------------------------- */

/* Do not remove this - Doom doesn't seem to like the one in unixlib */

int strcasecmp (const char * s1, const char * s2)
{
  char c,c1;
  int result;

  /* printf ("Compared %s with %s", s1, s2); */

  do
  {
    c = *s1++;
    c1 = tolower(c);
    c = *s2++;
    result = c1 - tolower (c);
  } while ((result == 0) && (c1));

  /* printf (" and returned %d\n", result); */
  return (result);
}

/* --------------------------------------------------------------------------------- */

/* Do not remove this - Doom doesn't seem to like the one in unixlib */

int strncasecmp (const char * s1, const char * s2, unsigned int max)
{
  char c,c1;
  int result;

  /* printf ("Compared %s with %s (%d)", s1, s2, max); */

  if (max == 0)
    return (0);

  do
  {
    c = *s1++;
    c1 = tolower(c);
    c = *s2++;
    result = c1 - tolower (c);
  } while ((result == 0) && (c1) && (--max));

  /* printf (" and returned %d\n", result); */
  return (result);
}

/* ------------------------------------------------------------------------ */

char * strdup (const char * string)
{
  char * rc;

  rc = malloc (strlen (string) + 1);
  if (rc)
    strcpy (rc, string);

  return (rc);
}

/* ------------------------------------------------------------------------ */

static void gs_trans (char * dest, const char * source)
{
  _kernel_swi_regs regs;

  regs.r[0] = (int) source;
  regs.r[1] = (int) dest;
  regs.r[2] = 1024;
  _kernel_swi (0x27, &regs, &regs);
}

/* ------------------------------------------------------------------- */

static int Recurse_Dir (const char *dirname, const char * leafname, const char * next)
{
  unsigned int pos;
  unsigned int pp;
  int rc;
  char cc;
  _kernel_osgbpb_block osgbpb_block;
  char filename [260];
  char * fullpath;
  char * next_1;
  char * next_leaf;

  // printf ("Scanning dir %s for %s - left = %s\n", dirname, leafname, next);
  pos = 0;
  rc = 0;
  do
  {
    do
    {
      osgbpb_block.dataptr  = filename;
      osgbpb_block.nbytes   = 1;
      osgbpb_block.fileptr  = pos;
      osgbpb_block.buf_len  = 250;
      osgbpb_block.wild_fld = (char*)leafname;

      _kernel_osgbpb (9, (unsigned) dirname, &osgbpb_block);
      pos = osgbpb_block.fileptr;
    } while ((osgbpb_block.nbytes == 0) && (pos != -1));

    if (osgbpb_block.nbytes == 0)
      pos = 0;

    if (pos)
    {
      if ((next == 0) || (next[0] == 0))
      {
        fullpath = malloc (strlen (dirname) + strlen (filename) + 4);
        if (fullpath)
        {
          strcpy (fullpath, dirname);
          strcat (fullpath, ".");
          strcat (fullpath, filename);
          next_1 = fullpath;
          if ((next_1 [0] == '@')
           && (next_1 [1] == '.'))
            next_1 += 2;
          // printf ("Added %s\n", next_1);
          D_AddFile (next_1);
          free (fullpath);
          rc++;
        }
      }
      else
      {
        fullpath = malloc (strlen (dirname) + strlen (filename) + strlen (next) + 4);
        if (fullpath)
        {
          strcpy (fullpath, dirname);
          strcat (fullpath, ".");
          strcat (fullpath, filename);

          next_1 = (char*)next;
          next_leaf = fullpath + strlen (fullpath) + 2;
          pp = 0;
          do
          {
            cc = *next_1;
            if (cc)
            {
              next_1++;
              if (cc == '.')
                cc = 0;
            }
            next_leaf [pp++] = cc;
          } while (cc);
          rc += Recurse_Dir (fullpath, next_leaf, next_1);
          free (fullpath);
        }
      }
    }
  } while (pos);
  return (rc);
}

/* ------------------------------------------------------------------- */

int RiscOs_expand_path (const char * path_name)
{
  char cc;
  unsigned int pos;
  char * ptr;
  char * dirname;
  char * leafname;
  char * pathname;
  char * pathname2;

  pos = strlen (path_name) + 200;
  pathname = malloc (pos);
  if (pathname == 0)
    return (0);

  gs_trans (pathname, path_name);

  pos = strlen (pathname) + 10;
  dirname = malloc (pos);
  if (dirname == 0)
  {
    free (pathname);
    return (0);
  }

  leafname = malloc (pos);
  if (leafname == 0)
  {
    free (pathname);
    free (dirname);
    return (0);
  }

  strcpy (dirname, "@");
  pathname2 = pathname;
  pos = 0;                  // Look first for "$." or "@."
  do
  {
    cc = pathname2 [pos];
    if (((cc == '$') || (cc == '@'))
     && (pathname2 [pos+1] == '.'))
    {
      pos++;
      strncpy (dirname, pathname2, pos);
      dirname [pos] = 0;
      pathname2 += (pos+1);
      cc = 0;
    }
    pos++;
  } while (cc);


  ptr = leafname;
  do
  {
    cc = *pathname2;
    if (cc)
    {
      pathname2++;
      if (cc == '.')
        cc = 0;
    }
    *ptr++ = cc;
  } while (cc);

  // printf ("dirname = %s, leafname = %s, pathname2 = %s\n", dirname, leafname, pathname2);
  pos = Recurse_Dir (dirname, leafname, pathname2);

  free (pathname);
  free (leafname);
  free (dirname);
  return (pos);
}

/* ------------------------------------------------------------------- */

DIR * opendir (const char * dirname)
{
  DIR * dir;
  unsigned int length;
  _kernel_osfile_block blk;
  int rc;

  rc = _kernel_osfile (17, dirname, &blk);

  switch (rc)
  {
    case 2: /* Ordinary directory */
    case 3: /* Image file */

      length = (sizeof (DIR)) + (sizeof (struct dirent)) + (strlen (dirname)) + 4;

      dir = (DIR *) malloc (length);
      if (dir)
      {
        memset ((unsigned int *) dir,0, length);

        length = (unsigned int) dir;
        length += sizeof (DIR);
        dir -> dd_buf = (char *) length;
        length += sizeof (struct dirent);
        dir -> dd_fd = length;

        strcpy ((char *) length, dirname);
      }
      break;

    default:
      dir = 0;
  }

  return (dir);
}

/* ------------------------------------------------------------------- */

struct dirent *readdir (DIR * dir)
{
//int rc;
  int next;
  struct dirent * dire;
  _kernel_osgbpb_block osgbpb_block;

  if (dir -> dd_seek != -1)			// From the previous call?
  {
    do
    {
      dire = (struct dirent *) dir -> dd_buf;

//    dir -> dd_fd					// R1 Pointer to directory name
      osgbpb_block.dataptr  = dire -> d_name;		// R2 Pointer to buffer
      osgbpb_block.nbytes   = 1;			// R3 Number of objects to read
      osgbpb_block.fileptr  = (int) dir -> dd_seek;	// R4 Where to start, 0 for first
      osgbpb_block.buf_len  = sizeof (dire -> d_name);	// R5 Length of buffer
      osgbpb_block.wild_fld = 0;			// R6 Wildcarded name to match (0 = "*")

      /* rc = */ _kernel_osgbpb (9, dir -> dd_fd, &osgbpb_block);

      dir -> dd_seek = (size_t) (next = osgbpb_block.fileptr);
      if (osgbpb_block.nbytes)
	return (dire);

      /* Finished when R4 = -1 */
    } while (next != -1);
  }  
  return (0);
}

/* ------------------------------------------------------------------- */

long telldir (const DIR * dir)
{
  return (dir -> dd_seek);
}

/* ------------------------------------------------------------------- */

void seekdir (DIR * dir, long position)
{
  dir -> dd_seek = (size_t) position;
}

/* ------------------------------------------------------------------- */

void rewinddir (DIR * dir)
{
  dir -> dd_seek = (size_t) 0;
}

/* ------------------------------------------------------------------- */

int closedir (DIR * dir)
{
  free (dir);
  return (0);
}

/* ------------------------------------------------------------------- */
#define TIME_O_HI 0x33
#define TIME_O_LO 0x6E996A00

/* ---------------------------------------------------------------------------- */

static unsigned int conv_arctime_to_unixtime (unsigned int hi, unsigned int lo)
{
  unsigned int unix_time;

  /* Must subtract TIME_O and then divide by 100 */

  if (hi > TIME_O_HI)
  {
    if (lo < TIME_O_LO)
    {
      hi -= 1;
    }
    lo -= TIME_O_LO;
    hi -= TIME_O_HI;
    unix_time = 0;
    while (hi)
    {
      if (lo < 2147482300)
      {
        hi--;
      }
      lo -= 2147482300;
      unix_time += 21474823;
    }
    lo = (lo / 100) + unix_time;
  }
  else
  {
    lo = 60 * 60;
  }

  unix_time = lo;

  return (unix_time);

}

/* --------------------------------------------------------------------------- */

static void clear_memory (unsigned int * ptr, unsigned int qty)
{
  while (qty > 3)
  {
    *ptr++ = 0;
    qty -= 4;
  }
}

/* --------------------------------------------------------------------------- */

int stat (const char * file_name, struct stat * file_stat)
{
  _kernel_osfile_block osfile_blk;
  int rc;
  int unix_time;

  // printf ("Statting %s", file_name);

  clear_memory ((unsigned int *) file_stat, sizeof(struct stat));

  rc = _kernel_osfile (17, file_name, &osfile_blk);
  if ((rc >= 1) && (rc <= 3))
  {
    file_stat -> st_size = osfile_blk.start;
    if (osfile_blk.end & 0x01)  file_stat -> st_mode |= S_IREAD;
    if (osfile_blk.end & 0x02)  file_stat -> st_mode |= S_IWRITE;
    if (osfile_blk.end & 0x10)  file_stat -> st_mode |= 0044;
    if (osfile_blk.end & 0x20)  file_stat -> st_mode |= 0022;
    if (rc == 1)
                                file_stat -> st_mode |= S_IFREG;
    else
                                file_stat -> st_mode |= S_IFDIR;
    unix_time = conv_arctime_to_unixtime (osfile_blk.load & 0xFF, osfile_blk.exec);
    file_stat -> st_atime = unix_time;
    file_stat -> st_mtime = unix_time;
    file_stat -> st_ctime = unix_time;
    file_stat -> st_nlink = 1;
    file_stat -> st_uid = 200;
    file_stat -> st_gid = 21;
    file_stat -> st_blksize = 512;

    /* file_stat -> st_blocks  = osfile_blk.start / 512;   */
    //errno = 0;
    rc = 0;
  }
  else
  {
    //errno = ENOENT;
    rc = 1;
  }
  // printf (" and returned %d (%X)\n", rc, file_stat -> st_mode);
  return (rc);
}

/* --------------------------------------------------------------------------- */

