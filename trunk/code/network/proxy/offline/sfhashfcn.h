/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
/*
	sfhashfcn.h
*/
#ifndef SFHASHFCN_INCLUDE 
#define SFHASHFCN_INCLUDE 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>


typedef struct _SFHASHFCN {

 unsigned seed;
 unsigned scale;
 unsigned hardener;
 unsigned (*hash_fcn)(struct _SFHASHFCN * p,
                      unsigned char *d,
                      int n );
 int      (*keycmp_fcn)( const void *s1,
                         const void *s2,
                         size_t n);
} SFHASHFCN;

SFHASHFCN * sfhashfcn_new( int nrows );
void sfhashfcn_free( SFHASHFCN * p );
void sfhashfcn_static( SFHASHFCN * p );

unsigned sfhashfcn_hash( SFHASHFCN * p, unsigned char *d, int n );

int sfhashfcn_set_keyops( SFHASHFCN * p,
                          unsigned (*hash_fcn)( SFHASHFCN * p,
                                                unsigned char *d,
                                                int n),
                          int (*keycmp_fcn)( const void *s1,
                                             const void *s2,
                                             size_t n));



#endif
/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
/*
*
*  sfghash.h
*
*  generic hash table - stores and maps key + data pairs
*
*  Author: Marc Norton
*
*/

#ifndef _SFGHASH_
#define _SFGHASH_

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sfhashfcn.h"

/*
*   ERROR DEFINES
*/
#define SFGHASH_NOMEM    -2
#define SFGHASH_ERR      -1
#define SFGHASH_OK        0
#define SFGHASH_INTABLE   1

/* 
*  Flags for ghash_new: userkeys 
*/
#define GH_COPYKEYS 0
#define GH_USERKEYS 1

/*
*   Generic HASH NODE
*/
typedef struct _sfghash_node
{
  struct _sfghash_node * next, * prev;

  void * key;   /* Copy of, or Pointer to, the Users key */
  void * data;  /* Pointer to the users data, this is never copied! */
     
} SFGHASH_NODE;

/*
*    Generic HASH table
*/
typedef struct _sfghash
{
  SFHASHFCN    * sfhashfcn;
  int          keysize;   /* bytes in key, if < 0 -> keys are strings */
  int          userkey;   /* user owns the key */

  SFGHASH_NODE ** table;  /* array of node ptr's */
  int             nrows;  /* # rows int the hash table use a prime number 211, 9871 */

  unsigned       count;  /* total # nodes in table */

  void         (*userfree)( void * );  

  int            crow;    // findfirst/next row in table
  SFGHASH_NODE * cnode; // findfirst/next node ptr

  int splay;

} SFGHASH, SFDICT;


/*
*   HASH PROTOTYPES
*/
SFGHASH * sfghash_new( int nrows, int keysize, int userkeys, void (*userfree)(void*p) );
void      sfghash_delete( SFGHASH * h );
int       sfghash_add ( SFGHASH * h, void * key, void * data );
int       sfghash_remove( SFGHASH * h, void * key);
int       sfghash_count( SFGHASH * h);
void    * sfghash_find( SFGHASH * h, void * key );
SFGHASH_NODE * sfghash_findfirst( SFGHASH * h );
SFGHASH_NODE * sfghash_findnext ( SFGHASH * h );
void sfghash_splaymode( SFGHASH * t, int n );

int sfghash_set_keyops( SFGHASH *h ,
                        unsigned (*hash_fcn)( SFHASHFCN * p,
                                              unsigned char *d,
                                              int n),
                        int (*keycmp_fcn)( const void *s1,
                                           const void *s2,
                                           size_t n));


#endif

/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
/*
*  sflsq.h
*
*  Simple LIST, STACK, QUEUE DICTIONARY(LIST BASED)interface
*
*  All of these functions are based on lists, which use
*  the standard malloc.
*
*  Note that NODE_DATA can be redifined with the
*  define below.
*
*  Author: Marc Norton
*/
#ifndef _SFLSQ_
#define _SFLSQ_

/*
*  
*/
typedef void * NODE_DATA;

/*
*    Simple list,stack or queue NODE
*/ 
typedef struct sf_lnode
{
  struct sf_lnode *next;
  struct sf_lnode *prev;
  NODE_DATA      ndata;
}
SF_QNODE,SF_SNODE,SF_LNODE;


/*
*	Integer Stack - uses an array from the subroutines stack
*/
typedef struct {
 unsigned *stack;
 int nstack;
 int n;
 int imalloc;
}
SF_ISTACK;
/*
*	Pointer Stack - uses an array from the subroutines stack
*/
typedef struct {
 void **stack;
 int nstack;
 int n;
 int imalloc;
}
SF_PSTACK;


/*
*  Simple Structure for Queue's, stacks, lists
*/ 
typedef struct sf_list
{
  SF_LNODE *head, *tail;  
  SF_LNODE *cur;  /* used for First/Next walking */
  int       count;
}
SF_QUEUE,SF_STACK,SF_LIST;



/*
*  Linked List Interface
*/ 
SF_LIST * sflist_new ( void ); 
void      sflist_init ( SF_LIST * s); 
int       sflist_add_tail ( SF_LIST* s, NODE_DATA ndata );
int       sflist_add_head ( SF_LIST* s, NODE_DATA ndata );
int       sflist_add_before ( SF_LIST* s, SF_LNODE * lnode, NODE_DATA ndata );
int       sflist_add_after ( SF_LIST* s, SF_LNODE * lnode, NODE_DATA ndata );
NODE_DATA sflist_remove_head ( SF_LIST * s);
NODE_DATA sflist_remove_tail ( SF_LIST * s); 
void      sflist_remove_node (SF_LIST * s, SF_LNODE * n, void (*free)(void*) );
int       sflist_count ( SF_LIST* s); 
NODE_DATA sflist_first( SF_LIST * s);
NODE_DATA sflist_next( SF_LIST * s);
SF_LNODE * sflist_first_node( SF_LIST * s );
SF_LNODE * sflist_next_node( SF_LIST * s );
NODE_DATA sflist_firstpos( SF_LIST * s, SF_LNODE ** v );
NODE_DATA sflist_nextpos ( SF_LIST * s, SF_LNODE ** v );
void      sflist_free ( SF_LIST * s); 
void      sflist_free_all( SF_LIST * s, void (*free)(void*) ); 

/*
*   Stack Interface ( LIFO - Last in, First out ) 
*/
SF_STACK *sfstack_new ( void ); 
void      sfstack_init ( SF_STACK * s); 
int       sfstack_add( SF_STACK* s, NODE_DATA ndata ); 
NODE_DATA sfstack_remove ( SF_STACK * s);
int       sfstack_count ( SF_STACK * s); 
void      sfstack_free ( SF_STACK * s); 
void      sfstack_free_all( SF_STACK* s, void (*free)(void*) ); 

/*
*   Queue Interface ( FIFO - First in, First out ) 
*/
SF_QUEUE *sfqueue_new ( void ); 
void      sfqueue_init ( SF_QUEUE * s); 
int       sfqueue_add( SF_QUEUE * s, NODE_DATA ndata ); 
NODE_DATA sfqueue_remove ( SF_QUEUE * s);
int       sfqueue_count ( SF_QUEUE * s); 
void      sfqueue_free ( SF_QUEUE * s); 
void      sfqueue_free_all( SF_QUEUE* s, void (*free)(void*) ); 

/*
* Performance Stack functions for Integer/Unsigned and Pointers, uses
* user provided array storage, perhaps from the program stack or a global.
* These are efficient, and use no memory functions.
*/
int sfistack_init( SF_ISTACK * s, unsigned * a,  int n  );
int sfistack_push( SF_ISTACK *s, unsigned value);
int sfistack_pop(  SF_ISTACK *s, unsigned * value);

int sfpstack_init( SF_PSTACK * s, void ** a,  int n  );
int sfpstack_push( SF_PSTACK *s, void * value);
int sfpstack_pop(  SF_PSTACK *s, void ** value);

#endif
/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
/*
**  sfmemcap.h
*/
#ifndef __SF_MEMCAP_H__
#define __SF_MEMCAP_H__

typedef struct
{
   unsigned memused;
   unsigned memcap;
   int      nblocks;

}MEMCAP;

void     sfmemcap_init(MEMCAP * mc, unsigned nbytes);
MEMCAP * sfmemcap_new( unsigned nbytes );
void     sfmemcap_delete( MEMCAP * mc );
void   * sfmemcap_alloc(MEMCAP * mc, unsigned nbytes);
void     sfmemcap_showmem(MEMCAP * mc );
void     sfmemcap_free( MEMCAP * mc, void * memory);
char   * sfmemcap_strdup(MEMCAP * mc, const char *str);
void   * sfmemcap_dupmem(MEMCAP * mc, void * src, int n );

#endif
/****************************************************************************
 *
 * Copyright (C) 2006-2008 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#ifndef SF_PRIME_TABLE
#define SF_PRIME_TABLE

int sf_nearest_prime( int n );

#endif
/* $Id$ */
/*
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Copyright (C) 2002 Martin Roesch <roesch@sourcefire.com>
**
** This is hi
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef _SF_SDLIST
#define _SF_SDLIST

/* based off Linked List structure p. 57  _Mastering algorithms in C_
 *
 * Differs from sf_list by using static listitem blocks.
 *
 * Use mempool as the interface to this code instead of trying to use it directly
 * 
 */

typedef struct _SDListItem {
    void *data;
    struct _SDListItem *next;
    struct _SDListItem *prev;
} SDListItem;


typedef struct sfSDList {
    int size;
    SDListItem *head;
    SDListItem *tail;
    void (*destroy)(void *data); /* delete function called for each
                                    member of the linked list */
} sfSDList;


/* initialize a DList */
int sf_sdlist_init(sfSDList *list, void (*destroy)(void *data));

/* delete an DList */
int sf_sdlist_delete(sfSDList *list);

/* insert item, putting data in container */
int sf_sdlist_insert_next(sfSDList *list, SDListItem *item, void *data,
                          SDListItem *container);

/* remove the item after the item */
int sf_sdlist_remove_next(sfSDList *list, SDListItem *item);

/* remove this item from the list */
int sf_sdlist_remove(sfSDList *list, SDListItem *item);

/* append at the end of the list */
int sf_sdlist_append(sfSDList *list, void *data, SDListItem *container);

void print_sdlist(sfSDList *list);

#endif /* _SF_DLIST */
/****************************************************************************
 *
 * Copyright (C) 2003-2008 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/
 
/*
*
*  sfxhash.h
*
*  generic hash table - stores and maps key + data pairs
*  (supports memcap and automatic memory recovery when out of memory)
*
*  Author: Marc Norton
*
*/

#ifndef _SFXHASH_
#define _SFXHASH_

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sfhashfcn.h"
/*
*   ERROR DEFINES
*/
#define SFXHASH_NOMEM    -2
#define SFXHASH_ERR      -1
#define SFXHASH_OK        0
#define SFXHASH_INTABLE   1

/*
*   HASH NODE
*/
typedef struct _sfxhash_node
{
  struct _sfxhash_node * gnext, * gprev; /* global node list - used for ageing nodes */
  struct _sfxhash_node * next,  * prev;  /* row node list */

  int    rindex; /* row index of table this node belongs to. */

  void * key;   /* Pointer to the key. */
  void * data;  /* Pointer to the users data, this is not copied ! */
     
} SFXHASH_NODE;

/*
*    SFGX HASH Table
*/
typedef struct _sfxhash
{
  SFHASHFCN     * sfhashfcn; /* hash function */
  int             keysize; /* bytes in key, if <= 0 -> keys are strings */
  int             datasize;/* bytes in key, if == 0 -> user data */
  SFXHASH_NODE ** table;   /* array of node ptr's */
  unsigned        nrows;   /* # rows int the hash table use a prime number 211, 9871 */
  unsigned        count;   /* total # nodes in table */
  
  unsigned        crow;    // findfirst/next row in table
  SFXHASH_NODE  * cnode;   // findfirst/next node ptr
  int             splay;

  unsigned        max_nodes;
  MEMCAP          mc;
  unsigned        overhead_bytes;  /** # of bytes that will be unavailable for nodes inside the table */    
  unsigned        overhead_blocks; /** # of blocks consumed by the table */
  unsigned        find_fail;
  unsigned        find_success;
    
  SFXHASH_NODE  * ghead, * gtail;  /* global - root of all nodes allocated in table */

  SFXHASH_NODE  * fhead, * ftail;  /* free list of nodes */
  int             recycle_nodes;
  unsigned        anr_tries; /* ANR requests to the userfunc */
  unsigned        anr_count; /* # ANR ops performaed */
  int             anr_flag;  /* 0=off, !0=on */

  int (*anrfree)( void * key, void * data );
  int (*usrfree)( void * key, void * data );
} SFXHASH;


/*
*   HASH PROTOTYPES
*/
int             sfxhash_calcrows(int num);
SFXHASH       * sfxhash_new( int nrows, int keysize, int datasize, int memcap, 
                             int anr_flag, 
                             int (*anrfunc)(void *key, void * data),
                             int (*usrfunc)(void *key, void * data),
                             int recycle_flag );

void            sfxhash_set_max_nodes( SFXHASH *h, int max_nodes );

void            sfxhash_delete( SFXHASH * h );
int             sfxhash_make_empty(SFXHASH *);

int             sfxhash_add ( SFXHASH * h, void * key, void * data );
SFXHASH_NODE * sfxhash_get_node( SFXHASH * t, void * key );
int             sfxhash_remove( SFXHASH * h, void * key );
unsigned        sfxhash_count( SFXHASH * h );
unsigned        sfxhash_anr_count( SFXHASH * h );

void          * sfxhash_mru( SFXHASH * t );
void          * sfxhash_lru( SFXHASH * t );
SFXHASH_NODE  * sfxhash_mru_node( SFXHASH * t );
SFXHASH_NODE  * sfxhash_lru_node( SFXHASH * t );
void          * sfxhash_find( SFXHASH * h, void * key );
SFXHASH_NODE  * sfxhash_find_node( SFXHASH * t, void * key);

SFXHASH_NODE  * sfxhash_findfirst( SFXHASH * h );
SFXHASH_NODE  * sfxhash_findnext ( SFXHASH * h );

SFXHASH_NODE  * sfxhash_ghead( SFXHASH * h );
SFXHASH_NODE  * sfxhash_gnext( SFXHASH_NODE * n );
void sfxhash_gmovetofront( SFXHASH *t, SFXHASH_NODE * hnode );


void            sfxhash_splaymode( SFXHASH * h, int mode );

void          * sfxhash_alloc( SFXHASH * t, unsigned nbytes );
void            sfxhash_free( SFXHASH * t, void * p );
int             sfxhash_free_node(SFXHASH *t, SFXHASH_NODE *node);

unsigned        sfxhash_maxdepth( SFXHASH * t );
unsigned        sfxhash_overhead_bytes( SFXHASH * t );
unsigned        sfxhash_overhead_blocks( SFXHASH * t );
unsigned        sfxhash_find_success( SFXHASH * h );
unsigned        sfxhash_find_fail( SFXHASH * h );
unsigned        sfxhash_find_total( SFXHASH * h );


int sfxhash_set_keyops( SFXHASH *h ,
                        unsigned (*hash_fcn)( SFHASHFCN * p,
                                              unsigned char *d,
                                              int n),
                        int (*keycmp_fcn)( const void *s1,
                                           const void *s2,
                                           size_t n));


#endif

