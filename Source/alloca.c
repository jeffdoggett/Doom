
#include <stdio.h>
#include <stdlib.h>

/* --------------------------------------------------------------------------------- */

/* On linux/bsd systems alloca is a clever memory claiming routine that claims memory */
/* on the stack. Hence when the function exits, the claimed memory gets cleaned */
/* up automatically. Since I'm not a clever enough programmer, I'll have to make */
/* some sort of bodge as usual! */

typedef struct alloca_store_s
{
  unsigned int stack_pos;
  unsigned int age;
  struct alloca_store_s * next;
} alloca_store_t;

static alloca_store_t * alloca_head = 0;

void * alloca (unsigned int size)
{
  unsigned int sp;
  alloca_store_t * as;
  alloca_store_t ** pas;

  sp = (unsigned int) &sp;

  /* First see whether this function has claimed memory */
  /* in the past and release it. */

  pas = &alloca_head;
  as = *pas;

  while (as)
  {
    if (as -> age)  as -> age--;
    if ((as -> age == 1)
     || (as -> stack_pos == sp))
    {
      /* printf ("Freed previous memory\n"); */
      *pas = as -> next;
      free (as);
    }
    else
    {
      pas = &(as -> next);
    }
    as = *pas;
  }

  /* printf ("Alloca called sp = %X, amount = %X, record = %d\n", sp, size, c); */

  as = malloc (size + (sizeof(alloca_store_t)));
  if (as == 0)
  {
    return (0);
  }

  as -> stack_pos = sp;
  as -> age = 100;
  as -> next = alloca_head;
  alloca_head = as;

  return ((char *) as + (sizeof(alloca_store_t)));
}

/* --------------------------------------------------------------------------------- */
