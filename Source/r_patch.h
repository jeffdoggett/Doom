
#if !defined(__R_PATCH_H__)
#define __R_PATCH_H__

typedef struct
{
    int                 topdelta;
    int                 length;
} rpost_t;

typedef struct
{
    int                 numPosts;
    rpost_t             *posts;
    unsigned char       *pixels;
} rcolumn_t;

typedef struct
{
    int                 width;
    int                 height;
    unsigned            widthmask;

    // this is the single malloc'ed/free'd array
    // for this patch
    unsigned char       *data;

    // these are pointers into the data array
    unsigned char       *pixels;
    rcolumn_t           *columns;
    rpost_t             *posts;

    unsigned int        locks;
} rpatch_t;

rpatch_t *R_CacheTextureCompositePatchNum(int id);
void R_UnlockTextureCompositePatchNum(int id);

rcolumn_t *R_GetPatchColumnWrapped(rpatch_t *patch, int columnIndex);
rcolumn_t *R_GetPatchColumnClamped(rpatch_t *patch, int columnIndex);

void R_InitPatches(void);

#endif
