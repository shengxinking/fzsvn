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
     sfhashfcn.c 

     Each hash table must allocate it's own SFGHASH struct, this is because
     sfghash_new uses the number of rows in the hash table to modulo the random
     values.

     Copyright (C) 2006-2008 Sourcefire, Inc.

     Updates:
     
     8/31/2006 - man - changed to use sfprimetable.c 
*/
 
#ifndef MODULUS_HASH
#include "cap.h" 
#endif
#include "sfhashfcn.h"

static void sf_free( void * p );

SFHASHFCN * sfhashfcn_new( int m )
{
  SFHASHFCN * p;
  static int one=1;
  
  if( one ) /* one time init */
  {
      srand( (unsigned) time(0) );
      one = 0;
  }

  // This can make all of the hashing static for testing.
  //#define rand() 0
   
  p = (SFHASHFCN*) calloc( 1,sizeof(SFHASHFCN) );
  if( !p )
      return 0;
 
#ifndef MODULUS_HASH
  if(pv.static_hash)
  {
    sfhashfcn_static(p);
  }
  else
#endif
  {
    p->seed     = sf_nearest_prime( (rand()%m)+3191 );
    p->scale    = sf_nearest_prime( (rand()%m)+709 );
    p->hardener = (rand()*rand()) + 133824503;
  }
   
  p->hash_fcn   = &sfhashfcn_hash;
  p->keycmp_fcn = &memcmp;
       
  return p;
}

void sfhashfcn_free( SFHASHFCN * p )
{
   if( p )
   {
       free( p);
   }
}

void sfhashfcn_static( SFHASHFCN * p )
{
    p->seed     = 3193;
    p->scale    = 719;
    p->hardener = 133824503;
}

unsigned sfhashfcn_hash( SFHASHFCN * p, unsigned char *d, int n )
{
    unsigned hash = p->seed;
    while( n )
    {
        hash *=  p->scale;
        hash += *d++;
        n--;
    }
    return hash ^ p->hardener;
}

/** 
 * Make sfhashfcn use a separate set of operators for the backend.
 *
 * @param h sfhashfcn ptr
 * @param hash_fcn user specified hash function
 * @param keycmp_fcn user specified key comparisoin function
 */
int sfhashfcn_set_keyops( SFHASHFCN *h,
                          unsigned (*hash_fcn)( SFHASHFCN * p,
                                                unsigned char *d,
                                                int n),
                          int (*keycmp_fcn)( const void *s1,
                                             const void *s2,
                                             size_t n))
{
    if(h && hash_fcn && keycmp_fcn)
    {
        h->hash_fcn   = hash_fcn;
        h->keycmp_fcn = keycmp_fcn;

        return 0;
    }

    return -1;
}

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

/*!
 *
 *  \f sfxhash.c
 *
 *  A Customized hash table library for storing and accessing key + data pairs.
 *
 *  This table incorporates a memory manager (memcap.c) to provide a memory cap,  
 *  and an automatic node recovery system for out of memory management. Keys and
 *  Data are copied into the hash table during the add operation. The data may
 *  be allocated and free'd by the user (by setting the datasize to zero ). A 
 *  user callback is provided to allow the user to do cleanup whenever a node
 *  is released, by either the ANR system or the relase() function.
 *
 *  Users can and should delete nodes when they know they are not needed anymore,
 *  but this custom table is designed for the case where nodes are allocated 
 *  permanently, we have to limit memory, and we wish to recycle old nodes.  
 *  Many problems have a natural node ageing paradigm working in our favor, 
 *  so automated node aging makes sense. i.e. thresholding, tcp state.
 *
 *  This hash table maps keys to data.  All keys must be unique.
 *  Uniqueness is enforcedby the code.
 *
 *  Features:
 *
 *    1) Keys must be fixed length (per table) binary byte sequences.
 *         keys are copied during the add function
 *    2) Data must be fixed length (per table) binary byte sequences.
 *         data is copied during the add function - if datasize > 0
 *       Data may be managed by the user as well.
 *    3) Table row sizes can be automatically adjusted to
 *       the nearest prime number size during table initialization/creation.
 *    4) Memory management includes tracking the size of each allocation, 
 *       number of allocations, enforcing a memory cap, and automatic node 
 *       recovery - when  memory is low the oldest untouched node
 *       is unlinked and recycled for use as a new node.
 *
 *  Per Node Memory Usage:
 *  ----------------------
 *     SFXHASH_NODE bytes
 *     KEYSIZE bytes
 *     [DATASIZE bytes] if datasize > 0 during call to sfxhash_new.
 *
 *  The hash node memory (sfxhash_node,key,and data) is allocated with 
 *  one call to s_alloc/memcap_alloc.
 *
 *  Author: Marc Norton
 *
 *  2003-06-03: cmg - added sfxhash_{l,m}ru to return {least,most}
 *              recently used node from the global list
 *
 *              - added _anrcount function
 *              - changed count function to return unsigned to match structure
 *
 *  2003-06-11: cmg added
 *              overhead_bytes + blocks to separate out the
 *              memcap constraints from the hash table itself
 *              find success v fail
 *
 *  2003-06-19: cmg added
 *
 *              ability to set own hash function
 *              ability to set own key cmp function
 *
 *  2003-06-30: rdempster
 *              fixed bug in that would anr from the freelist
 *              
 *  2005-11-15: modified sfxhash_add to check if 'data' is zero before memcpy'ing.
 *              this allows user to pass null for data, and set up the data area
 *              themselves after the call - this is much more flexible.
 *  8/31/2006: man - changed to use prime table lookup.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "sfprimetable.h"
#include "util.h"
#include "debug.h"

/*
 * Private Malloc - abstract the memory system
 */
static INLINE
void * s_alloc( SFXHASH * t, int n )
{
    return sfmemcap_alloc( &t->mc, n );
}
static INLINE
void s_free( SFXHASH * t, void * p )
{
    sfmemcap_free( &t->mc, p );
}

/*
 *   User access to the memory management, do they need it ? WaitAndSee
 */
void * sfxhash_alloc( SFXHASH * t, unsigned nbytes )
{
    return s_alloc( t, nbytes );
}
void   sfxhash_free( SFXHASH * t, void * p )
{
    s_free( t, p );
}

int sfxhash_nearest_powerof2(int nrows) 
{
    int i;

    nrows -= 1;
    for(i=1; i<sizeof(nrows) * 8; i <<= 1)          
        nrows = nrows | (nrows >> i);
    nrows += 1;

    return nrows;
}

int sfxhash_calcrows(int num)
{
    return sfxhash_nearest_powerof2(num);
//  return sf_nearest_prime( nrows );
}
        

/*!
 *
 * Create a new hash table
 *
 * By default, this will "splay" nodes to the top of a free list. 
 *
 * @param nrows    number of rows in hash table
 * @param keysize  key size in bytes, same for all keys
 * @param datasize datasize in bytes, zero indicates user manages data
 * @param maxmem   maximum memory to use in bytes
 * @param anr_flag Automatic Node Recovery boolean flag
 * @param anrfree  users Automatic Node Recovery memory release function
 * @param usrfree  users standard memory release function
 *
 * @return SFXHASH*
 * @retval  0 out of memory
 * @retval !0 Valid SFXHASH pointer  
 *
 */
/*
  Notes:
  if nrows < 0 don't cal the nearest prime.
  datasize must be the same for all entries, unless datasize is zero.
  maxmem of 0 indicates no memory limits.

*/
SFXHASH * sfxhash_new( int nrows, int keysize, int datasize, int maxmem, 
                       int anr_flag, 
                       int (*anrfree)(void * key, void * data),
                       int (*usrfree)(void * key, void * data),
                       int recycle_flag )
{
    int       i;
    SFXHASH * h;

    if( nrows > 0 ) /* make sure we have a prime number */
    {
//        nrows = sf_nearest_prime( nrows );
        /* If nrows is not a power of two, need to find the 
         * next highest power of two */
        nrows = sfxhash_nearest_powerof2(nrows);
    }
    else   /* use the magnitude of nrows as is */
    { 
        nrows = -nrows;
    }

    /* Allocate the table structure from general memory */
    //h = (SFXHASH*) calloc( 1, sizeof(SFXHASH) );
    h = (SFXHASH*)SnortAlloc(sizeof(SFXHASH));
    if( !h ) 
    {
        return 0;
    }

    /* this has a default hashing function */
    h->sfhashfcn = sfhashfcn_new( nrows );
    
    if( !h->sfhashfcn ) 
    {
        free(h);
        return 0;
    }

    sfmemcap_init( &h->mc, maxmem );

    /* Allocate the array of node ptrs */
    h->table = (SFXHASH_NODE**) s_alloc( h, sizeof(SFXHASH_NODE*) * nrows );
    if( !h->table ) 
    {
        free(h->sfhashfcn);
        free(h);
        return 0;
    }

    for( i=0; i<nrows; i++ )
    {
        h->table[i] = 0;
    }

    h->anrfree  = anrfree;
    h->usrfree  = usrfree;
    h->keysize  = keysize;
    h->datasize = datasize;
    h->nrows    = nrows;
    h->max_nodes = 0;
    h->crow     = 0; 
    h->cnode    = 0; 
    h->count    = 0;
    h->ghead    = 0;
    h->gtail    = 0;
    h->anr_count= 0;    
    h->anr_tries= 0;
    h->anr_flag = anr_flag; 
    h->splay    = 1; 
    h->recycle_nodes = recycle_flag;

    h->find_success = 0;
    h->find_fail    = 0;
    
    /* save off how much we've already allocated from our memcap */    
    h->overhead_bytes = h->mc.memused;
    h->overhead_blocks = h->mc.nblocks;

    return h;
}

/*!
 *  Set the maximum nodes used in this hash table.
 *  Specifying 0 is unlimited (or otherwise limited by memcap).
 * 
 * @param h SFXHASH table pointer
 * @param max_nodes maximum nodes to allow.
 *
 */
void sfxhash_set_max_nodes( SFXHASH *h, int max_nodes )
{
    if (h)
    {
        h->max_nodes = max_nodes;
    }
}

/*!
 *  Set Splay mode : Splays nodes to front of list on each access
 * 
 * @param t SFXHASH table pointer
 * @param n boolean flag toggles splaying of hash nodes
 *
 */
void sfxhash_splaymode( SFXHASH * t, int n )
{
    t->splay = n;
}


/*!
 *  Free all nodes in the free list
 *
 *  Removes and frees all of the nodes in the free list
 *  No need to call the user free, since that should've been
 *  done when those nodes were put back in the free list.
 *
 * @param h SFXHASH table pointer
 */
static
void sfxhash_delete_free_list(SFXHASH *t)
{
    SFXHASH_NODE *cur = NULL;
    SFXHASH_NODE *next = NULL;

    if (t == NULL || t->fhead == NULL)
        return;

    cur = t->fhead;

    while (cur != NULL)
    {
        next = cur->gnext;
        s_free(t, (void *)cur);
        cur = next;
    }

    t->fhead = NULL;
    t->ftail = NULL;
}

/*!
 *  Delete the hash Table 
 *
 *  free key's, free node's, and free the users data.
 *
 * @param h SFXHASH table pointer
 *
 */
void sfxhash_delete( SFXHASH * h )
{
    unsigned    i;
    SFXHASH_NODE * node, * onode;

    if( !h ) return;

    if( h->sfhashfcn ) sfhashfcn_free( h->sfhashfcn );
 
    if( h->table )
    {  
        for(i=0;i<h->nrows;i++)
        {
            for( node=h->table[i]; node;  )
            {
                onode = node;
                node  = node->next;
                
                /* Notify user that we are about to free this node function */
                if( h->usrfree )
                    h->usrfree( onode->key, onode->data );
        
                s_free( h,onode );
            }
        }
        s_free( h, h->table );
        h->table = 0;
    }

    sfxhash_delete_free_list( h );

    free( h ); /* free the table from general memory */
}

/*!
 *  Empty out the hash table
 *
 * @param h SFXHASH table pointer
 *
 * @return -1 on error
 */
int sfxhash_make_empty(SFXHASH *h)
{
    SFXHASH_NODE *n = NULL;
    SFXHASH_NODE *tmp = NULL;
    unsigned i;

    if (h == NULL)
        return -1;

    for (i = 0; i < h->nrows; i++)
    {
        for (n = h->table[i]; n != NULL; n = tmp)
        {
            tmp = n->next;
            if (sfxhash_free_node(h, n) != SFXHASH_OK)
            {
                return -1;
            }
        }
    }

    h->max_nodes = 0;
    h->crow = 0; 
    h->cnode = NULL; 
    h->count = 0;
    h->ghead = NULL;
    h->gtail = NULL;
    h->anr_count = 0;    
    h->anr_tries = 0;
    h->find_success = 0;
    h->find_fail = 0;
 
    return 0;
}


/*!
 *  Get the # of Nodes in HASH the table
 *
 * @param t SFXHASH table pointer
 *
 */
unsigned sfxhash_count( SFXHASH * t )
{
    return t->count;
}

/*!
 *  Get the # auto recovery 
 *
 * @param t SFXHASH table pointer
 *
 */
unsigned sfxhash_anr_count( SFXHASH * t )
{
    return t->anr_count;
}

/*!
 *  Get the # finds
 *
 * @param t SFXHASH table pointer
 *
 */
unsigned sfxhash_find_total( SFXHASH * t )
{
    return t->find_success + t->find_fail;
}

/*!
 *  Get the # unsucessful finds
 *
 * @param t SFXHASH table pointer
 *
 */
unsigned sfxhash_find_fail( SFXHASH * t )
{
    return t->find_fail;
}

/*!
 *  Get the # sucessful finds
 *
 * @param t SFXHASH table pointer
 *
 */
unsigned sfxhash_find_success( SFXHASH * t )
{
    return t->find_success;
}




/*!
 *  Get the # of overhead bytes
 *
 * @param t SFXHASH table pointer
 *
 */
unsigned sfxhash_overhead_bytes( SFXHASH * t )
{
    return t->overhead_bytes;
}

/*!
 *  Get the # of overhead blocks
 *
 * @param t SFXHASH table pointer
 *
 */
unsigned sfxhash_overhead_blocks( SFXHASH * t )
{
    return t->overhead_blocks;
}

/*
 *  Free List - uses the NODE gnext/gprev fields
 */
static
void sfxhash_save_free_node( SFXHASH *t, SFXHASH_NODE * hnode )
{
    /* Add A Node to the Free Node List */
    if( t->fhead ) /* add the node to head of the the existing list */
    {
        hnode->gprev    = 0;  
        hnode->gnext    = t->fhead;
        t->fhead->gprev = hnode;
        t->fhead        = hnode;
        /* tail is not affected */
    }
    else /* 1st node in this list */
    {
        hnode->gprev = 0;
        hnode->gnext = 0;
        t->fhead    = hnode;
        t->ftail    = hnode;
    }
}
static
SFXHASH_NODE * sfxhash_get_free_node( SFXHASH *t )
{
    SFXHASH_NODE * node = t->fhead;

    /* Remove A Node from the Free Node List - remove the head node */
    if( t->fhead  ) 
    {
        t->fhead = t->fhead->gnext;
        if( t->fhead ) 
            t->fhead->gprev = 0;

        if( t->ftail  == node ) /* no more nodes - clear the tail */
            t->ftail  =  0;
    }

    return node;
}

static
void sfxhash_glink_node( SFXHASH *t, SFXHASH_NODE * hnode )
{
    /* Add The Node */
    if( t->ghead ) /* add the node to head of the the existing list */
    {
        hnode->gprev    = 0;  
        hnode->gnext    = t->ghead;
        t->ghead->gprev = hnode;
        t->ghead        = hnode;
        /* tail is not affected */
    }
    else /* 1st node in this list */
    {
        hnode->gprev = 0;
        hnode->gnext = 0;
        t->ghead    = hnode;
        t->gtail    = hnode;
    }
}

static
void sfxhash_gunlink_node( SFXHASH *t, SFXHASH_NODE * hnode )
{
    /* Remove the Head Node */
    if( t->ghead == hnode ) /* add the node to head of the the existing list */
    {
        t->ghead = t->ghead->gnext;
        if( t->ghead ) 
            t->ghead->gprev = 0;
    }

    if( hnode->gprev ) hnode->gprev->gnext = hnode->gnext;
    if( hnode->gnext ) hnode->gnext->gprev = hnode->gprev;

    if( t->gtail  == hnode )
        t->gtail  =  hnode->gprev;             
}

void sfxhash_gmovetofront( SFXHASH *t, SFXHASH_NODE * hnode )
{
    if( hnode != t->ghead )
    {
        sfxhash_gunlink_node( t, hnode );
        sfxhash_glink_node( t, hnode );
    }
}

/*
 *
 */
static
void sfxhash_link_node( SFXHASH * t, SFXHASH_NODE * hnode )
{
    /* Add The Node to the Hash Table Row List */
    if( t->table[hnode->rindex] ) /* add the node to the existing list */
    {
        hnode->prev = 0;  // insert node as head node
        hnode->next=t->table[hnode->rindex];
        t->table[hnode->rindex]->prev = hnode;
        t->table[hnode->rindex] = hnode;
    }
    else /* 1st node in this list */
    {
        hnode->prev=0;
        hnode->next=0;
        t->table[hnode->rindex] = hnode;
    }
}

static
void sfxhash_unlink_node( SFXHASH * t, SFXHASH_NODE * hnode )
{
    if( hnode->prev )  // definitely not the 1st node in the list
    {
        hnode->prev->next = hnode->next;
        if( hnode->next ) 
            hnode->next->prev = hnode->prev;
    }
    else if( t->table[hnode->rindex] )  // must be the 1st node in the list
    {
        t->table[hnode->rindex] = t->table[hnode->rindex]->next;
        if( t->table[hnode->rindex] )
            t->table[hnode->rindex]->prev = 0;
    }
}

/*
 *  move a node to the front of the row list at row = 'index'
 */
static void s_movetofront( SFXHASH *t, SFXHASH_NODE * n )
{
    /* Modify Hash Node Row List */
    if( t->table[n->rindex] != n ) // if not at front of list already...
    {
        /* Unlink the node */
        sfxhash_unlink_node( t, n );
     
        /* Link at front of list */
        sfxhash_link_node( t, n );
    }

    /* Move node in the global hash node list to the front */
    sfxhash_gmovetofront( t, n );
}

/*
 * Allocat a new hash node, uses Auto Node Recovery if needed and enabled.
 * 
 * The oldest node is the one with the longest time since it was last touched, 
 * and does not have any direct indication of how long the node has been around.
 * We don't monitor the actual time since last being touched, instead we use a
 * splayed global list of node pointers. As nodes are accessed they are splayed
 * to the front of the list. The oldest node is just the tail node.
 *
 */
static 
SFXHASH_NODE * sfxhash_newnode( SFXHASH * t )
{
    SFXHASH_NODE * hnode;

    /* Recycle Old Nodes - if any */
    hnode = sfxhash_get_free_node( t );

    /* Allocate memory for a node */
    if( ! hnode )
    {
        if ((t->max_nodes == 0) || (t->count < t->max_nodes))
        {
            hnode = (SFXHASH_NODE*)s_alloc( t, sizeof(SFXHASH_NODE) +
                                         t->keysize + t->datasize );
        }
    }
        
    /*  If we still haven't found hnode, we're at our memory limit.
     *
     *  Uses Automatic Node Recovery, to recycle the oldest node-based on access
     *  (Unlink and reuse the tail node)
     */ 
    if( !hnode && t->anr_flag && t->gtail )
    {
        /* Find the oldes node the users willing to let go. */
        for(hnode = t->gtail; hnode; hnode = hnode->gprev )
        {
            if( t->anrfree ) /* User has provided a permission+release callback function */
            {
                t->anr_tries++;/* Count # ANR requests */
                
                /* Ask the user for permission to release this node, but let them say no! */
                if( t->anrfree( hnode->key, hnode->data ) )
                {
                    /* NO, don't recycle this node, user's not ready to let it go. */                            
                    continue;
                }
                
                /* YES, user said we can recycle this node */
            }

            sfxhash_gunlink_node( t, hnode ); /* unlink from the global list */
            sfxhash_unlink_node( t, hnode ); /* unlink from the row list */
            t->count--;
            t->anr_count++; /* count # of ANR operations */
            break;
        }
    }

    /* either we are returning a node or we're all full and the user
     * won't let us allocate anymore and we return NULL */
    return hnode;
}

/*
 *
 *  Find a Node based on the key, return the node and the index.
 *  The index is valid even if the return value is NULL, in which
 *  case the index is the corect row in which the node should be 
 *  created.
 *
 */

#define hashsize(n) ((u_int32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)

static 
SFXHASH_NODE * sfxhash_find_node_row( SFXHASH * t, void * key, int * rindex )
{
    unsigned       hashkey;
    int            index;
    SFXHASH_NODE  *hnode;

    hashkey = t->sfhashfcn->hash_fcn( t->sfhashfcn,
                                      (unsigned char*)key,
                                      t->keysize );
    
/*     printf("hashkey: %u t->keysize: %d\n", hashkey, t->keysize); */
/*     flowkey_fprint(stdout, key);  */
/*     printf("****\n"); */

//     index   = hashkey % t->nrows;
    /* Modulus is slow. Switched to a table size that is a power of 2. */      
    index  = hashkey & (t->nrows - 1);

    *rindex = index;
   
    for( hnode=t->table[index]; hnode; hnode=hnode->next )
    {
        if( !t->sfhashfcn->keycmp_fcn(hnode->key,key,t->keysize) )
        {
            if( t->splay > 0 )
                s_movetofront(t,hnode);

            t->find_success++;            
            return hnode;
        }
    }

    t->find_fail++;            
    return NULL;
}



/*!
 * Add a key + data pair to the hash table
 *
 * 2003-06-06:
 *  - unique_tracker.c assumes that this splays
 *    nodes to the top when they are added.
 *
 *    This is done because of the successful find.
 *
 * @param t SFXHASH table pointer
 * @param key  users key pointer
 * @param data  users data pointer
 *
 * @return integer
 * @retval SFXHASH_OK      success
 * @retval SFXHASH_INTABLE already in the table, t->cnode points to the node
 * @retval SFXHASH_NOMEM   not enough memory
 */
int sfxhash_add( SFXHASH * t, void * key, void * data )
{
    int            index;
    SFXHASH_NODE * hnode;

    /* Enforce uniqueness: Check for the key in the table */
    hnode = sfxhash_find_node_row( t, key, &index );

    if( hnode )
    {
        t->cnode = hnode;

        return SFXHASH_INTABLE; /* found it - return it. */
    }

    /* 
     *  Alloc new hash node - allocate key space and data space at the same time.
     */
    hnode = sfxhash_newnode( t );
    if( !hnode )
    {
        return SFXHASH_NOMEM;
    }

    /* Set up the new key pointer */
    hnode->key = (char*)hnode + sizeof(SFXHASH_NODE);

    /* Copy the key */
    memcpy(hnode->key,key,t->keysize);

    /* Save our table row index */
    hnode->rindex = index;

    /* Copy the users data - or if datasize is zero set ptr to users data */
    if( t->datasize )
    {
        /* Set up the new data pointer */
        hnode->data= (char*)hnode + sizeof(SFXHASH_NODE) + t->keysize;

        if(data)
        {
           memcpy(hnode->data,data,t->datasize);
        }
    }
    else 
    {
        hnode->data = data;
    }
    
    /* Link the node into the table row list */
    sfxhash_link_node ( t, hnode );

    /* Link at the front of the global node list */
    sfxhash_glink_node( t, hnode );

    /* Track # active nodes */
    t->count++;

    return SFXHASH_OK;
}


/*!
 * Add a key to the hash table, return the hash node
 *
 * 2003-06-06:
 *  - unique_tracker.c assumes that this splays
 *    nodes to the top when they are added.
 *
 *    This is done because of the successful find.
 *
 * @param t SFXHASH table pointer
 * @param key  users key pointer
 *
 * @return integer
 * @retval SFXHASH_OK      success
 * @retval SFXHASH_INTABLE already in the table, t->cnode points to the node
 * @retval SFXHASH_NOMEM   not enough memory
 */
SFXHASH_NODE * sfxhash_get_node( SFXHASH * t, void * key )
{
    int            index;
    SFXHASH_NODE * hnode;

    /* Enforce uniqueness: Check for the key in the table */
    hnode = sfxhash_find_node_row( t, key, &index );

    if( hnode )
    {
        t->cnode = hnode;

        return hnode; /* found it - return it. */
    }

    /* 
     *  Alloc new hash node - allocate key space and data space at the same time.
     */
    hnode = sfxhash_newnode( t );
    if( !hnode )
    {
        return NULL;
    }

    /* Set up the new key pointer */
    hnode->key = (char*)hnode + sizeof(SFXHASH_NODE);

    /* Copy the key */
    memcpy(hnode->key,key,t->keysize);

    /* Save our table row index */
    hnode->rindex = index;

    /* Copy the users data - or if datasize is zero set ptr to users data */
    if( t->datasize )
    {
        /* Set up the new data pointer */
        hnode->data= (char*)hnode + sizeof(SFXHASH_NODE) + t->keysize;
    }
    else 
    {
        hnode->data = NULL;
    }
    
    /* Link the node into the table row list */
    sfxhash_link_node ( t, hnode );

    /* Link at the front of the global node list */
    sfxhash_glink_node( t, hnode );

    /* Track # active nodes */
    t->count++;

    return hnode;
}


/*!
 * Find a Node based on the key
 *
 * @param t SFXHASH table pointer
 * @param key  users key pointer
 *
 * @return SFXHASH_NODE*   valid pointer to the hash node
 * @retval 0               node not found
 *
 */
SFXHASH_NODE * sfxhash_find_node( SFXHASH * t, void * key)
{
    int            rindex;

    return sfxhash_find_node_row( t, key, &rindex );
}

/*!
 * Find the users data based associated with the key
 *
 * @param t SFXHASH table pointer
 * @param key  users key pointer
 *
 * @return void*   valid pointer to the users data
 * @retval 0       node not found
 *
 */
void * sfxhash_find( SFXHASH * t, void * key)
{
    SFXHASH_NODE * hnode;
    int            rindex;

    hnode = sfxhash_find_node_row( t, key, &rindex );

    if( hnode ) return hnode->data;

    return NULL;
}


/** 
 * Get the HEAD of the in use list
 * 
 * @param t table pointer 
 * 
 * @return the head of the list or NULL
 */
SFXHASH_NODE *sfxhash_ghead( SFXHASH * t )
{
    if(t)
    {
        return t->ghead;
    }

    return NULL;
}


/** 
 * Walk the global list
 * 
 * @param n current node
 * 
 * @return the next node in the list or NULL when at the end
 */
SFXHASH_NODE *sfxhash_gnext( SFXHASH_NODE *n )
{
    if(n)
    {
        return n->gnext;
    }

    return NULL;
}


/*!
 * Return the most recently used data from the global list
 *
 * @param t SFXHASH table pointer
 *
 * @return void*   valid pointer to the users data
 * @retval 0       node not found
 *
 */
void * sfxhash_mru( SFXHASH * t )
{
    SFXHASH_NODE * hnode;

    hnode = sfxhash_ghead(t);

    if( hnode )
        return hnode->data;
        
    return NULL;
}

/*!
 * Return the least recently used data from the global list
 *
 * @param t SFXHASH table pointer
 *
 * @return void*   valid pointer to the users data
 * @retval 0       node not found
 *
 */
void * sfxhash_lru( SFXHASH * t )
{
    SFXHASH_NODE * hnode;

    hnode = t->gtail;

    if( hnode ) return hnode->data;

    return NULL;
}

/*!
 * Return the most recently used node from the global list
 *
 * @param t SFXHASH table pointer
 *
 * @return SFXHASH_NODE*   valid pointer to a node
 * @retval 0       node not found
 *
 */
SFXHASH_NODE * sfxhash_mru_node( SFXHASH * t )
{
    SFXHASH_NODE * hnode;

    hnode = sfxhash_ghead(t);

    if( hnode )
        return hnode;
        
    return NULL;
}

/*!
 * Return the least recently used node from the global list
 *
 * @param t SFXHASH table pointer
 *
 * @return SFXHASH_NODE*   valid pointer to a node
 * @retval 0       node not found
 *
 */
SFXHASH_NODE * sfxhash_lru_node( SFXHASH * t )
{
    SFXHASH_NODE * hnode;

    hnode = t->gtail;

    if( hnode ) return hnode;

    return NULL;
}

/*!
 * Get some hash table statistics. NOT FOR REAL TIME USE.
 *
 * 
 * @param t SFXHASH table pointer
 * @param filled how many 
 *
 * @return max depth of the table
 *
 */
unsigned sfxhash_maxdepth( SFXHASH * t )
{
    unsigned i;
    unsigned max_depth = 0;

    SFXHASH_NODE * hnode;

    for( i=0; i<t->nrows; i++ )
    {
        unsigned cur_depth = 0;

        for(hnode = t->table[i]; hnode != NULL; hnode = hnode->next)
        {
            cur_depth++;
        }

        if(cur_depth > max_depth)
            max_depth = cur_depth;
    }

    return max_depth;
}

/*
 *  Unlink and free the node
 */
int sfxhash_free_node( SFXHASH * t, SFXHASH_NODE * hnode)
{
    sfxhash_unlink_node( t, hnode ); /* unlink from the hash table row list */

    sfxhash_gunlink_node( t, hnode ); /* unlink from global-hash-node list */

    t->count--;

    if( t->usrfree )
    {
        t->usrfree( hnode->key, hnode->data );
    }

    if( t->recycle_nodes )
    {
        sfxhash_save_free_node( t, hnode );
    }
    else
    {
        s_free( t, hnode );
    }

    return SFXHASH_OK;
}

/*!
 * Remove a Key + Data Pair from the table.
 *
 * @param t SFXHASH table pointer
 * @param key  users key pointer
 *
 * @return 0   success
 * @retval !0  failed
 *
 */
int sfxhash_remove( SFXHASH * t, void * key)
{
    SFXHASH_NODE * hnode;
    unsigned hashkey, index;

    hashkey = t->sfhashfcn->hash_fcn( t->sfhashfcn,
                                      (unsigned char*)key,
                                      t->keysize );
    
//    index = hashkey % t->nrows;
    /* Modulus is slow */
    index   = hashkey & (t->nrows - 1);

    hnode = t->table[index];
    
    for( hnode=t->table[index]; hnode; hnode=hnode->next )
    {
        if( !t->sfhashfcn->keycmp_fcn(hnode->key,key,t->keysize) )
        {
            return sfxhash_free_node( t, hnode );
        }
    }

    return SFXHASH_ERR;  
}

/* 
   Internal use only 
*/
static 
void sfxhash_next( SFXHASH * t )
{
    if( !t->cnode )
        return ;
 
    /* Next node in current node list */
    t->cnode = t->cnode->next;
    if( t->cnode )
    {
        return;
    }

    /* Next row */ 
    /* Get 1st node in next non-emtoy row/node list */
    for( t->crow++; t->crow < t->nrows; t->crow++ )
    {    
        t->cnode = t->table[ t->crow ];
        if( t->cnode ) 
        {
            return;
        }
    }
}
/*!
 * Find and return the first hash table node
 *
 * @param t SFXHASH table pointer
 *
 * @return 0   failed
 * @retval !0  valid SFXHASH_NODE *
 *
 */
SFXHASH_NODE * sfxhash_findfirst( SFXHASH * t )
{
    SFXHASH_NODE * n;

    if(!t)
        return NULL;

    /* Start with 1st row */
    for( t->crow=0; t->crow < t->nrows; t->crow++ )
    {    
        /* Get 1st Non-Null node in row list */
        t->cnode = t->table[ t->crow ];
        if( t->cnode )
        {
            n = t->cnode;
            sfxhash_next( t ); // load t->cnode with the next entry
            return n;
        }
    }
    
    return NULL;
}

/*!
 * Find and return the next hash table node
 *
 * @param t SFXHASH table pointer
 *
 * @return 0   failed
 * @retval !0  valid SFXHASH_NODE *
 *
 */
SFXHASH_NODE * sfxhash_findnext( SFXHASH * t )
{
    SFXHASH_NODE * n;

    n = t->cnode;
    if( !n ) /* Done, no more entries */
    {
        return NULL;
    }

    /*
      Preload next node into current node 
    */
    sfxhash_next( t ); 

    return  n;
}


/** 
 * Make sfhashfcn use a separate set of operators for the backend.
 *
 * @param h sfhashfcn ptr
 * @param hash_fcn user specified hash function
 * @param keycmp_fcn user specified key comparisoin function
 */

int sfxhash_set_keyops( SFXHASH *h ,
                        unsigned (*hash_fcn)( SFHASHFCN * p,
                                              unsigned char *d,
                                              int n),
                        int (*keycmp_fcn)( const void *s1,
                                           const void *s2,
                                           size_t n))
{
    if(h && hash_fcn && keycmp_fcn)
    {
        return sfhashfcn_set_keyops(h->sfhashfcn, hash_fcn, keycmp_fcn);
    }

    return -1;
}


/*
 * -----------------------------------------------------------------------------------------
 *   Test Driver for Hashing
 * -----------------------------------------------------------------------------------------
 */
#ifdef SFXHASH_MAIN 


/* 
   This is called when the user releases a node or kills the table 
*/
int usrfree( void * key, void * data )
{

    /* Release any data you need to */
    return 0;  
}

/* 
   Auto Node Recovery Callback - optional 

   This is called to ask the user to kill a node, if it reutrns !0 than the hash
   library does not kill this node.  If the user os willing to let the node die,
   the user must do any free'ing or clean up on the node during this call.
*/
int anrfree( void * key, void * data )
{
    static int bx = 0;

    /* Decide if we can free this node. */

    //bx++; if(bx == 4 )bx=0;       /* for testing */

    /* if we are allowing the node to die, kill it */
    if( !bx ) usrfree( key, data );

    return bx;  /* Allow the caller to  kill this nodes data + key */
}

/*
 *       Hash test program : use 'sfxhash 1000 50000' to stress the Auto_NodeRecover feature
 */
int main ( int argc, char ** argv )
{
    int             i;
    SFXHASH      * t;
    SFXHASH_NODE * n;
    char            strkey[256], strdata[256], * p;
    int             num = 100;
    int             mem = 0;

    memset(strkey,0,20);
    memset(strdata,0,20);

    if( argc > 1 )
    {
        num = atoi(argv[1]);
    }

    if( argc > 2 )
    {
        mem = atoi(argv[2]);
    }

    /* Create a Hash Table */
    t = sfxhash_new( 100,        /* one row per element in table, when possible */
                     20,        /* key size :  padded with zeros */ 
                     20,        /* data size:  padded with zeros */ 
                     mem,       /* max bytes,  0=no max */  
                     1,         /* enable AutoNodeRecovery */
                     anrfree,   /* provide a function to let user know we want to kill a node */
                     usrfree, /* provide a function to release user memory */
                     1);      /* Recycle nodes */
    if(!t)
    {
        printf("Low Memory!\n");
        exit(0);
    }
    /* Add Nodes to the Hash Table */
    for(i=0;i<num;i++) 
    {
        snprintf(strkey, sizeof(strkey), "KeyWord%5.5d",i+1);
        strkey[sizeof(strkey) - 1] = '\0';
        snprintf(strdata, sizeof(strdata), "KeyWord%5.5d",i+1);
        strdata[sizeof(strdata) - 1] = '\0';
        //strupr(strdata);
        sfxhash_add( t, strkey  /* user key */ ,  strdata /* user data */ );
    }  

    /* Find and Display Nodes in the Hash Table */
    printf("\n** FIND KEY TEST\n");
    for(i=0;i<num;i++) 
    {
        snprintf(strkey, sizeof(strkey) - 1, "KeyWord%5.5d",i+1);
        strkey[sizeof(strkey) - 1] = '\0';

        p = (char*) sfxhash_find( t, strkey );

        if(p)printf("Hash-key=%*s, data=%*s\n", strlen(strkey),strkey, strlen(strkey), p );
    }  

    /* Show memcap memory */
    printf("\n...******\n");
    sfmemcap_showmem(&t->mc);
    printf("...******\n");

    /* Display All Nodes in the Hash Table findfirst/findnext */
    printf("\n...FINDFIRST / FINDNEXT TEST\n");
    for( n  = sfxhash_findfirst(t); 
         n != 0; 
         n  = sfxhash_findnext(t) )
    {
        printf("hash-findfirst/next: n=%x, key=%s, data=%s\n", n, n->key, n->data );

        /*
          remove node we are looking at, this is first/next safe.
        */
        if( sfxhash_remove(t,n->key) ) 
        {
            printf("...ERROR: Could not remove the key node!\n");
        }
        else  
        {
            printf("...key node removed\n");
        }
    }

    printf("...Auto-Node-Recovery: %d recycle attempts, %d completions.\n",t->anr_tries,t->anr_count);

    /* Free the table and it's user data */
    printf("...sfxhash_delete\n");

    sfxhash_delete( t );
   
    printf("\nnormal pgm finish\n\n");

    return 0;
}
#endif


/* $Id$ */
/*
** Copyright (C) 2002-2008 Sourcefire, Inc.
** Copyright (C) 2002 Martin Roesch <roesch@sourcefire.com>
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

/* Function: int sf_sdlist_init(sfSDlist *list, void (*destroy)(void *data))
 * 
 * Purpose: initialize an dlist
 * Args: list - pointer to a dlist structure
 *       destroy - free function ( use NULL for none )
 * Returns:
 *     1 on failure , 0 on success
 */ 

int sf_sdlist_init(sfSDList *list, void (*destroy)(void *data))
{
    list->destroy = destroy;
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    return 0;
}


/* Function: int sf_sdlist_delete(sfSDList *list) 
 * 
 * Purpose: delete every item of a list
 * Args: list -> pointer to a dlist structure
 * 
 * Returns: 1 on failure , 0 on success
 */ 
int sf_sdlist_delete(sfSDList *list)
{
    while(list->head != NULL)
    {
        sf_sdlist_remove_next(list, NULL);
    }
    
    return 0;
}

/*
 * Function: int sf_sdlist_insert_next(sfSDList *list, SDListItem *item,
 *                                    void *data, SDListItem *container) 
 *
 * Purpose: insert data in container in the list after the item
 * Args: list - dlist structure
 *       item - current position in list structure
 *       data - current data to insert
 *       container - place to put the data 
 *
 * Returns: 0 on sucess,  1 on failure
 */ 
int sf_sdlist_insert_next(sfSDList *list, SDListItem *item, void *data,
                          SDListItem *container) 
{
    SDListItem *new = container;
    
    if(!new) return -1;

    new->data = data;

    if(item == NULL)
    {
        /* We are inserting at the head of the list HEAD */

        if(list->size == 0)
        {
            list->tail = new;
        }
        
        new->next = list->head;
        list->head = new;
    }
    else
    {
        if(item->next == NULL)
        {
            /* TAIL */
            list->tail = new;
        }

        new->next = item->next;
        item->next = new;        
    }

    new->prev = item;
    list->size++;
    return 0;
}

int sf_sdlist_append(sfSDList *list, void *data, SDListItem *container) {
    return sf_sdlist_insert_next(list, list->tail, data, container);
}

int sf_sdlist_remove_next(sfSDList *list, SDListItem *item) {
    SDListItem *li = NULL;
    void *data;

    if(list->size == 0)
    {
        return -1;
    }

    /* remove the head */
    if(item == NULL)
    {
        li = list->head;
        data = li->data;
        list->head = li->next;
    }
    else
    {
        data = item->data;        
        if(item->next == NULL)
        {
            return -1;
        }

        li = item->next;
        item->next = li->next;
        item->prev = li->prev;
    }

    if(li->next != NULL)
    {
        li->next->prev = item;
    }

    
    if(list->destroy != NULL)
        list->destroy(data);
    
    list->size--;
    
    if(list->size == 0) {
        list->tail = NULL;
    }

    return 0;
}


/*
 * Function: int sf_sdlist_remove(sfSDList *list, SDListItem *item)
 *
 * Purpose: remove the item pointed to by item
 * Args: list - list pointer
 *       item - item to unlink from the list
 *
 * Returns: 0 on success , 1 on exception
 *  
 */ 
int sf_sdlist_remove(sfSDList *list, SDListItem *item)
{
    SDListItem *next_item;
    SDListItem *prev_item;

    if(item == NULL)
    {
        return -1;
    }

    next_item = item->next;
    prev_item = item->prev;

    if(next_item != NULL)
    {
        next_item->prev = prev_item;
    } else {
        list->tail = prev_item;
    }

    if(prev_item != NULL)
    {
        prev_item->next = next_item;       
    } else {
        /* HEAD */
        list->head = next_item;
    }
        
        
        
    
    if(list->destroy != NULL)
        list->destroy(item->data);


    list->size--;
    
    if(list->size == 0)
    {
        list->head = NULL;
        list->tail = NULL;
    }

    return 0;
}

void print_sdlist(sfSDList *a)
{
    SDListItem *li;
    printf("***");
    printf(" size: %d\n", a->size);
    for(li = a->head; li != NULL; li = li->next) {
        printf(" `- %p\n", (void*)li);
    }
}

#ifdef TEST_SDLIST
void bad(void *d) {
    free(d);
    return;
}

int main(void) {
    sfSDList a;

    SDListItem *li;
    SDListItem listpool[1000];
    
    sf_sdlist_init(&a, &bad);
    if(sf_sdlist_append(&a, (char *) SnortStrdup("hello"), &listpool[0]))
    {
        printf("error appending!\n");
    }
    
    sf_sdlist_append(&a, (char *)SnortStrdup("goodbye"), &listpool[1]);

    sf_sdlist_insert_next(&a, NULL, (char *)SnortStrdup("woo"), &listpool[2]);

    printf("list size %d\n", a.size);
    
    for(li = a.head; li != NULL; li = li->next)
    {
        printf("%s\n", (char *) li->data);
    }


    printf("*** removing ***\n");
    sf_sdlist_remove(&a, &listpool[1]);
    printf("list size %d\n", a.size);
    for(li = a.head; li != NULL; li = li->next)
    {
        printf("%s\n", (char *) li->data);
    }

    sf_sdlist_delete(&a);

    printf("list size %d\n", a.size);
    return 0;
}
#endif /* TEST_SDLIST */
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

/*
* sfprimetable.c
*
* Prime number calculation via Table lookups.
*
* This was implemented for use with the hasing functions
* in sfghash, and sfxhash. 
*
*/
/* 0-8K, increments=8 */
static 
unsigned prime_table0[1024]={
 3, /* 1 */ 
 7, /* 9 */ 
 17, /* 17 */ 
 23, /* 25 */ 
 31, /* 33 */ 
 41, /* 41 */ 
 47, /* 49 */ 
 53, /* 57 */ 
 61, /* 65 */ 
 73, /* 73 */ 
 79, /* 81 */ 
 89, /* 89 */ 
 97, /* 97 */ 
 103, /* 105 */ 
 113, /* 113 */ 
 113, /* 121 */ 
 127, /* 129 */ 
 137, /* 137 */ 
 139, /* 145 */ 
 151, /* 153 */ 
 157, /* 161 */ 
 167, /* 169 */ 
 173, /* 177 */ 
 181, /* 185 */ 
 193, /* 193 */ 
 199, /* 201 */ 
 199, /* 209 */ 
 211, /* 217 */ 
 223, /* 225 */ 
 233, /* 233 */ 
 241, /* 241 */ 
 241, /* 249 */ 
 257, /* 257 */ 
 263, /* 265 */ 
 271, /* 273 */ 
 281, /* 281 */ 
 283, /* 289 */ 
 293, /* 297 */ 
 293, /* 305 */ 
 313, /* 313 */ 
 317, /* 321 */ 
 317, /* 329 */ 
 337, /* 337 */ 
 337, /* 345 */ 
 353, /* 353 */ 
 359, /* 361 */ 
 367, /* 369 */ 
 373, /* 377 */ 
 383, /* 385 */ 
 389, /* 393 */ 
 401, /* 401 */ 
 409, /* 409 */ 
 409, /* 417 */ 
 421, /* 425 */ 
 433, /* 433 */ 
 439, /* 441 */ 
 449, /* 449 */ 
 457, /* 457 */ 
 463, /* 465 */ 
 467, /* 473 */ 
 479, /* 481 */ 
 487, /* 489 */ 
 491, /* 497 */ 
 503, /* 505 */ 
 509, /* 513 */ 
 521, /* 521 */ 
 523, /* 529 */ 
 523, /* 537 */ 
 541, /* 545 */ 
 547, /* 553 */ 
 557, /* 561 */ 
 569, /* 569 */ 
 577, /* 577 */ 
 577, /* 585 */ 
 593, /* 593 */ 
 601, /* 601 */ 
 607, /* 609 */ 
 617, /* 617 */ 
 619, /* 625 */ 
 631, /* 633 */ 
 641, /* 641 */ 
 647, /* 649 */ 
 653, /* 657 */ 
 661, /* 665 */ 
 673, /* 673 */ 
 677, /* 681 */ 
 683, /* 689 */ 
 691, /* 697 */ 
 701, /* 705 */ 
 709, /* 713 */ 
 719, /* 721 */ 
 727, /* 729 */ 
 733, /* 737 */ 
 743, /* 745 */ 
 751, /* 753 */ 
 761, /* 761 */ 
 769, /* 769 */ 
 773, /* 777 */ 
 773, /* 785 */ 
 787, /* 793 */ 
 797, /* 801 */ 
 809, /* 809 */ 
 811, /* 817 */ 
 823, /* 825 */ 
 829, /* 833 */ 
 839, /* 841 */ 
 839, /* 849 */ 
 857, /* 857 */ 
 863, /* 865 */ 
 863, /* 873 */ 
 881, /* 881 */ 
 887, /* 889 */ 
 887, /* 897 */ 
 887, /* 905 */ 
 911, /* 913 */ 
 919, /* 921 */ 
 929, /* 929 */ 
 937, /* 937 */ 
 941, /* 945 */ 
 953, /* 953 */ 
 953, /* 961 */ 
 967, /* 969 */ 
 977, /* 977 */ 
 983, /* 985 */ 
 991, /* 993 */ 
 997, /* 1001 */ 
 1009, /* 1009 */ 
 1013, /* 1017 */ 
 1021, /* 1025 */ 
 1033, /* 1033 */ 
 1039, /* 1041 */ 
 1049, /* 1049 */ 
 1051, /* 1057 */ 
 1063, /* 1065 */ 
 1069, /* 1073 */ 
 1069, /* 1081 */ 
 1087, /* 1089 */ 
 1097, /* 1097 */ 
 1103, /* 1105 */ 
 1109, /* 1113 */ 
 1117, /* 1121 */ 
 1129, /* 1129 */ 
 1129, /* 1137 */ 
 1129, /* 1145 */ 
 1153, /* 1153 */ 
 1153, /* 1161 */ 
 1163, /* 1169 */ 
 1171, /* 1177 */ 
 1181, /* 1185 */ 
 1193, /* 1193 */ 
 1201, /* 1201 */ 
 1201, /* 1209 */ 
 1217, /* 1217 */ 
 1223, /* 1225 */ 
 1231, /* 1233 */ 
 1237, /* 1241 */ 
 1249, /* 1249 */ 
 1249, /* 1257 */ 
 1259, /* 1265 */ 
 1259, /* 1273 */ 
 1279, /* 1281 */ 
 1289, /* 1289 */ 
 1297, /* 1297 */ 
 1303, /* 1305 */ 
 1307, /* 1313 */ 
 1321, /* 1321 */ 
 1327, /* 1329 */ 
 1327, /* 1337 */ 
 1327, /* 1345 */ 
 1327, /* 1353 */ 
 1361, /* 1361 */ 
 1367, /* 1369 */ 
 1373, /* 1377 */ 
 1381, /* 1385 */ 
 1381, /* 1393 */ 
 1399, /* 1401 */ 
 1409, /* 1409 */ 
 1409, /* 1417 */ 
 1423, /* 1425 */ 
 1433, /* 1433 */ 
 1439, /* 1441 */ 
 1447, /* 1449 */ 
 1453, /* 1457 */ 
 1459, /* 1465 */ 
 1471, /* 1473 */ 
 1481, /* 1481 */ 
 1489, /* 1489 */ 
 1493, /* 1497 */ 
 1499, /* 1505 */ 
 1511, /* 1513 */ 
 1511, /* 1521 */ 
 1523, /* 1529 */ 
 1531, /* 1537 */ 
 1543, /* 1545 */ 
 1553, /* 1553 */ 
 1559, /* 1561 */ 
 1567, /* 1569 */ 
 1571, /* 1577 */ 
 1583, /* 1585 */ 
 1583, /* 1593 */ 
 1601, /* 1601 */ 
 1609, /* 1609 */ 
 1613, /* 1617 */ 
 1621, /* 1625 */ 
 1627, /* 1633 */ 
 1637, /* 1641 */ 
 1637, /* 1649 */ 
 1657, /* 1657 */ 
 1663, /* 1665 */ 
 1669, /* 1673 */ 
 1669, /* 1681 */ 
 1669, /* 1689 */ 
 1697, /* 1697 */ 
 1699, /* 1705 */ 
 1709, /* 1713 */ 
 1721, /* 1721 */ 
 1723, /* 1729 */ 
 1733, /* 1737 */ 
 1741, /* 1745 */ 
 1753, /* 1753 */ 
 1759, /* 1761 */ 
 1759, /* 1769 */ 
 1777, /* 1777 */ 
 1783, /* 1785 */ 
 1789, /* 1793 */ 
 1801, /* 1801 */ 
 1801, /* 1809 */ 
 1811, /* 1817 */ 
 1823, /* 1825 */ 
 1831, /* 1833 */ 
 1831, /* 1841 */ 
 1847, /* 1849 */ 
 1847, /* 1857 */ 
 1861, /* 1865 */ 
 1873, /* 1873 */ 
 1879, /* 1881 */ 
 1889, /* 1889 */ 
 1889, /* 1897 */ 
 1901, /* 1905 */ 
 1913, /* 1913 */ 
 1913, /* 1921 */ 
 1913, /* 1929 */ 
 1933, /* 1937 */ 
 1933, /* 1945 */ 
 1951, /* 1953 */ 
 1951, /* 1961 */ 
 1951, /* 1969 */ 
 1973, /* 1977 */ 
 1979, /* 1985 */ 
 1993, /* 1993 */ 
 1999, /* 2001 */ 
 2003, /* 2009 */ 
 2017, /* 2017 */ 
 2017, /* 2025 */ 
 2029, /* 2033 */ 
 2039, /* 2041 */ 
 2039, /* 2049 */ 
 2053, /* 2057 */ 
 2063, /* 2065 */ 
 2069, /* 2073 */ 
 2081, /* 2081 */ 
 2089, /* 2089 */ 
 2089, /* 2097 */ 
 2099, /* 2105 */ 
 2113, /* 2113 */ 
 2113, /* 2121 */ 
 2129, /* 2129 */ 
 2137, /* 2137 */ 
 2143, /* 2145 */ 
 2153, /* 2153 */ 
 2161, /* 2161 */ 
 2161, /* 2169 */ 
 2161, /* 2177 */ 
 2179, /* 2185 */ 
 2179, /* 2193 */ 
 2179, /* 2201 */ 
 2207, /* 2209 */ 
 2213, /* 2217 */ 
 2221, /* 2225 */ 
 2221, /* 2233 */ 
 2239, /* 2241 */ 
 2243, /* 2249 */ 
 2251, /* 2257 */ 
 2251, /* 2265 */ 
 2273, /* 2273 */ 
 2281, /* 2281 */ 
 2287, /* 2289 */ 
 2297, /* 2297 */ 
 2297, /* 2305 */ 
 2311, /* 2313 */ 
 2311, /* 2321 */ 
 2311, /* 2329 */ 
 2333, /* 2337 */ 
 2341, /* 2345 */ 
 2351, /* 2353 */ 
 2357, /* 2361 */ 
 2357, /* 2369 */ 
 2377, /* 2377 */ 
 2383, /* 2385 */ 
 2393, /* 2393 */ 
 2399, /* 2401 */ 
 2399, /* 2409 */ 
 2417, /* 2417 */ 
 2423, /* 2425 */ 
 2423, /* 2433 */ 
 2441, /* 2441 */ 
 2447, /* 2449 */ 
 2447, /* 2457 */ 
 2459, /* 2465 */ 
 2473, /* 2473 */ 
 2477, /* 2481 */ 
 2477, /* 2489 */ 
 2477, /* 2497 */ 
 2503, /* 2505 */ 
 2503, /* 2513 */ 
 2521, /* 2521 */ 
 2521, /* 2529 */ 
 2531, /* 2537 */ 
 2543, /* 2545 */ 
 2551, /* 2553 */ 
 2557, /* 2561 */ 
 2557, /* 2569 */ 
 2557, /* 2577 */ 
 2579, /* 2585 */ 
 2593, /* 2593 */ 
 2593, /* 2601 */ 
 2609, /* 2609 */ 
 2617, /* 2617 */ 
 2621, /* 2625 */ 
 2633, /* 2633 */ 
 2633, /* 2641 */ 
 2647, /* 2649 */ 
 2657, /* 2657 */ 
 2663, /* 2665 */ 
 2671, /* 2673 */ 
 2677, /* 2681 */ 
 2689, /* 2689 */ 
 2693, /* 2697 */ 
 2699, /* 2705 */ 
 2713, /* 2713 */ 
 2719, /* 2721 */ 
 2729, /* 2729 */ 
 2731, /* 2737 */ 
 2741, /* 2745 */ 
 2753, /* 2753 */ 
 2753, /* 2761 */ 
 2767, /* 2769 */ 
 2777, /* 2777 */ 
 2777, /* 2785 */ 
 2791, /* 2793 */ 
 2801, /* 2801 */ 
 2803, /* 2809 */ 
 2803, /* 2817 */ 
 2819, /* 2825 */ 
 2833, /* 2833 */ 
 2837, /* 2841 */ 
 2843, /* 2849 */ 
 2857, /* 2857 */ 
 2861, /* 2865 */ 
 2861, /* 2873 */ 
 2879, /* 2881 */ 
 2887, /* 2889 */ 
 2897, /* 2897 */ 
 2903, /* 2905 */ 
 2909, /* 2913 */ 
 2917, /* 2921 */ 
 2927, /* 2929 */ 
 2927, /* 2937 */ 
 2939, /* 2945 */ 
 2953, /* 2953 */ 
 2957, /* 2961 */ 
 2969, /* 2969 */ 
 2971, /* 2977 */ 
 2971, /* 2985 */ 
 2971, /* 2993 */ 
 3001, /* 3001 */ 
 3001, /* 3009 */ 
 3011, /* 3017 */ 
 3023, /* 3025 */ 
 3023, /* 3033 */ 
 3041, /* 3041 */ 
 3049, /* 3049 */ 
 3049, /* 3057 */ 
 3061, /* 3065 */ 
 3067, /* 3073 */ 
 3079, /* 3081 */ 
 3089, /* 3089 */ 
 3089, /* 3097 */ 
 3089, /* 3105 */ 
 3109, /* 3113 */ 
 3121, /* 3121 */ 
 3121, /* 3129 */ 
 3137, /* 3137 */ 
 3137, /* 3145 */ 
 3137, /* 3153 */ 
 3137, /* 3161 */ 
 3169, /* 3169 */ 
 3169, /* 3177 */ 
 3181, /* 3185 */ 
 3191, /* 3193 */ 
 3191, /* 3201 */ 
 3209, /* 3209 */ 
 3217, /* 3217 */ 
 3221, /* 3225 */ 
 3229, /* 3233 */ 
 3229, /* 3241 */ 
 3229, /* 3249 */ 
 3257, /* 3257 */ 
 3259, /* 3265 */ 
 3271, /* 3273 */ 
 3271, /* 3281 */ 
 3271, /* 3289 */ 
 3271, /* 3297 */ 
 3301, /* 3305 */ 
 3313, /* 3313 */ 
 3319, /* 3321 */ 
 3329, /* 3329 */ 
 3331, /* 3337 */ 
 3343, /* 3345 */ 
 3347, /* 3353 */ 
 3361, /* 3361 */ 
 3361, /* 3369 */ 
 3373, /* 3377 */ 
 3373, /* 3385 */ 
 3391, /* 3393 */ 
 3391, /* 3401 */ 
 3407, /* 3409 */ 
 3413, /* 3417 */ 
 3413, /* 3425 */ 
 3433, /* 3433 */ 
 3433, /* 3441 */ 
 3449, /* 3449 */ 
 3457, /* 3457 */ 
 3463, /* 3465 */ 
 3469, /* 3473 */ 
 3469, /* 3481 */ 
 3469, /* 3489 */ 
 3491, /* 3497 */ 
 3499, /* 3505 */ 
 3511, /* 3513 */ 
 3517, /* 3521 */ 
 3529, /* 3529 */ 
 3533, /* 3537 */ 
 3541, /* 3545 */ 
 3547, /* 3553 */ 
 3559, /* 3561 */ 
 3559, /* 3569 */ 
 3571, /* 3577 */ 
 3583, /* 3585 */ 
 3593, /* 3593 */ 
 3593, /* 3601 */ 
 3607, /* 3609 */ 
 3617, /* 3617 */ 
 3623, /* 3625 */ 
 3631, /* 3633 */ 
 3637, /* 3641 */ 
 3643, /* 3649 */ 
 3643, /* 3657 */ 
 3659, /* 3665 */ 
 3673, /* 3673 */ 
 3677, /* 3681 */ 
 3677, /* 3689 */ 
 3697, /* 3697 */ 
 3701, /* 3705 */ 
 3709, /* 3713 */ 
 3719, /* 3721 */ 
 3727, /* 3729 */ 
 3733, /* 3737 */ 
 3739, /* 3745 */ 
 3739, /* 3753 */ 
 3761, /* 3761 */ 
 3769, /* 3769 */ 
 3769, /* 3777 */ 
 3779, /* 3785 */ 
 3793, /* 3793 */ 
 3797, /* 3801 */ 
 3803, /* 3809 */ 
 3803, /* 3817 */ 
 3823, /* 3825 */ 
 3833, /* 3833 */ 
 3833, /* 3841 */ 
 3847, /* 3849 */ 
 3853, /* 3857 */ 
 3863, /* 3865 */ 
 3863, /* 3873 */ 
 3881, /* 3881 */ 
 3889, /* 3889 */ 
 3889, /* 3897 */ 
 3889, /* 3905 */ 
 3911, /* 3913 */ 
 3919, /* 3921 */ 
 3929, /* 3929 */ 
 3931, /* 3937 */ 
 3943, /* 3945 */ 
 3947, /* 3953 */ 
 3947, /* 3961 */ 
 3967, /* 3969 */ 
 3967, /* 3977 */ 
 3967, /* 3985 */ 
 3989, /* 3993 */ 
 4001, /* 4001 */ 
 4007, /* 4009 */ 
 4013, /* 4017 */ 
 4021, /* 4025 */ 
 4027, /* 4033 */ 
 4027, /* 4041 */ 
 4049, /* 4049 */ 
 4057, /* 4057 */ 
 4057, /* 4065 */ 
 4073, /* 4073 */ 
 4079, /* 4081 */ 
 4079, /* 4089 */ 
 4093, /* 4097 */ 
 4099, /* 4105 */ 
 4111, /* 4113 */ 
 4111, /* 4121 */ 
 4129, /* 4129 */ 
 4133, /* 4137 */ 
 4139, /* 4145 */ 
 4153, /* 4153 */ 
 4159, /* 4161 */ 
 4159, /* 4169 */ 
 4177, /* 4177 */ 
 4177, /* 4185 */ 
 4177, /* 4193 */ 
 4201, /* 4201 */ 
 4201, /* 4209 */ 
 4217, /* 4217 */ 
 4219, /* 4225 */ 
 4231, /* 4233 */ 
 4241, /* 4241 */ 
 4243, /* 4249 */ 
 4253, /* 4257 */ 
 4261, /* 4265 */ 
 4273, /* 4273 */ 
 4273, /* 4281 */ 
 4289, /* 4289 */ 
 4297, /* 4297 */ 
 4297, /* 4305 */ 
 4297, /* 4313 */ 
 4297, /* 4321 */ 
 4327, /* 4329 */ 
 4337, /* 4337 */ 
 4339, /* 4345 */ 
 4349, /* 4353 */ 
 4357, /* 4361 */ 
 4363, /* 4369 */ 
 4373, /* 4377 */ 
 4373, /* 4385 */ 
 4391, /* 4393 */ 
 4397, /* 4401 */ 
 4409, /* 4409 */ 
 4409, /* 4417 */ 
 4423, /* 4425 */ 
 4423, /* 4433 */ 
 4441, /* 4441 */ 
 4447, /* 4449 */ 
 4457, /* 4457 */ 
 4463, /* 4465 */ 
 4463, /* 4473 */ 
 4481, /* 4481 */ 
 4483, /* 4489 */ 
 4493, /* 4497 */ 
 4493, /* 4505 */ 
 4513, /* 4513 */ 
 4519, /* 4521 */ 
 4523, /* 4529 */ 
 4523, /* 4537 */ 
 4523, /* 4545 */ 
 4549, /* 4553 */ 
 4561, /* 4561 */ 
 4567, /* 4569 */ 
 4567, /* 4577 */ 
 4583, /* 4585 */ 
 4591, /* 4593 */ 
 4597, /* 4601 */ 
 4603, /* 4609 */ 
 4603, /* 4617 */ 
 4621, /* 4625 */ 
 4621, /* 4633 */ 
 4639, /* 4641 */ 
 4649, /* 4649 */ 
 4657, /* 4657 */ 
 4663, /* 4665 */ 
 4673, /* 4673 */ 
 4679, /* 4681 */ 
 4679, /* 4689 */ 
 4691, /* 4697 */ 
 4703, /* 4705 */ 
 4703, /* 4713 */ 
 4721, /* 4721 */ 
 4729, /* 4729 */ 
 4733, /* 4737 */ 
 4733, /* 4745 */ 
 4751, /* 4753 */ 
 4759, /* 4761 */ 
 4759, /* 4769 */ 
 4759, /* 4777 */ 
 4783, /* 4785 */ 
 4793, /* 4793 */ 
 4801, /* 4801 */ 
 4801, /* 4809 */ 
 4817, /* 4817 */ 
 4817, /* 4825 */ 
 4831, /* 4833 */ 
 4831, /* 4841 */ 
 4831, /* 4849 */ 
 4831, /* 4857 */ 
 4861, /* 4865 */ 
 4871, /* 4873 */ 
 4877, /* 4881 */ 
 4889, /* 4889 */ 
 4889, /* 4897 */ 
 4903, /* 4905 */ 
 4909, /* 4913 */ 
 4919, /* 4921 */ 
 4919, /* 4929 */ 
 4937, /* 4937 */ 
 4943, /* 4945 */ 
 4951, /* 4953 */ 
 4957, /* 4961 */ 
 4969, /* 4969 */ 
 4973, /* 4977 */ 
 4973, /* 4985 */ 
 4993, /* 4993 */ 
 4999, /* 5001 */ 
 5009, /* 5009 */ 
 5011, /* 5017 */ 
 5023, /* 5025 */ 
 5023, /* 5033 */ 
 5039, /* 5041 */ 
 5039, /* 5049 */ 
 5051, /* 5057 */ 
 5059, /* 5065 */ 
 5059, /* 5073 */ 
 5081, /* 5081 */ 
 5087, /* 5089 */ 
 5087, /* 5097 */ 
 5101, /* 5105 */ 
 5113, /* 5113 */ 
 5119, /* 5121 */ 
 5119, /* 5129 */ 
 5119, /* 5137 */ 
 5119, /* 5145 */ 
 5153, /* 5153 */ 
 5153, /* 5161 */ 
 5167, /* 5169 */ 
 5171, /* 5177 */ 
 5179, /* 5185 */ 
 5189, /* 5193 */ 
 5197, /* 5201 */ 
 5209, /* 5209 */ 
 5209, /* 5217 */ 
 5209, /* 5225 */ 
 5233, /* 5233 */ 
 5237, /* 5241 */ 
 5237, /* 5249 */ 
 5237, /* 5257 */ 
 5261, /* 5265 */ 
 5273, /* 5273 */ 
 5281, /* 5281 */ 
 5281, /* 5289 */ 
 5297, /* 5297 */ 
 5303, /* 5305 */ 
 5309, /* 5313 */ 
 5309, /* 5321 */ 
 5323, /* 5329 */ 
 5333, /* 5337 */ 
 5333, /* 5345 */ 
 5351, /* 5353 */ 
 5351, /* 5361 */ 
 5351, /* 5369 */ 
 5351, /* 5377 */ 
 5381, /* 5385 */ 
 5393, /* 5393 */ 
 5399, /* 5401 */ 
 5407, /* 5409 */ 
 5417, /* 5417 */ 
 5419, /* 5425 */ 
 5431, /* 5433 */ 
 5441, /* 5441 */ 
 5449, /* 5449 */ 
 5449, /* 5457 */ 
 5449, /* 5465 */ 
 5471, /* 5473 */ 
 5479, /* 5481 */ 
 5483, /* 5489 */ 
 5483, /* 5497 */ 
 5503, /* 5505 */ 
 5507, /* 5513 */ 
 5521, /* 5521 */ 
 5527, /* 5529 */ 
 5531, /* 5537 */ 
 5531, /* 5545 */ 
 5531, /* 5553 */ 
 5557, /* 5561 */ 
 5569, /* 5569 */ 
 5573, /* 5577 */ 
 5581, /* 5585 */ 
 5591, /* 5593 */ 
 5591, /* 5601 */ 
 5591, /* 5609 */ 
 5591, /* 5617 */ 
 5623, /* 5625 */ 
 5623, /* 5633 */ 
 5641, /* 5641 */ 
 5647, /* 5649 */ 
 5657, /* 5657 */ 
 5659, /* 5665 */ 
 5669, /* 5673 */ 
 5669, /* 5681 */ 
 5689, /* 5689 */ 
 5693, /* 5697 */ 
 5701, /* 5705 */ 
 5711, /* 5713 */ 
 5717, /* 5721 */ 
 5717, /* 5729 */ 
 5737, /* 5737 */ 
 5743, /* 5745 */ 
 5749, /* 5753 */ 
 5749, /* 5761 */ 
 5749, /* 5769 */ 
 5749, /* 5777 */ 
 5783, /* 5785 */ 
 5791, /* 5793 */ 
 5801, /* 5801 */ 
 5807, /* 5809 */ 
 5813, /* 5817 */ 
 5821, /* 5825 */ 
 5827, /* 5833 */ 
 5839, /* 5841 */ 
 5849, /* 5849 */ 
 5857, /* 5857 */ 
 5861, /* 5865 */ 
 5869, /* 5873 */ 
 5881, /* 5881 */ 
 5881, /* 5889 */ 
 5897, /* 5897 */ 
 5903, /* 5905 */ 
 5903, /* 5913 */ 
 5903, /* 5921 */ 
 5927, /* 5929 */ 
 5927, /* 5937 */ 
 5939, /* 5945 */ 
 5953, /* 5953 */ 
 5953, /* 5961 */ 
 5953, /* 5969 */ 
 5953, /* 5977 */ 
 5981, /* 5985 */ 
 5987, /* 5993 */ 
 5987, /* 6001 */ 
 6007, /* 6009 */ 
 6011, /* 6017 */ 
 6011, /* 6025 */ 
 6029, /* 6033 */ 
 6037, /* 6041 */ 
 6047, /* 6049 */ 
 6053, /* 6057 */ 
 6053, /* 6065 */ 
 6073, /* 6073 */ 
 6079, /* 6081 */ 
 6089, /* 6089 */ 
 6091, /* 6097 */ 
 6101, /* 6105 */ 
 6113, /* 6113 */ 
 6121, /* 6121 */ 
 6121, /* 6129 */ 
 6133, /* 6137 */ 
 6143, /* 6145 */ 
 6151, /* 6153 */ 
 6151, /* 6161 */ 
 6163, /* 6169 */ 
 6173, /* 6177 */ 
 6173, /* 6185 */ 
 6173, /* 6193 */ 
 6199, /* 6201 */ 
 6203, /* 6209 */ 
 6217, /* 6217 */ 
 6221, /* 6225 */ 
 6229, /* 6233 */ 
 6229, /* 6241 */ 
 6247, /* 6249 */ 
 6257, /* 6257 */ 
 6263, /* 6265 */ 
 6271, /* 6273 */ 
 6277, /* 6281 */ 
 6287, /* 6289 */ 
 6287, /* 6297 */ 
 6301, /* 6305 */ 
 6311, /* 6313 */ 
 6317, /* 6321 */ 
 6329, /* 6329 */ 
 6337, /* 6337 */ 
 6343, /* 6345 */ 
 6353, /* 6353 */ 
 6361, /* 6361 */ 
 6367, /* 6369 */ 
 6373, /* 6377 */ 
 6379, /* 6385 */ 
 6389, /* 6393 */ 
 6397, /* 6401 */ 
 6397, /* 6409 */ 
 6397, /* 6417 */ 
 6421, /* 6425 */ 
 6427, /* 6433 */ 
 6427, /* 6441 */ 
 6449, /* 6449 */ 
 6451, /* 6457 */ 
 6451, /* 6465 */ 
 6473, /* 6473 */ 
 6481, /* 6481 */ 
 6481, /* 6489 */ 
 6491, /* 6497 */ 
 6491, /* 6505 */ 
 6491, /* 6513 */ 
 6521, /* 6521 */ 
 6529, /* 6529 */ 
 6529, /* 6537 */ 
 6529, /* 6545 */ 
 6553, /* 6553 */ 
 6553, /* 6561 */ 
 6569, /* 6569 */ 
 6577, /* 6577 */ 
 6581, /* 6585 */ 
 6581, /* 6593 */ 
 6599, /* 6601 */ 
 6607, /* 6609 */ 
 6607, /* 6617 */ 
 6619, /* 6625 */ 
 6619, /* 6633 */ 
 6637, /* 6641 */ 
 6637, /* 6649 */ 
 6653, /* 6657 */ 
 6661, /* 6665 */ 
 6673, /* 6673 */ 
 6679, /* 6681 */ 
 6689, /* 6689 */ 
 6691, /* 6697 */ 
 6703, /* 6705 */ 
 6709, /* 6713 */ 
 6719, /* 6721 */ 
 6719, /* 6729 */ 
 6737, /* 6737 */ 
 6737, /* 6745 */ 
 6737, /* 6753 */ 
 6761, /* 6761 */ 
 6763, /* 6769 */ 
 6763, /* 6777 */ 
 6781, /* 6785 */ 
 6793, /* 6793 */ 
 6793, /* 6801 */ 
 6803, /* 6809 */ 
 6803, /* 6817 */ 
 6823, /* 6825 */ 
 6833, /* 6833 */ 
 6841, /* 6841 */ 
 6841, /* 6849 */ 
 6857, /* 6857 */ 
 6863, /* 6865 */ 
 6871, /* 6873 */ 
 6871, /* 6881 */ 
 6883, /* 6889 */ 
 6883, /* 6897 */ 
 6899, /* 6905 */ 
 6911, /* 6913 */ 
 6917, /* 6921 */ 
 6917, /* 6929 */ 
 6917, /* 6937 */ 
 6917, /* 6945 */ 
 6949, /* 6953 */ 
 6961, /* 6961 */ 
 6967, /* 6969 */ 
 6977, /* 6977 */ 
 6983, /* 6985 */ 
 6991, /* 6993 */ 
 7001, /* 7001 */ 
 7001, /* 7009 */ 
 7013, /* 7017 */ 
 7019, /* 7025 */ 
 7027, /* 7033 */ 
 7039, /* 7041 */ 
 7043, /* 7049 */ 
 7057, /* 7057 */ 
 7057, /* 7065 */ 
 7069, /* 7073 */ 
 7079, /* 7081 */ 
 7079, /* 7089 */ 
 7079, /* 7097 */ 
 7103, /* 7105 */ 
 7109, /* 7113 */ 
 7121, /* 7121 */ 
 7129, /* 7129 */ 
 7129, /* 7137 */ 
 7129, /* 7145 */ 
 7151, /* 7153 */ 
 7159, /* 7161 */ 
 7159, /* 7169 */ 
 7177, /* 7177 */ 
 7177, /* 7185 */ 
 7193, /* 7193 */ 
 7193, /* 7201 */ 
 7207, /* 7209 */ 
 7213, /* 7217 */ 
 7219, /* 7225 */ 
 7229, /* 7233 */ 
 7237, /* 7241 */ 
 7247, /* 7249 */ 
 7253, /* 7257 */ 
 7253, /* 7265 */ 
 7253, /* 7273 */ 
 7253, /* 7281 */ 
 7283, /* 7289 */ 
 7297, /* 7297 */ 
 7297, /* 7305 */ 
 7309, /* 7313 */ 
 7321, /* 7321 */ 
 7321, /* 7329 */ 
 7333, /* 7337 */ 
 7333, /* 7345 */ 
 7351, /* 7353 */ 
 7351, /* 7361 */ 
 7369, /* 7369 */ 
 7369, /* 7377 */ 
 7369, /* 7385 */ 
 7393, /* 7393 */ 
 7393, /* 7401 */ 
 7393, /* 7409 */ 
 7417, /* 7417 */ 
 7417, /* 7425 */ 
 7433, /* 7433 */ 
 7433, /* 7441 */ 
 7433, /* 7449 */ 
 7457, /* 7457 */ 
 7459, /* 7465 */ 
 7459, /* 7473 */ 
 7481, /* 7481 */ 
 7489, /* 7489 */ 
 7489, /* 7497 */ 
 7499, /* 7505 */ 
 7507, /* 7513 */ 
 7517, /* 7521 */ 
 7529, /* 7529 */ 
 7537, /* 7537 */ 
 7541, /* 7545 */ 
 7549, /* 7553 */ 
 7561, /* 7561 */ 
 7561, /* 7569 */ 
 7577, /* 7577 */ 
 7583, /* 7585 */ 
 7591, /* 7593 */ 
 7591, /* 7601 */ 
 7607, /* 7609 */ 
 7607, /* 7617 */ 
 7621, /* 7625 */ 
 7621, /* 7633 */ 
 7639, /* 7641 */ 
 7649, /* 7649 */ 
 7649, /* 7657 */ 
 7649, /* 7665 */ 
 7673, /* 7673 */ 
 7681, /* 7681 */ 
 7687, /* 7689 */ 
 7691, /* 7697 */ 
 7703, /* 7705 */ 
 7703, /* 7713 */ 
 7717, /* 7721 */ 
 7727, /* 7729 */ 
 7727, /* 7737 */ 
 7741, /* 7745 */ 
 7753, /* 7753 */ 
 7759, /* 7761 */ 
 7759, /* 7769 */ 
 7759, /* 7777 */ 
 7759, /* 7785 */ 
 7793, /* 7793 */ 
 7793, /* 7801 */ 
 7793, /* 7809 */ 
 7817, /* 7817 */ 
 7823, /* 7825 */ 
 7829, /* 7833 */ 
 7841, /* 7841 */ 
 7841, /* 7849 */ 
 7853, /* 7857 */ 
 7853, /* 7865 */ 
 7873, /* 7873 */ 
 7879, /* 7881 */ 
 7883, /* 7889 */ 
 7883, /* 7897 */ 
 7901, /* 7905 */ 
 7907, /* 7913 */ 
 7919, /* 7921 */ 
 7927, /* 7929 */ 
 7937, /* 7937 */ 
 7937, /* 7945 */ 
 7951, /* 7953 */ 
 7951, /* 7961 */ 
 7963, /* 7969 */ 
 7963, /* 7977 */ 
 7963, /* 7985 */ 
 7993, /* 7993 */ 
 7993, /* 8001 */ 
 8009, /* 8009 */ 
 8017, /* 8017 */ 
 8017, /* 8025 */ 
 8017, /* 8033 */ 
 8039, /* 8041 */ 
 8039, /* 8049 */ 
 8053, /* 8057 */ 
 8059, /* 8065 */ 
 8069, /* 8073 */ 
 8081, /* 8081 */ 
 8089, /* 8089 */ 
 8093, /* 8097 */ 
 8101, /* 8105 */ 
 8111, /* 8113 */ 
 8117, /* 8121 */ 
 8123, /* 8129 */ 
 8123, /* 8137 */ 
 8123, /* 8145 */ 
 8147, /* 8153 */ 
 8161, /* 8161 */ 
 8167, /* 8169 */ 
 8171, /* 8177 */ 
 8179, /* 8185 */ 
};
/* 0-64K, increments=64 */
static 
unsigned prime_table1[]={
 1, /* 1 */ 
 61, /* 65 */ 
 127, /* 129 */ 
 193, /* 193 */ 
 257, /* 257 */ 
 317, /* 321 */ 
 383, /* 385 */ 
 449, /* 449 */ 
 509, /* 513 */ 
 577, /* 577 */ 
 641, /* 641 */ 
 701, /* 705 */ 
 769, /* 769 */ 
 829, /* 833 */ 
 887, /* 897 */ 
 953, /* 961 */ 
 1021, /* 1025 */ 
 1087, /* 1089 */ 
 1153, /* 1153 */ 
 1217, /* 1217 */ 
 1279, /* 1281 */ 
 1327, /* 1345 */ 
 1409, /* 1409 */ 
 1471, /* 1473 */ 
 1531, /* 1537 */ 
 1601, /* 1601 */ 
 1663, /* 1665 */ 
 1723, /* 1729 */ 
 1789, /* 1793 */ 
 1847, /* 1857 */ 
 1913, /* 1921 */ 
 1979, /* 1985 */ 
 2039, /* 2049 */ 
 2113, /* 2113 */ 
 2161, /* 2177 */ 
 2239, /* 2241 */ 
 2297, /* 2305 */ 
 2357, /* 2369 */ 
 2423, /* 2433 */ 
 2477, /* 2497 */ 
 2557, /* 2561 */ 
 2621, /* 2625 */ 
 2689, /* 2689 */ 
 2753, /* 2753 */ 
 2803, /* 2817 */ 
 2879, /* 2881 */ 
 2939, /* 2945 */ 
 3001, /* 3009 */ 
 3067, /* 3073 */ 
 3137, /* 3137 */ 
 3191, /* 3201 */ 
 3259, /* 3265 */ 
 3329, /* 3329 */ 
 3391, /* 3393 */ 
 3457, /* 3457 */ 
 3517, /* 3521 */ 
 3583, /* 3585 */ 
 3643, /* 3649 */ 
 3709, /* 3713 */ 
 3769, /* 3777 */ 
 3833, /* 3841 */ 
 3889, /* 3905 */ 
 3967, /* 3969 */ 
 4027, /* 4033 */ 
 4093, /* 4097 */ 
 4159, /* 4161 */ 
 4219, /* 4225 */ 
 4289, /* 4289 */ 
 4349, /* 4353 */ 
 4409, /* 4417 */ 
 4481, /* 4481 */ 
 4523, /* 4545 */ 
 4603, /* 4609 */ 
 4673, /* 4673 */ 
 4733, /* 4737 */ 
 4801, /* 4801 */ 
 4861, /* 4865 */ 
 4919, /* 4929 */ 
 4993, /* 4993 */ 
 5051, /* 5057 */ 
 5119, /* 5121 */ 
 5179, /* 5185 */ 
 5237, /* 5249 */ 
 5309, /* 5313 */ 
 5351, /* 5377 */ 
 5441, /* 5441 */ 
 5503, /* 5505 */ 
 5569, /* 5569 */ 
 5623, /* 5633 */ 
 5693, /* 5697 */ 
 5749, /* 5761 */ 
 5821, /* 5825 */ 
 5881, /* 5889 */ 
 5953, /* 5953 */ 
 6011, /* 6017 */ 
 6079, /* 6081 */ 
 6143, /* 6145 */ 
 6203, /* 6209 */ 
 6271, /* 6273 */ 
 6337, /* 6337 */ 
 6397, /* 6401 */ 
 6451, /* 6465 */ 
 6529, /* 6529 */ 
 6581, /* 6593 */ 
 6653, /* 6657 */ 
 6719, /* 6721 */ 
 6781, /* 6785 */ 
 6841, /* 6849 */ 
 6911, /* 6913 */ 
 6977, /* 6977 */ 
 7039, /* 7041 */ 
 7103, /* 7105 */ 
 7159, /* 7169 */ 
 7229, /* 7233 */ 
 7297, /* 7297 */ 
 7351, /* 7361 */ 
 7417, /* 7425 */ 
 7489, /* 7489 */ 
 7549, /* 7553 */ 
 7607, /* 7617 */ 
 7681, /* 7681 */ 
 7741, /* 7745 */ 
 7793, /* 7809 */ 
 7873, /* 7873 */ 
 7937, /* 7937 */ 
 7993, /* 8001 */ 
 8059, /* 8065 */ 
 8123, /* 8129 */ 
 8191, /* 8193 */ 
 8243, /* 8257 */ 
 8317, /* 8321 */ 
 8377, /* 8385 */ 
 8447, /* 8449 */ 
 8513, /* 8513 */ 
 8573, /* 8577 */ 
 8641, /* 8641 */ 
 8699, /* 8705 */ 
 8761, /* 8769 */ 
 8831, /* 8833 */ 
 8893, /* 8897 */ 
 8951, /* 8961 */ 
 9013, /* 9025 */ 
 9067, /* 9089 */ 
 9151, /* 9153 */ 
 9209, /* 9217 */ 
 9281, /* 9281 */ 
 9343, /* 9345 */ 
 9403, /* 9409 */ 
 9473, /* 9473 */ 
 9533, /* 9537 */ 
 9601, /* 9601 */ 
 9661, /* 9665 */ 
 9721, /* 9729 */ 
 9791, /* 9793 */ 
 9857, /* 9857 */ 
 9907, /* 9921 */ 
 9973, /* 9985 */ 
 10039, /* 10049 */ 
 10111, /* 10113 */ 
 10177, /* 10177 */ 
 10223, /* 10241 */ 
 10303, /* 10305 */ 
 10369, /* 10369 */ 
 10433, /* 10433 */ 
 10487, /* 10497 */ 
 10559, /* 10561 */ 
 10613, /* 10625 */ 
 10687, /* 10689 */ 
 10753, /* 10753 */ 
 10799, /* 10817 */ 
 10867, /* 10881 */ 
 10939, /* 10945 */ 
 11003, /* 11009 */ 
 11071, /* 11073 */ 
 11131, /* 11137 */ 
 11197, /* 11201 */ 
 11261, /* 11265 */ 
 11329, /* 11329 */ 
 11393, /* 11393 */ 
 11447, /* 11457 */ 
 11519, /* 11521 */ 
 11579, /* 11585 */ 
 11633, /* 11649 */ 
 11701, /* 11713 */ 
 11777, /* 11777 */ 
 11839, /* 11841 */ 
 11903, /* 11905 */ 
 11969, /* 11969 */ 
 12011, /* 12033 */ 
 12097, /* 12097 */ 
 12161, /* 12161 */ 
 12211, /* 12225 */ 
 12289, /* 12289 */ 
 12347, /* 12353 */ 
 12413, /* 12417 */ 
 12479, /* 12481 */ 
 12541, /* 12545 */ 
 12601, /* 12609 */ 
 12671, /* 12673 */ 
 12721, /* 12737 */ 
 12799, /* 12801 */ 
 12853, /* 12865 */ 
 12923, /* 12929 */ 
 12983, /* 12993 */ 
 13049, /* 13057 */ 
 13121, /* 13121 */ 
 13183, /* 13185 */ 
 13249, /* 13249 */ 
 13313, /* 13313 */ 
 13367, /* 13377 */ 
 13441, /* 13441 */ 
 13499, /* 13505 */ 
 13567, /* 13569 */ 
 13633, /* 13633 */ 
 13697, /* 13697 */ 
 13759, /* 13761 */ 
 13807, /* 13825 */ 
 13883, /* 13889 */ 
 13933, /* 13953 */ 
 14011, /* 14017 */ 
 14081, /* 14081 */ 
 14143, /* 14145 */ 
 14207, /* 14209 */ 
 14251, /* 14273 */ 
 14327, /* 14337 */ 
 14401, /* 14401 */ 
 14461, /* 14465 */ 
 14519, /* 14529 */ 
 14593, /* 14593 */ 
 14657, /* 14657 */ 
 14717, /* 14721 */ 
 14783, /* 14785 */ 
 14843, /* 14849 */ 
 14897, /* 14913 */ 
 14969, /* 14977 */ 
 15031, /* 15041 */ 
 15101, /* 15105 */ 
 15161, /* 15169 */ 
 15233, /* 15233 */ 
 15289, /* 15297 */ 
 15361, /* 15361 */ 
 15413, /* 15425 */ 
 15473, /* 15489 */ 
 15551, /* 15553 */ 
 15607, /* 15617 */ 
 15679, /* 15681 */ 
 15739, /* 15745 */ 
 15809, /* 15809 */ 
 15859, /* 15873 */ 
 15937, /* 15937 */ 
 16001, /* 16001 */ 
 16063, /* 16065 */ 
 16127, /* 16129 */ 
 16193, /* 16193 */ 
 16253, /* 16257 */ 
 16319, /* 16321 */ 
 16381, /* 16385 */ 
 16447, /* 16449 */ 
 16493, /* 16513 */ 
 16573, /* 16577 */ 
 16633, /* 16641 */ 
 16703, /* 16705 */ 
 16763, /* 16769 */ 
 16831, /* 16833 */ 
 16889, /* 16897 */ 
 16943, /* 16961 */ 
 17021, /* 17025 */ 
 17077, /* 17089 */ 
 17137, /* 17153 */ 
 17209, /* 17217 */ 
 17257, /* 17281 */ 
 17341, /* 17345 */ 
 17401, /* 17409 */ 
 17471, /* 17473 */ 
 17519, /* 17537 */ 
 17599, /* 17601 */ 
 17659, /* 17665 */ 
 17729, /* 17729 */ 
 17791, /* 17793 */ 
 17851, /* 17857 */ 
 17921, /* 17921 */ 
 17981, /* 17985 */ 
 18049, /* 18049 */ 
 18097, /* 18113 */ 
 18169, /* 18177 */ 
 18233, /* 18241 */ 
 18301, /* 18305 */ 
 18367, /* 18369 */ 
 18433, /* 18433 */ 
 18493, /* 18497 */ 
 18553, /* 18561 */ 
 18617, /* 18625 */ 
 18679, /* 18689 */ 
 18749, /* 18753 */ 
 18803, /* 18817 */ 
 18869, /* 18881 */ 
 18919, /* 18945 */ 
 19009, /* 19009 */ 
 19073, /* 19073 */ 
 19121, /* 19137 */ 
 19183, /* 19201 */ 
 19259, /* 19265 */ 
 19319, /* 19329 */ 
 19391, /* 19393 */ 
 19457, /* 19457 */ 
 19507, /* 19521 */ 
 19583, /* 19585 */ 
 19609, /* 19649 */ 
 19709, /* 19713 */ 
 19777, /* 19777 */ 
 19841, /* 19841 */ 
 19891, /* 19905 */ 
 19963, /* 19969 */ 
 20029, /* 20033 */ 
 20089, /* 20097 */ 
 20161, /* 20161 */ 
 20219, /* 20225 */ 
 20287, /* 20289 */ 
 20353, /* 20353 */ 
 20411, /* 20417 */ 
 20479, /* 20481 */ 
 20543, /* 20545 */ 
 20599, /* 20609 */ 
 20663, /* 20673 */ 
 20731, /* 20737 */ 
 20789, /* 20801 */ 
 20857, /* 20865 */ 
 20929, /* 20929 */ 
 20983, /* 20993 */ 
 21031, /* 21057 */ 
 21121, /* 21121 */ 
 21179, /* 21185 */ 
 21247, /* 21249 */ 
 21313, /* 21313 */ 
 21377, /* 21377 */ 
 21433, /* 21441 */ 
 21503, /* 21505 */ 
 21569, /* 21569 */ 
 21617, /* 21633 */ 
 21683, /* 21697 */ 
 21757, /* 21761 */ 
 21821, /* 21825 */ 
 21881, /* 21889 */ 
 21943, /* 21953 */ 
 22013, /* 22017 */ 
 22079, /* 22081 */ 
 22133, /* 22145 */ 
 22193, /* 22209 */ 
 22273, /* 22273 */ 
 22307, /* 22337 */ 
 22397, /* 22401 */ 
 22453, /* 22465 */ 
 22511, /* 22529 */ 
 22573, /* 22593 */ 
 22651, /* 22657 */ 
 22721, /* 22721 */ 
 22783, /* 22785 */ 
 22817, /* 22849 */ 
 22907, /* 22913 */ 
 22973, /* 22977 */ 
 23041, /* 23041 */ 
 23099, /* 23105 */ 
 23167, /* 23169 */ 
 23227, /* 23233 */ 
 23297, /* 23297 */ 
 23357, /* 23361 */ 
 23417, /* 23425 */ 
 23473, /* 23489 */ 
 23549, /* 23553 */ 
 23609, /* 23617 */ 
 23677, /* 23681 */ 
 23743, /* 23745 */ 
 23801, /* 23809 */ 
 23873, /* 23873 */ 
 23929, /* 23937 */ 
 24001, /* 24001 */ 
 24061, /* 24065 */ 
 24121, /* 24129 */ 
 24181, /* 24193 */ 
 24251, /* 24257 */ 
 24317, /* 24321 */ 
 24379, /* 24385 */ 
 24443, /* 24449 */ 
 24509, /* 24513 */ 
 24571, /* 24577 */ 
 24631, /* 24641 */ 
 24697, /* 24705 */ 
 24767, /* 24769 */ 
 24821, /* 24833 */ 
 24889, /* 24897 */ 
 24953, /* 24961 */ 
 25013, /* 25025 */ 
 25087, /* 25089 */ 
 25153, /* 25153 */ 
 25189, /* 25217 */ 
 25261, /* 25281 */ 
 25343, /* 25345 */ 
 25409, /* 25409 */ 
 25471, /* 25473 */ 
 25537, /* 25537 */ 
 25601, /* 25601 */ 
 25657, /* 25665 */ 
 25717, /* 25729 */ 
 25793, /* 25793 */ 
 25849, /* 25857 */ 
 25919, /* 25921 */ 
 25981, /* 25985 */ 
 26041, /* 26049 */ 
 26113, /* 26113 */ 
 26177, /* 26177 */ 
 26237, /* 26241 */ 
 26297, /* 26305 */ 
 26357, /* 26369 */ 
 26431, /* 26433 */ 
 26497, /* 26497 */ 
 26561, /* 26561 */ 
 26597, /* 26625 */ 
 26687, /* 26689 */ 
 26737, /* 26753 */ 
 26813, /* 26817 */ 
 26881, /* 26881 */ 
 26927, /* 26945 */ 
 26993, /* 27009 */ 
 27073, /* 27073 */ 
 27127, /* 27137 */ 
 27197, /* 27201 */ 
 27259, /* 27265 */ 
 27329, /* 27329 */ 
 27367, /* 27393 */ 
 27457, /* 27457 */ 
 27509, /* 27521 */ 
 27583, /* 27585 */ 
 27647, /* 27649 */ 
 27701, /* 27713 */ 
 27773, /* 27777 */ 
 27827, /* 27841 */ 
 27901, /* 27905 */ 
 27967, /* 27969 */ 
 28031, /* 28033 */ 
 28097, /* 28097 */ 
 28151, /* 28161 */ 
 28219, /* 28225 */ 
 28289, /* 28289 */ 
 28351, /* 28353 */ 
 28411, /* 28417 */ 
 28477, /* 28481 */ 
 28541, /* 28545 */ 
 28607, /* 28609 */ 
 28669, /* 28673 */ 
 28729, /* 28737 */ 
 28793, /* 28801 */ 
 28859, /* 28865 */ 
 28927, /* 28929 */ 
 28979, /* 28993 */ 
 29033, /* 29057 */ 
 29101, /* 29121 */ 
 29179, /* 29185 */ 
 29243, /* 29249 */ 
 29311, /* 29313 */ 
 29363, /* 29377 */ 
 29437, /* 29441 */ 
 29501, /* 29505 */ 
 29569, /* 29569 */ 
 29633, /* 29633 */ 
 29683, /* 29697 */ 
 29761, /* 29761 */ 
 29819, /* 29825 */ 
 29881, /* 29889 */ 
 29947, /* 29953 */ 
 30013, /* 30017 */ 
 30071, /* 30081 */ 
 30139, /* 30145 */ 
 30203, /* 30209 */ 
 30271, /* 30273 */ 
 30323, /* 30337 */ 
 30391, /* 30401 */ 
 30449, /* 30465 */ 
 30529, /* 30529 */ 
 30593, /* 30593 */ 
 30649, /* 30657 */ 
 30713, /* 30721 */ 
 30781, /* 30785 */ 
 30841, /* 30849 */ 
 30911, /* 30913 */ 
 30977, /* 30977 */ 
 31039, /* 31041 */ 
 31091, /* 31105 */ 
 31159, /* 31169 */ 
 31231, /* 31233 */ 
 31277, /* 31297 */ 
 31357, /* 31361 */ 
 31397, /* 31425 */ 
 31489, /* 31489 */ 
 31547, /* 31553 */ 
 31607, /* 31617 */ 
 31667, /* 31681 */ 
 31741, /* 31745 */ 
 31799, /* 31809 */ 
 31873, /* 31873 */ 
 31907, /* 31937 */ 
 31991, /* 32001 */ 
 32063, /* 32065 */ 
 32119, /* 32129 */ 
 32191, /* 32193 */ 
 32257, /* 32257 */ 
 32321, /* 32321 */ 
 32381, /* 32385 */ 
 32443, /* 32449 */ 
 32507, /* 32513 */ 
 32573, /* 32577 */ 
 32633, /* 32641 */ 
 32693, /* 32705 */ 
 32749, /* 32769 */ 
 32833, /* 32833 */ 
 32887, /* 32897 */ 
 32957, /* 32961 */ 
 33023, /* 33025 */ 
 33083, /* 33089 */ 
 33151, /* 33153 */ 
 33211, /* 33217 */ 
 33247, /* 33281 */ 
 33343, /* 33345 */ 
 33409, /* 33409 */ 
 33469, /* 33473 */ 
 33533, /* 33537 */ 
 33601, /* 33601 */ 
 33647, /* 33665 */ 
 33721, /* 33729 */ 
 33791, /* 33793 */ 
 33857, /* 33857 */ 
 33911, /* 33921 */ 
 33967, /* 33985 */ 
 34039, /* 34049 */ 
 34061, /* 34113 */ 
 34171, /* 34177 */ 
 34231, /* 34241 */ 
 34303, /* 34305 */ 
 34369, /* 34369 */ 
 34429, /* 34433 */ 
 34487, /* 34497 */ 
 34549, /* 34561 */ 
 34613, /* 34625 */ 
 34687, /* 34689 */ 
 34747, /* 34753 */ 
 34807, /* 34817 */ 
 34877, /* 34881 */ 
 34939, /* 34945 */ 
 34981, /* 35009 */ 
 35069, /* 35073 */ 
 35129, /* 35137 */ 
 35201, /* 35201 */ 
 35257, /* 35265 */ 
 35327, /* 35329 */ 
 35393, /* 35393 */ 
 35449, /* 35457 */ 
 35521, /* 35521 */ 
 35573, /* 35585 */ 
 35617, /* 35649 */ 
 35677, /* 35713 */ 
 35771, /* 35777 */ 
 35839, /* 35841 */ 
 35899, /* 35905 */ 
 35969, /* 35969 */ 
 36017, /* 36033 */ 
 36097, /* 36097 */ 
 36161, /* 36161 */ 
 36217, /* 36225 */ 
 36277, /* 36289 */ 
 36353, /* 36353 */ 
 36389, /* 36417 */ 
 36479, /* 36481 */ 
 36541, /* 36545 */ 
 36607, /* 36609 */ 
 36671, /* 36673 */ 
 36721, /* 36737 */ 
 36793, /* 36801 */ 
 36857, /* 36865 */ 
 36929, /* 36929 */ 
 36979, /* 36993 */ 
 37057, /* 37057 */ 
 37117, /* 37121 */ 
 37181, /* 37185 */ 
 37243, /* 37249 */ 
 37313, /* 37313 */ 
 37369, /* 37377 */ 
 37441, /* 37441 */ 
 37501, /* 37505 */ 
 37567, /* 37569 */ 
 37633, /* 37633 */ 
 37693, /* 37697 */ 
 37747, /* 37761 */ 
 37813, /* 37825 */ 
 37889, /* 37889 */ 
 37951, /* 37953 */ 
 38011, /* 38017 */ 
 38069, /* 38081 */ 
 38119, /* 38145 */ 
 38201, /* 38209 */ 
 38273, /* 38273 */ 
 38333, /* 38337 */ 
 38393, /* 38401 */ 
 38461, /* 38465 */ 
 38501, /* 38529 */ 
 38593, /* 38593 */ 
 38653, /* 38657 */ 
 38713, /* 38721 */ 
 38783, /* 38785 */ 
 38839, /* 38849 */ 
 38903, /* 38913 */ 
 38977, /* 38977 */ 
 39041, /* 39041 */ 
 39103, /* 39105 */ 
 39163, /* 39169 */ 
 39233, /* 39233 */ 
 39293, /* 39297 */ 
 39359, /* 39361 */ 
 39419, /* 39425 */ 
 39461, /* 39489 */ 
 39551, /* 39553 */ 
 39607, /* 39617 */ 
 39679, /* 39681 */ 
 39733, /* 39745 */ 
 39799, /* 39809 */ 
 39869, /* 39873 */ 
 39937, /* 39937 */ 
 39989, /* 40001 */ 
 40063, /* 40065 */ 
 40129, /* 40129 */ 
 40193, /* 40193 */ 
 40253, /* 40257 */ 
 40289, /* 40321 */ 
 40361, /* 40385 */ 
 40433, /* 40449 */ 
 40507, /* 40513 */ 
 40577, /* 40577 */ 
 40639, /* 40641 */ 
 40699, /* 40705 */ 
 40763, /* 40769 */ 
 40829, /* 40833 */ 
 40897, /* 40897 */ 
 40961, /* 40961 */ 
 41023, /* 41025 */ 
 41081, /* 41089 */ 
 41149, /* 41153 */ 
 41213, /* 41217 */ 
 41281, /* 41281 */ 
 41341, /* 41345 */ 
 41399, /* 41409 */ 
 41467, /* 41473 */ 
 41521, /* 41537 */ 
 41597, /* 41601 */ 
 41659, /* 41665 */ 
 41729, /* 41729 */ 
 41777, /* 41793 */ 
 41851, /* 41857 */ 
 41911, /* 41921 */ 
 41983, /* 41985 */ 
 42043, /* 42049 */ 
 42101, /* 42113 */ 
 42169, /* 42177 */ 
 42239, /* 42241 */ 
 42299, /* 42305 */ 
 42359, /* 42369 */ 
 42433, /* 42433 */ 
 42491, /* 42497 */ 
 42557, /* 42561 */ 
 42611, /* 42625 */ 
 42689, /* 42689 */ 
 42751, /* 42753 */ 
 42797, /* 42817 */ 
 42863, /* 42881 */ 
 42943, /* 42945 */ 
 43003, /* 43009 */ 
 43067, /* 43073 */ 
 43133, /* 43137 */ 
 43201, /* 43201 */ 
 43261, /* 43265 */ 
 43321, /* 43329 */ 
 43391, /* 43393 */ 
 43457, /* 43457 */ 
 43517, /* 43521 */ 
 43579, /* 43585 */ 
 43649, /* 43649 */ 
 43711, /* 43713 */ 
 43777, /* 43777 */ 
 43801, /* 43841 */ 
 43891, /* 43905 */ 
 43969, /* 43969 */ 
 44029, /* 44033 */ 
 44089, /* 44097 */ 
 44159, /* 44161 */ 
 44221, /* 44225 */ 
 44281, /* 44289 */ 
 44351, /* 44353 */ 
 44417, /* 44417 */ 
 44453, /* 44481 */ 
 44543, /* 44545 */ 
 44587, /* 44609 */ 
 44657, /* 44673 */ 
 44729, /* 44737 */ 
 44797, /* 44801 */ 
 44851, /* 44865 */ 
 44927, /* 44929 */ 
 44987, /* 44993 */ 
 45053, /* 45057 */ 
 45121, /* 45121 */ 
 45181, /* 45185 */ 
 45247, /* 45249 */ 
 45307, /* 45313 */ 
 45377, /* 45377 */ 
 45439, /* 45441 */ 
 45503, /* 45505 */ 
 45569, /* 45569 */ 
 45631, /* 45633 */ 
 45697, /* 45697 */ 
 45757, /* 45761 */ 
 45823, /* 45825 */ 
 45887, /* 45889 */ 
 45953, /* 45953 */ 
 45989, /* 46017 */ 
 46073, /* 46081 */ 
 46141, /* 46145 */ 
 46199, /* 46209 */ 
 46273, /* 46273 */ 
 46337, /* 46337 */ 
 46399, /* 46401 */ 
 46457, /* 46465 */ 
 46523, /* 46529 */ 
 46591, /* 46593 */ 
 46649, /* 46657 */ 
 46703, /* 46721 */ 
 46771, /* 46785 */ 
 46831, /* 46849 */ 
 46901, /* 46913 */ 
 46957, /* 46977 */ 
 47041, /* 47041 */ 
 47093, /* 47105 */ 
 47161, /* 47169 */ 
 47221, /* 47233 */ 
 47297, /* 47297 */ 
 47353, /* 47361 */ 
 47419, /* 47425 */ 
 47459, /* 47489 */ 
 47543, /* 47553 */ 
 47609, /* 47617 */ 
 47681, /* 47681 */ 
 47743, /* 47745 */ 
 47809, /* 47809 */ 
 47869, /* 47873 */ 
 47933, /* 47937 */ 
 47981, /* 48001 */ 
 48049, /* 48065 */ 
 48121, /* 48129 */ 
 48193, /* 48193 */ 
 48247, /* 48257 */ 
 48313, /* 48321 */ 
 48383, /* 48385 */ 
 48449, /* 48449 */ 
 48497, /* 48513 */ 
 48571, /* 48577 */ 
 48623, /* 48641 */ 
 48679, /* 48705 */ 
 48767, /* 48769 */ 
 48823, /* 48833 */ 
 48889, /* 48897 */ 
 48953, /* 48961 */ 
 49019, /* 49025 */ 
 49081, /* 49089 */ 
 49139, /* 49153 */ 
 49211, /* 49217 */ 
 49279, /* 49281 */ 
 49339, /* 49345 */ 
 49409, /* 49409 */ 
 49463, /* 49473 */ 
 49537, /* 49537 */ 
 49597, /* 49601 */ 
 49663, /* 49665 */ 
 49727, /* 49729 */ 
 49789, /* 49793 */ 
 49853, /* 49857 */ 
 49921, /* 49921 */ 
 49957, /* 49985 */ 
 50047, /* 50049 */ 
 50111, /* 50113 */ 
 50177, /* 50177 */ 
 50231, /* 50241 */ 
 50291, /* 50305 */ 
 50363, /* 50369 */ 
 50423, /* 50433 */ 
 50497, /* 50497 */ 
 50551, /* 50561 */ 
 50599, /* 50625 */ 
 50683, /* 50689 */ 
 50753, /* 50753 */ 
 50789, /* 50817 */ 
 50873, /* 50881 */ 
 50929, /* 50945 */ 
 51001, /* 51009 */ 
 51071, /* 51073 */ 
 51137, /* 51137 */ 
 51199, /* 51201 */ 
 51263, /* 51265 */ 
 51329, /* 51329 */ 
 51383, /* 51393 */ 
 51449, /* 51457 */ 
 51521, /* 51521 */ 
 51581, /* 51585 */ 
 51647, /* 51649 */ 
 51713, /* 51713 */ 
 51769, /* 51777 */ 
 51839, /* 51841 */ 
 51899, /* 51905 */ 
 51949, /* 51969 */ 
 52027, /* 52033 */ 
 52081, /* 52097 */ 
 52153, /* 52161 */ 
 52223, /* 52225 */ 
 52289, /* 52289 */ 
 52321, /* 52353 */ 
 52391, /* 52417 */ 
 52457, /* 52481 */ 
 52543, /* 52545 */ 
 52609, /* 52609 */ 
 52673, /* 52673 */ 
 52733, /* 52737 */ 
 52783, /* 52801 */ 
 52861, /* 52865 */ 
 52919, /* 52929 */ 
 52981, /* 52993 */ 
 53051, /* 53057 */ 
 53117, /* 53121 */ 
 53173, /* 53185 */ 
 53239, /* 53249 */ 
 53309, /* 53313 */ 
 53377, /* 53377 */ 
 53441, /* 53441 */ 
 53503, /* 53505 */ 
 53569, /* 53569 */ 
 53633, /* 53633 */ 
 53693, /* 53697 */ 
 53759, /* 53761 */ 
 53819, /* 53825 */ 
 53887, /* 53889 */ 
 53951, /* 53953 */ 
 54013, /* 54017 */ 
 54059, /* 54081 */ 
 54139, /* 54145 */ 
 54193, /* 54209 */ 
 54269, /* 54273 */ 
 54331, /* 54337 */ 
 54401, /* 54401 */ 
 54449, /* 54465 */ 
 54521, /* 54529 */ 
 54583, /* 54593 */ 
 54647, /* 54657 */ 
 54721, /* 54721 */ 
 54779, /* 54785 */ 
 54833, /* 54849 */ 
 54907, /* 54913 */ 
 54973, /* 54977 */ 
 55021, /* 55041 */ 
 55103, /* 55105 */ 
 55163, /* 55169 */ 
 55229, /* 55233 */ 
 55291, /* 55297 */ 
 55351, /* 55361 */ 
 55411, /* 55425 */ 
 55487, /* 55489 */ 
 55547, /* 55553 */ 
 55609, /* 55617 */ 
 55681, /* 55681 */ 
 55733, /* 55745 */ 
 55807, /* 55809 */ 
 55871, /* 55873 */ 
 55933, /* 55937 */ 
 55997, /* 56001 */ 
 56053, /* 56065 */ 
 56123, /* 56129 */ 
 56179, /* 56193 */ 
 56249, /* 56257 */ 
 56311, /* 56321 */ 
 56383, /* 56385 */ 
 56443, /* 56449 */ 
 56509, /* 56513 */ 
 56569, /* 56577 */ 
 56633, /* 56641 */ 
 56701, /* 56705 */ 
 56767, /* 56769 */ 
 56827, /* 56833 */ 
 56897, /* 56897 */ 
 56957, /* 56961 */ 
 56999, /* 57025 */ 
 57089, /* 57089 */ 
 57149, /* 57153 */ 
 57203, /* 57217 */ 
 57271, /* 57281 */ 
 57331, /* 57345 */ 
 57397, /* 57409 */ 
 57467, /* 57473 */ 
 57529, /* 57537 */ 
 57601, /* 57601 */ 
 57653, /* 57665 */ 
 57727, /* 57729 */ 
 57793, /* 57793 */ 
 57853, /* 57857 */ 
 57917, /* 57921 */ 
 57977, /* 57985 */ 
 58049, /* 58049 */ 
 58111, /* 58113 */ 
 58171, /* 58177 */ 
 58237, /* 58241 */ 
 58271, /* 58305 */ 
 58369, /* 58369 */ 
 58427, /* 58433 */ 
 58481, /* 58497 */ 
 58549, /* 58561 */ 
 58613, /* 58625 */ 
 58687, /* 58689 */ 
 58741, /* 58753 */ 
 58789, /* 58817 */ 
 58831, /* 58881 */ 
 58943, /* 58945 */ 
 59009, /* 59009 */ 
 59069, /* 59073 */ 
 59123, /* 59137 */ 
 59197, /* 59201 */ 
 59263, /* 59265 */ 
 59281, /* 59329 */ 
 59393, /* 59393 */ 
 59453, /* 59457 */ 
 59513, /* 59521 */ 
 59581, /* 59585 */ 
 59629, /* 59649 */ 
 59707, /* 59713 */ 
 59771, /* 59777 */ 
 59833, /* 59841 */ 
 59887, /* 59905 */ 
 59957, /* 59969 */ 
 60029, /* 60033 */ 
 60091, /* 60097 */ 
 60161, /* 60161 */ 
 60223, /* 60225 */ 
 60289, /* 60289 */ 
 60353, /* 60353 */ 
 60413, /* 60417 */ 
 60457, /* 60481 */ 
 60539, /* 60545 */ 
 60607, /* 60609 */ 
 60661, /* 60673 */ 
 60737, /* 60737 */ 
 60793, /* 60801 */ 
 60859, /* 60865 */ 
 60923, /* 60929 */ 
 60961, /* 60993 */ 
 61057, /* 61057 */ 
 61121, /* 61121 */ 
 61169, /* 61185 */ 
 61231, /* 61249 */ 
 61297, /* 61313 */ 
 61363, /* 61377 */ 
 61441, /* 61441 */ 
 61493, /* 61505 */ 
 61561, /* 61569 */ 
 61631, /* 61633 */ 
 61687, /* 61697 */ 
 61757, /* 61761 */ 
 61819, /* 61825 */ 
 61879, /* 61889 */ 
 61949, /* 61953 */ 
 62017, /* 62017 */ 
 62081, /* 62081 */ 
 62143, /* 62145 */ 
 62207, /* 62209 */ 
 62273, /* 62273 */ 
 62327, /* 62337 */ 
 62401, /* 62401 */ 
 62459, /* 62465 */ 
 62507, /* 62529 */ 
 62591, /* 62593 */ 
 62653, /* 62657 */ 
 62701, /* 62721 */ 
 62773, /* 62785 */ 
 62827, /* 62849 */ 
 62903, /* 62913 */ 
 62971, /* 62977 */ 
 63031, /* 63041 */ 
 63103, /* 63105 */ 
 63149, /* 63169 */ 
 63211, /* 63233 */ 
 63281, /* 63297 */ 
 63361, /* 63361 */ 
 63421, /* 63425 */ 
 63487, /* 63489 */ 
 63541, /* 63553 */ 
 63617, /* 63617 */ 
 63671, /* 63681 */ 
 63743, /* 63745 */ 
 63809, /* 63809 */ 
 63863, /* 63873 */ 
 63929, /* 63937 */ 
 63997, /* 64001 */ 
 64063, /* 64065 */ 
 64123, /* 64129 */ 
 64189, /* 64193 */ 
 64237, /* 64257 */ 
 64319, /* 64321 */ 
 64381, /* 64385 */ 
 64439, /* 64449 */ 
 64513, /* 64513 */ 
 64577, /* 64577 */ 
 64633, /* 64641 */ 
 64693, /* 64705 */ 
 64763, /* 64769 */ 
 64817, /* 64833 */ 
 64891, /* 64897 */ 
 64951, /* 64961 */ 
 65011, /* 65025 */ 
 65089, /* 65089 */ 
 65147, /* 65153 */ 
 65213, /* 65217 */ 
 65269, /* 65281 */ 
 65327, /* 65345 */ 
 65407, /* 65409 */ 
 65449, /* 65473 */ 
};
/* 0-1M, increments=1024 */
static 
unsigned prime_table2[1024]={
  1021, /* 1024 */ 
  2039, /* 2048 */ 
  3067, /* 3072 */ 
  4093, /* 4096 */ 
  5119, /* 5120 */ 
  6143, /* 6144 */ 
  7159, /* 7168 */ 
  8191, /* 8192 */ 
  9209, /* 9216 */ 
  10223, /* 10240 */ 
  11261, /* 11264 */ 
  12281, /* 12288 */ 
  13309, /* 13312 */ 
  14327, /* 14336 */ 
  15359, /* 15360 */ 
  16381, /* 16384 */ 
  17401, /* 17408 */ 
  18427, /* 18432 */ 
  19447, /* 19456 */ 
  20479, /* 20480 */ 
  21503, /* 21504 */ 
  22511, /* 22528 */ 
  23549, /* 23552 */ 
  24571, /* 24576 */ 
  25589, /* 25600 */ 
  26597, /* 26624 */ 
  27647, /* 27648 */ 
  28669, /* 28672 */ 
  29683, /* 29696 */ 
  30713, /* 30720 */ 
  31741, /* 31744 */ 
  32749, /* 32768 */ 
  33791, /* 33792 */ 
  34807, /* 34816 */ 
  35839, /* 35840 */ 
  36857, /* 36864 */ 
  37879, /* 37888 */ 
  38903, /* 38912 */ 
  39929, /* 39936 */ 
  40949, /* 40960 */ 
  41983, /* 41984 */ 
  43003, /* 43008 */ 
  44029, /* 44032 */ 
  45053, /* 45056 */ 
  46073, /* 46080 */ 
  47093, /* 47104 */ 
  48121, /* 48128 */ 
  49139, /* 49152 */ 
  50159, /* 50176 */ 
  51199, /* 51200 */ 
  52223, /* 52224 */ 
  53239, /* 53248 */ 
  54269, /* 54272 */ 
  55291, /* 55296 */ 
  56311, /* 56320 */ 
  57331, /* 57344 */ 
  58367, /* 58368 */ 
  59387, /* 59392 */ 
  60413, /* 60416 */ 
  61417, /* 61440 */ 
  62459, /* 62464 */ 
  63487, /* 63488 */ 
  64499, /* 64512 */ 
  65521, /* 65536 */ 
  66553, /* 66560 */ 
  67579, /* 67584 */ 
  68597, /* 68608 */ 
  69623, /* 69632 */ 
  70639, /* 70656 */ 
  71671, /* 71680 */ 
  72701, /* 72704 */ 
  73727, /* 73728 */ 
  74747, /* 74752 */ 
  75773, /* 75776 */ 
  76781, /* 76800 */ 
  77813, /* 77824 */ 
  78839, /* 78848 */ 
  79867, /* 79872 */ 
  80863, /* 80896 */ 
  81919, /* 81920 */ 
  82939, /* 82944 */ 
  83939, /* 83968 */ 
  84991, /* 84992 */ 
  86011, /* 86016 */ 
  87037, /* 87040 */ 
  88037, /* 88064 */ 
  89087, /* 89088 */ 
  90107, /* 90112 */ 
  91129, /* 91136 */ 
  92153, /* 92160 */ 
  93179, /* 93184 */ 
  94207, /* 94208 */ 
  95231, /* 95232 */ 
  96233, /* 96256 */ 
  97259, /* 97280 */ 
  98299, /* 98304 */ 
  99317, /* 99328 */ 
  100343, /* 100352 */ 
  101363, /* 101376 */ 
  102397, /* 102400 */ 
  103423, /* 103424 */ 
  104417, /* 104448 */ 
  105467, /* 105472 */ 
  106487, /* 106496 */ 
  107509, /* 107520 */ 
  108541, /* 108544 */ 
  109567, /* 109568 */ 
  110587, /* 110592 */ 
  111611, /* 111616 */ 
  112621, /* 112640 */ 
  113657, /* 113664 */ 
  114679, /* 114688 */ 
  115693, /* 115712 */ 
  116731, /* 116736 */ 
  117757, /* 117760 */ 
  118757, /* 118784 */ 
  119797, /* 119808 */ 
  120829, /* 120832 */ 
  121853, /* 121856 */ 
  122869, /* 122880 */ 
  123887, /* 123904 */ 
  124919, /* 124928 */ 
  125941, /* 125952 */ 
  126967, /* 126976 */ 
  127997, /* 128000 */ 
  129023, /* 129024 */ 
  130043, /* 130048 */ 
  131071, /* 131072 */ 
  132071, /* 132096 */ 
  133117, /* 133120 */ 
  134129, /* 134144 */ 
  135151, /* 135168 */ 
  136189, /* 136192 */ 
  137209, /* 137216 */ 
  138239, /* 138240 */ 
  139241, /* 139264 */ 
  140281, /* 140288 */ 
  141311, /* 141312 */ 
  142327, /* 142336 */ 
  143357, /* 143360 */ 
  144383, /* 144384 */ 
  145399, /* 145408 */ 
  146423, /* 146432 */ 
  147451, /* 147456 */ 
  148471, /* 148480 */ 
  149503, /* 149504 */ 
  150523, /* 150528 */ 
  151549, /* 151552 */ 
  152567, /* 152576 */ 
  153589, /* 153600 */ 
  154621, /* 154624 */ 
  155627, /* 155648 */ 
  156671, /* 156672 */ 
  157679, /* 157696 */ 
  158699, /* 158720 */ 
  159739, /* 159744 */ 
  160757, /* 160768 */ 
  161783, /* 161792 */ 
  162791, /* 162816 */ 
  163819, /* 163840 */ 
  164839, /* 164864 */ 
  165887, /* 165888 */ 
  166909, /* 166912 */ 
  167917, /* 167936 */ 
  168943, /* 168960 */ 
  169957, /* 169984 */ 
  171007, /* 171008 */ 
  172031, /* 172032 */ 
  173053, /* 173056 */ 
  174079, /* 174080 */ 
  175103, /* 175104 */ 
  176123, /* 176128 */ 
  177131, /* 177152 */ 
  178169, /* 178176 */ 
  179173, /* 179200 */ 
  180221, /* 180224 */ 
  181243, /* 181248 */ 
  182261, /* 182272 */ 
  183289, /* 183296 */ 
  184309, /* 184320 */ 
  185327, /* 185344 */ 
  186343, /* 186368 */ 
  187387, /* 187392 */ 
  188407, /* 188416 */ 
  189439, /* 189440 */ 
  190409, /* 190464 */ 
  191473, /* 191488 */ 
  192499, /* 192512 */ 
  193513, /* 193536 */ 
  194543, /* 194560 */ 
  195581, /* 195584 */ 
  196597, /* 196608 */ 
  197621, /* 197632 */ 
  198647, /* 198656 */ 
  199679, /* 199680 */ 
  200699, /* 200704 */ 
  201709, /* 201728 */ 
  202751, /* 202752 */ 
  203773, /* 203776 */ 
  204797, /* 204800 */ 
  205823, /* 205824 */ 
  206827, /* 206848 */ 
  207869, /* 207872 */ 
  208891, /* 208896 */ 
  209917, /* 209920 */ 
  210943, /* 210944 */ 
  211949, /* 211968 */ 
  212987, /* 212992 */ 
  214009, /* 214016 */ 
  214993, /* 215040 */ 
  216061, /* 216064 */ 
  217081, /* 217088 */ 
  218111, /* 218112 */ 
  219133, /* 219136 */ 
  220151, /* 220160 */ 
  221173, /* 221184 */ 
  222199, /* 222208 */ 
  223229, /* 223232 */ 
  224251, /* 224256 */ 
  225263, /* 225280 */ 
  226283, /* 226304 */ 
  227303, /* 227328 */ 
  228341, /* 228352 */ 
  229373, /* 229376 */ 
  230393, /* 230400 */ 
  231419, /* 231424 */ 
  232439, /* 232448 */ 
  233437, /* 233472 */ 
  234473, /* 234496 */ 
  235519, /* 235520 */ 
  236527, /* 236544 */ 
  237563, /* 237568 */ 
  238591, /* 238592 */ 
  239611, /* 239616 */ 
  240631, /* 240640 */ 
  241663, /* 241664 */ 
  242681, /* 242688 */ 
  243709, /* 243712 */ 
  244733, /* 244736 */ 
  245759, /* 245760 */ 
  246781, /* 246784 */ 
  247799, /* 247808 */ 
  248827, /* 248832 */ 
  249853, /* 249856 */ 
  250871, /* 250880 */ 
  251903, /* 251904 */ 
  252919, /* 252928 */ 
  253951, /* 253952 */ 
  254971, /* 254976 */ 
  255989, /* 256000 */ 
  257017, /* 257024 */ 
  258031, /* 258048 */ 
  259033, /* 259072 */ 
  260089, /* 260096 */ 
  261101, /* 261120 */ 
  262139, /* 262144 */ 
  263167, /* 263168 */ 
  264179, /* 264192 */ 
  265207, /* 265216 */ 
  266239, /* 266240 */ 
  267259, /* 267264 */ 
  268283, /* 268288 */ 
  269281, /* 269312 */ 
  270329, /* 270336 */ 
  271357, /* 271360 */ 
  272383, /* 272384 */ 
  273367, /* 273408 */ 
  274423, /* 274432 */ 
  275453, /* 275456 */ 
  276467, /* 276480 */ 
  277499, /* 277504 */ 
  278503, /* 278528 */ 
  279551, /* 279552 */ 
  280561, /* 280576 */ 
  281581, /* 281600 */ 
  282617, /* 282624 */ 
  283639, /* 283648 */ 
  284659, /* 284672 */ 
  285673, /* 285696 */ 
  286711, /* 286720 */ 
  287731, /* 287744 */ 
  288767, /* 288768 */ 
  289789, /* 289792 */ 
  290803, /* 290816 */ 
  291833, /* 291840 */ 
  292849, /* 292864 */ 
  293863, /* 293888 */ 
  294911, /* 294912 */ 
  295909, /* 295936 */ 
  296941, /* 296960 */ 
  297971, /* 297984 */ 
  298999, /* 299008 */ 
  300023, /* 300032 */ 
  301051, /* 301056 */ 
  302053, /* 302080 */ 
  303097, /* 303104 */ 
  304127, /* 304128 */ 
  305147, /* 305152 */ 
  306169, /* 306176 */ 
  307189, /* 307200 */ 
  308219, /* 308224 */ 
  309241, /* 309248 */ 
  310243, /* 310272 */ 
  311293, /* 311296 */ 
  312313, /* 312320 */ 
  313343, /* 313344 */ 
  314359, /* 314368 */ 
  315389, /* 315392 */ 
  316403, /* 316416 */ 
  317437, /* 317440 */ 
  318457, /* 318464 */ 
  319483, /* 319488 */ 
  320483, /* 320512 */ 
  321509, /* 321536 */ 
  322559, /* 322560 */ 
  323581, /* 323584 */ 
  324593, /* 324608 */ 
  325631, /* 325632 */ 
  326633, /* 326656 */ 
  327673, /* 327680 */ 
  328687, /* 328704 */ 
  329723, /* 329728 */ 
  330749, /* 330752 */ 
  331769, /* 331776 */ 
  332791, /* 332800 */ 
  333821, /* 333824 */ 
  334843, /* 334848 */ 
  335857, /* 335872 */ 
  336887, /* 336896 */ 
  337919, /* 337920 */ 
  338927, /* 338944 */ 
  339959, /* 339968 */ 
  340979, /* 340992 */ 
  341993, /* 342016 */ 
  343037, /* 343040 */ 
  344053, /* 344064 */ 
  345067, /* 345088 */ 
  346111, /* 346112 */ 
  347131, /* 347136 */ 
  348149, /* 348160 */ 
  349183, /* 349184 */ 
  350191, /* 350208 */ 
  351229, /* 351232 */ 
  352249, /* 352256 */ 
  353263, /* 353280 */ 
  354301, /* 354304 */ 
  355321, /* 355328 */ 
  356351, /* 356352 */ 
  357359, /* 357376 */ 
  358373, /* 358400 */ 
  359419, /* 359424 */ 
  360439, /* 360448 */ 
  361469, /* 361472 */ 
  362473, /* 362496 */ 
  363497, /* 363520 */ 
  364543, /* 364544 */ 
  365567, /* 365568 */ 
  366547, /* 366592 */ 
  367613, /* 367616 */ 
  368633, /* 368640 */ 
  369661, /* 369664 */ 
  370687, /* 370688 */ 
  371699, /* 371712 */ 
  372733, /* 372736 */ 
  373757, /* 373760 */ 
  374783, /* 374784 */ 
  375799, /* 375808 */ 
  376823, /* 376832 */ 
  377851, /* 377856 */ 
  378869, /* 378880 */ 
  379903, /* 379904 */ 
  380917, /* 380928 */ 
  381949, /* 381952 */ 
  382961, /* 382976 */ 
  383987, /* 384000 */ 
  385013, /* 385024 */ 
  386047, /* 386048 */ 
  387071, /* 387072 */ 
  388081, /* 388096 */ 
  389117, /* 389120 */ 
  390119, /* 390144 */ 
  391163, /* 391168 */ 
  392177, /* 392192 */ 
  393209, /* 393216 */ 
  394223, /* 394240 */ 
  395261, /* 395264 */ 
  396269, /* 396288 */ 
  397303, /* 397312 */ 
  398323, /* 398336 */ 
  399353, /* 399360 */ 
  400381, /* 400384 */ 
  401407, /* 401408 */ 
  402419, /* 402432 */ 
  403439, /* 403456 */ 
  404461, /* 404480 */ 
  405499, /* 405504 */ 
  406517, /* 406528 */ 
  407527, /* 407552 */ 
  408563, /* 408576 */ 
  409597, /* 409600 */ 
  410623, /* 410624 */ 
  411641, /* 411648 */ 
  412667, /* 412672 */ 
  413689, /* 413696 */ 
  414709, /* 414720 */ 
  415729, /* 415744 */ 
  416761, /* 416768 */ 
  417773, /* 417792 */ 
  418813, /* 418816 */ 
  419831, /* 419840 */ 
  420859, /* 420864 */ 
  421847, /* 421888 */ 
  422911, /* 422912 */ 
  423931, /* 423936 */ 
  424939, /* 424960 */ 
  425977, /* 425984 */ 
  427001, /* 427008 */ 
  428027, /* 428032 */ 
  429043, /* 429056 */ 
  430061, /* 430080 */ 
  431099, /* 431104 */ 
  432121, /* 432128 */ 
  433151, /* 433152 */ 
  434167, /* 434176 */ 
  435191, /* 435200 */ 
  436217, /* 436224 */ 
  437243, /* 437248 */ 
  438271, /* 438272 */ 
  439289, /* 439296 */ 
  440311, /* 440320 */ 
  441319, /* 441344 */ 
  442367, /* 442368 */ 
  443389, /* 443392 */ 
  444403, /* 444416 */ 
  445433, /* 445440 */ 
  446461, /* 446464 */ 
  447481, /* 447488 */ 
  448451, /* 448512 */ 
  449473, /* 449536 */ 
  450557, /* 450560 */ 
  451579, /* 451584 */ 
  452597, /* 452608 */ 
  453631, /* 453632 */ 
  454637, /* 454656 */ 
  455659, /* 455680 */ 
  456697, /* 456704 */ 
  457711, /* 457728 */ 
  458747, /* 458752 */ 
  459763, /* 459776 */ 
  460793, /* 460800 */ 
  461819, /* 461824 */ 
  462841, /* 462848 */ 
  463867, /* 463872 */ 
  464879, /* 464896 */ 
  465917, /* 465920 */ 
  466919, /* 466944 */ 
  467963, /* 467968 */ 
  468983, /* 468992 */ 
  469993, /* 470016 */ 
  471007, /* 471040 */ 
  472063, /* 472064 */ 
  473027, /* 473088 */ 
  474101, /* 474112 */ 
  475109, /* 475136 */ 
  476143, /* 476160 */ 
  477163, /* 477184 */ 
  478207, /* 478208 */ 
  479231, /* 479232 */ 
  480209, /* 480256 */ 
  481249, /* 481280 */ 
  482281, /* 482304 */ 
  483323, /* 483328 */ 
  484339, /* 484352 */ 
  485371, /* 485376 */ 
  486397, /* 486400 */ 
  487423, /* 487424 */ 
  488441, /* 488448 */ 
  489457, /* 489472 */ 
  490493, /* 490496 */ 
  491503, /* 491520 */ 
  492523, /* 492544 */ 
  493567, /* 493568 */ 
  494591, /* 494592 */ 
  495613, /* 495616 */ 
  496631, /* 496640 */ 
  497663, /* 497664 */ 
  498679, /* 498688 */ 
  499711, /* 499712 */ 
  500729, /* 500736 */ 
  501731, /* 501760 */ 
  502781, /* 502784 */ 
  503803, /* 503808 */ 
  504821, /* 504832 */ 
  505823, /* 505856 */ 
  506873, /* 506880 */ 
  507901, /* 507904 */ 
  508919, /* 508928 */ 
  509947, /* 509952 */ 
  510943, /* 510976 */ 
  511997, /* 512000 */ 
  513017, /* 513024 */ 
  514021, /* 514048 */ 
  515041, /* 515072 */ 
  516091, /* 516096 */ 
  517091, /* 517120 */ 
  518137, /* 518144 */ 
  519161, /* 519168 */ 
  520151, /* 520192 */ 
  521201, /* 521216 */ 
  522239, /* 522240 */ 
  523261, /* 523264 */ 
  524287, /* 524288 */ 
  525299, /* 525312 */ 
  526307, /* 526336 */ 
  527353, /* 527360 */ 
  528383, /* 528384 */ 
  529393, /* 529408 */ 
  530429, /* 530432 */ 
  531383, /* 531456 */ 
  532453, /* 532480 */ 
  533459, /* 533504 */ 
  534511, /* 534528 */ 
  535547, /* 535552 */ 
  536563, /* 536576 */ 
  537599, /* 537600 */ 
  538621, /* 538624 */ 
  539641, /* 539648 */ 
  540629, /* 540672 */ 
  541693, /* 541696 */ 
  542719, /* 542720 */ 
  543713, /* 543744 */ 
  544759, /* 544768 */ 
  545791, /* 545792 */ 
  546781, /* 546816 */ 
  547831, /* 547840 */ 
  548861, /* 548864 */ 
  549883, /* 549888 */ 
  550909, /* 550912 */ 
  551933, /* 551936 */ 
  552917, /* 552960 */ 
  553981, /* 553984 */ 
  554977, /* 555008 */ 
  556027, /* 556032 */ 
  557041, /* 557056 */ 
  558067, /* 558080 */ 
  559099, /* 559104 */ 
  560123, /* 560128 */ 
  561109, /* 561152 */ 
  562169, /* 562176 */ 
  563197, /* 563200 */ 
  564197, /* 564224 */ 
  565247, /* 565248 */ 
  566233, /* 566272 */ 
  567277, /* 567296 */ 
  568303, /* 568320 */ 
  569323, /* 569344 */ 
  570359, /* 570368 */ 
  571381, /* 571392 */ 
  572399, /* 572416 */ 
  573437, /* 573440 */ 
  574439, /* 574464 */ 
  575479, /* 575488 */ 
  576509, /* 576512 */ 
  577531, /* 577536 */ 
  578537, /* 578560 */ 
  579583, /* 579584 */ 
  580607, /* 580608 */ 
  581617, /* 581632 */ 
  582649, /* 582656 */ 
  583673, /* 583680 */ 
  584699, /* 584704 */ 
  585727, /* 585728 */ 
  586741, /* 586752 */ 
  587773, /* 587776 */ 
  588779, /* 588800 */ 
  589811, /* 589824 */ 
  590839, /* 590848 */ 
  591863, /* 591872 */ 
  592877, /* 592896 */ 
  593903, /* 593920 */ 
  594931, /* 594944 */ 
  595967, /* 595968 */ 
  596987, /* 596992 */ 
  598007, /* 598016 */ 
  599023, /* 599040 */ 
  600053, /* 600064 */ 
  601079, /* 601088 */ 
  602111, /* 602112 */ 
  603133, /* 603136 */ 
  604073, /* 604160 */ 
  605177, /* 605184 */ 
  606181, /* 606208 */ 
  607219, /* 607232 */ 
  608213, /* 608256 */ 
  609277, /* 609280 */ 
  610301, /* 610304 */ 
  611323, /* 611328 */ 
  612349, /* 612352 */ 
  613367, /* 613376 */ 
  614387, /* 614400 */ 
  615413, /* 615424 */ 
  616439, /* 616448 */ 
  617471, /* 617472 */ 
  618463, /* 618496 */ 
  619511, /* 619520 */ 
  620531, /* 620544 */ 
  621541, /* 621568 */ 
  622577, /* 622592 */ 
  623591, /* 623616 */ 
  624607, /* 624640 */ 
  625663, /* 625664 */ 
  626687, /* 626688 */ 
  627709, /* 627712 */ 
  628721, /* 628736 */ 
  629747, /* 629760 */ 
  630737, /* 630784 */ 
  631789, /* 631808 */ 
  632813, /* 632832 */ 
  633833, /* 633856 */ 
  634871, /* 634880 */ 
  635893, /* 635904 */ 
  636919, /* 636928 */ 
  637939, /* 637952 */ 
  638971, /* 638976 */ 
  639997, /* 640000 */ 
  640993, /* 641024 */ 
  642013, /* 642048 */ 
  643061, /* 643072 */ 
  644089, /* 644096 */ 
  645097, /* 645120 */ 
  646103, /* 646144 */ 
  647161, /* 647168 */ 
  648191, /* 648192 */ 
  649183, /* 649216 */ 
  650227, /* 650240 */ 
  651257, /* 651264 */ 
  652283, /* 652288 */ 
  653311, /* 653312 */ 
  654323, /* 654336 */ 
  655357, /* 655360 */ 
  656377, /* 656384 */ 
  657403, /* 657408 */ 
  658417, /* 658432 */ 
  659453, /* 659456 */ 
  660449, /* 660480 */ 
  661483, /* 661504 */ 
  662527, /* 662528 */ 
  663547, /* 663552 */ 
  664571, /* 664576 */ 
  665591, /* 665600 */ 
  666607, /* 666624 */ 
  667643, /* 667648 */ 
  668671, /* 668672 */ 
  669689, /* 669696 */ 
  670711, /* 670720 */ 
  671743, /* 671744 */ 
  672767, /* 672768 */ 
  673787, /* 673792 */ 
  674813, /* 674816 */ 
  675839, /* 675840 */ 
  676861, /* 676864 */ 
  677857, /* 677888 */ 
  678907, /* 678912 */ 
  679933, /* 679936 */ 
  680959, /* 680960 */ 
  681983, /* 681984 */ 
  683003, /* 683008 */ 
  684017, /* 684032 */ 
  685051, /* 685056 */ 
  686057, /* 686080 */ 
  687101, /* 687104 */ 
  688111, /* 688128 */ 
  689141, /* 689152 */ 
  690163, /* 690176 */ 
  691199, /* 691200 */ 
  692221, /* 692224 */ 
  693223, /* 693248 */ 
  694271, /* 694272 */ 
  695293, /* 695296 */ 
  696317, /* 696320 */ 
  697327, /* 697344 */ 
  698359, /* 698368 */ 
  699383, /* 699392 */ 
  700393, /* 700416 */ 
  701419, /* 701440 */ 
  702451, /* 702464 */ 
  703471, /* 703488 */ 
  704507, /* 704512 */ 
  705533, /* 705536 */ 
  706547, /* 706560 */ 
  707573, /* 707584 */ 
  708601, /* 708608 */ 
  709609, /* 709632 */ 
  710641, /* 710656 */ 
  711679, /* 711680 */ 
  712697, /* 712704 */ 
  713681, /* 713728 */ 
  714751, /* 714752 */ 
  715753, /* 715776 */ 
  716789, /* 716800 */ 
  717817, /* 717824 */ 
  718847, /* 718848 */ 
  719839, /* 719872 */ 
  720887, /* 720896 */ 
  721909, /* 721920 */ 
  722933, /* 722944 */ 
  723967, /* 723968 */ 
  724991, /* 724992 */ 
  726013, /* 726016 */ 
  727021, /* 727040 */ 
  728047, /* 728064 */ 
  729073, /* 729088 */ 
  730111, /* 730112 */ 
  731117, /* 731136 */ 
  732157, /* 732160 */ 
  733177, /* 733184 */ 
  734207, /* 734208 */ 
  735211, /* 735232 */ 
  736249, /* 736256 */ 
  737279, /* 737280 */ 
  738301, /* 738304 */ 
  739327, /* 739328 */ 
  740351, /* 740352 */ 
  741373, /* 741376 */ 
  742393, /* 742400 */ 
  743423, /* 743424 */ 
  744431, /* 744448 */ 
  745471, /* 745472 */ 
  746483, /* 746496 */ 
  747499, /* 747520 */ 
  748541, /* 748544 */ 
  749557, /* 749568 */ 
  750571, /* 750592 */ 
  751613, /* 751616 */ 
  752639, /* 752640 */ 
  753659, /* 753664 */ 
  754651, /* 754688 */ 
  755707, /* 755712 */ 
  756727, /* 756736 */ 
  757753, /* 757760 */ 
  758783, /* 758784 */ 
  759799, /* 759808 */ 
  760813, /* 760832 */ 
  761833, /* 761856 */ 
  762877, /* 762880 */ 
  763901, /* 763904 */ 
  764903, /* 764928 */ 
  765949, /* 765952 */ 
  766967, /* 766976 */ 
  767957, /* 768000 */ 
  769019, /* 769024 */ 
  770047, /* 770048 */ 
  771049, /* 771072 */ 
  772091, /* 772096 */ 
  773117, /* 773120 */ 
  774143, /* 774144 */ 
  775163, /* 775168 */ 
  776183, /* 776192 */ 
  777209, /* 777216 */ 
  778237, /* 778240 */ 
  779249, /* 779264 */ 
  780287, /* 780288 */ 
  781309, /* 781312 */ 
  782329, /* 782336 */ 
  783359, /* 783360 */ 
  784379, /* 784384 */ 
  785377, /* 785408 */ 
  786431, /* 786432 */ 
  787447, /* 787456 */ 
  788479, /* 788480 */ 
  789493, /* 789504 */ 
  790523, /* 790528 */ 
  791543, /* 791552 */ 
  792563, /* 792576 */ 
  793591, /* 793600 */ 
  794593, /* 794624 */ 
  795647, /* 795648 */ 
  796657, /* 796672 */ 
  797689, /* 797696 */ 
  798713, /* 798720 */ 
  799741, /* 799744 */ 
  800759, /* 800768 */ 
  801791, /* 801792 */ 
  802811, /* 802816 */ 
  803819, /* 803840 */ 
  804857, /* 804864 */ 
  805877, /* 805888 */ 
  806903, /* 806912 */ 
  807931, /* 807936 */ 
  808957, /* 808960 */ 
  809983, /* 809984 */ 
  810989, /* 811008 */ 
  812011, /* 812032 */ 
  813049, /* 813056 */ 
  814069, /* 814080 */ 
  815063, /* 815104 */ 
  816121, /* 816128 */ 
  817151, /* 817152 */ 
  818173, /* 818176 */ 
  819187, /* 819200 */ 
  820223, /* 820224 */ 
  821209, /* 821248 */ 
  822259, /* 822272 */ 
  823283, /* 823296 */ 
  824287, /* 824320 */ 
  825343, /* 825344 */ 
  826363, /* 826368 */ 
  827389, /* 827392 */ 
  828409, /* 828416 */ 
  829399, /* 829440 */ 
  830449, /* 830464 */ 
  831461, /* 831488 */ 
  832499, /* 832512 */ 
  833509, /* 833536 */ 
  834527, /* 834560 */ 
  835559, /* 835584 */ 
  836573, /* 836608 */ 
  837631, /* 837632 */ 
  838633, /* 838656 */ 
  839669, /* 839680 */ 
  840703, /* 840704 */ 
  841727, /* 841728 */ 
  842747, /* 842752 */ 
  843763, /* 843776 */ 
  844777, /* 844800 */ 
  845809, /* 845824 */ 
  846841, /* 846848 */ 
  847871, /* 847872 */ 
  848893, /* 848896 */ 
  849917, /* 849920 */ 
  850943, /* 850944 */ 
  851957, /* 851968 */ 
  852989, /* 852992 */ 
  853999, /* 854016 */ 
  855031, /* 855040 */ 
  856061, /* 856064 */ 
  857083, /* 857088 */ 
  858103, /* 858112 */ 
  859121, /* 859136 */ 
  860143, /* 860160 */ 
  861167, /* 861184 */ 
  862207, /* 862208 */ 
  863231, /* 863232 */ 
  864251, /* 864256 */ 
  865261, /* 865280 */ 
  866293, /* 866304 */ 
  867319, /* 867328 */ 
  868349, /* 868352 */ 
  869371, /* 869376 */ 
  870391, /* 870400 */ 
  871393, /* 871424 */ 
  872441, /* 872448 */ 
  873469, /* 873472 */ 
  874487, /* 874496 */ 
  875519, /* 875520 */ 
  876529, /* 876544 */ 
  877567, /* 877568 */ 
  878573, /* 878592 */ 
  879607, /* 879616 */ 
  880603, /* 880640 */ 
  881663, /* 881664 */ 
  882659, /* 882688 */ 
  883703, /* 883712 */ 
  884717, /* 884736 */ 
  885737, /* 885760 */ 
  886777, /* 886784 */ 
  887759, /* 887808 */ 
  888827, /* 888832 */ 
  889829, /* 889856 */ 
  890867, /* 890880 */ 
  891899, /* 891904 */ 
  892919, /* 892928 */ 
  893939, /* 893952 */ 
  894973, /* 894976 */ 
  895987, /* 896000 */ 
  897019, /* 897024 */ 
  898033, /* 898048 */ 
  899069, /* 899072 */ 
  900091, /* 900096 */ 
  901111, /* 901120 */ 
  902141, /* 902144 */ 
  903163, /* 903168 */ 
  904181, /* 904192 */ 
  905213, /* 905216 */ 
  906233, /* 906240 */ 
  907259, /* 907264 */ 
  908287, /* 908288 */ 
  909301, /* 909312 */ 
  910307, /* 910336 */ 
  911359, /* 911360 */ 
  912367, /* 912384 */ 
  913397, /* 913408 */ 
  914429, /* 914432 */ 
  915451, /* 915456 */ 
  916477, /* 916480 */ 
  917503, /* 917504 */ 
  918497, /* 918528 */ 
  919531, /* 919552 */ 
  920561, /* 920576 */ 
  921589, /* 921600 */ 
  922619, /* 922624 */ 
  923641, /* 923648 */ 
  924661, /* 924672 */ 
  925679, /* 925696 */ 
  926707, /* 926720 */ 
  927743, /* 927744 */ 
  928703, /* 928768 */ 
  929791, /* 929792 */ 
  930779, /* 930816 */ 
  931837, /* 931840 */ 
  932863, /* 932864 */ 
  933883, /* 933888 */ 
  934909, /* 934912 */ 
  935903, /* 935936 */ 
  936953, /* 936960 */ 
  937969, /* 937984 */ 
  939007, /* 939008 */ 
  940031, /* 940032 */ 
  941041, /* 941056 */ 
  942079, /* 942080 */ 
  943097, /* 943104 */ 
  944123, /* 944128 */ 
  945151, /* 945152 */ 
  946163, /* 946176 */ 
  947197, /* 947200 */ 
  948187, /* 948224 */ 
  949243, /* 949248 */ 
  950269, /* 950272 */ 
  951283, /* 951296 */ 
  952313, /* 952320 */ 
  953341, /* 953344 */ 
  954367, /* 954368 */ 
  955391, /* 955392 */ 
  956401, /* 956416 */ 
  957433, /* 957440 */ 
  958459, /* 958464 */ 
  959479, /* 959488 */ 
  960499, /* 960512 */ 
  961531, /* 961536 */ 
  962543, /* 962560 */ 
  963581, /* 963584 */ 
  964589, /* 964608 */ 
  965623, /* 965632 */ 
  966653, /* 966656 */ 
  967667, /* 967680 */ 
  968699, /* 968704 */ 
  969721, /* 969728 */ 
  970747, /* 970752 */ 
  971767, /* 971776 */ 
  972799, /* 972800 */ 
  973823, /* 973824 */ 
  974837, /* 974848 */ 
  975869, /* 975872 */ 
  976883, /* 976896 */ 
  977897, /* 977920 */ 
  978931, /* 978944 */ 
  979949, /* 979968 */ 
  980963, /* 980992 */ 
  981983, /* 982016 */ 
  982981, /* 983040 */ 
  984059, /* 984064 */ 
  985079, /* 985088 */ 
  986101, /* 986112 */ 
  987127, /* 987136 */ 
  988157, /* 988160 */ 
  989173, /* 989184 */ 
  990181, /* 990208 */ 
  991229, /* 991232 */ 
  992249, /* 992256 */ 
  993269, /* 993280 */ 
  994303, /* 994304 */ 
  995327, /* 995328 */ 
  996329, /* 996352 */ 
  997369, /* 997376 */ 
  998399, /* 998400 */ 
  999389, /* 999424 */ 
  1000429, /* 1000448 */ 
  1001467, /* 1001472 */ 
  1002493, /* 1002496 */ 
  1003517, /* 1003520 */ 
  1004537, /* 1004544 */ 
  1005553, /* 1005568 */ 
  1006589, /* 1006592 */ 
  1007609, /* 1007616 */ 
  1008617, /* 1008640 */ 
  1009651, /* 1009664 */ 
  1010687, /* 1010688 */ 
  1011697, /* 1011712 */ 
  1012733, /* 1012736 */ 
  1013741, /* 1013760 */ 
  1014779, /* 1014784 */ 
  1015769, /* 1015808 */ 
  1016789, /* 1016832 */ 
  1017851, /* 1017856 */ 
  1018879, /* 1018880 */ 
  1019903, /* 1019904 */ 
  1020913, /* 1020928 */ 
  1021919, /* 1021952 */ 
  1022963, /* 1022976 */ 
  1023991, /* 1024000 */ 
  1025021, /* 1025024 */ 
  1026043, /* 1026048 */ 
  1027067, /* 1027072 */ 
  1028089, /* 1028096 */ 
  1029113, /* 1029120 */ 
  1030121, /* 1030144 */ 
  1031161, /* 1031168 */ 
  1032191, /* 1032192 */ 
  1033189, /* 1033216 */ 
  1034239, /* 1034240 */ 
  1035263, /* 1035264 */ 
  1036271, /* 1036288 */ 
  1037303, /* 1037312 */ 
  1038329, /* 1038336 */ 
  1039351, /* 1039360 */ 
  1040381, /* 1040384 */ 
  1041373, /* 1041408 */ 
  1042427, /* 1042432 */ 
  1043453, /* 1043456 */ 
  1044479, /* 1044480 */ 
  1045493, /* 1045504 */ 
  1046527, /* 1046528 */ 
  1047551, /* 1047552 */ 
};
/* 0-128M, increments=102400 */
static 
unsigned prime_table3[1024]={
 131071, /* 131072 */ 
 262139, /* 262144 */ 
 393209, /* 393216 */ 
 524287, /* 524288 */ 
 655357, /* 655360 */ 
 786431, /* 786432 */ 
 917503, /* 917504 */ 
 1048573, /* 1048576 */ 
 1179641, /* 1179648 */ 
 1310719, /* 1310720 */ 
 1441771, /* 1441792 */ 
 1572853, /* 1572864 */ 
 1703903, /* 1703936 */ 
 1835003, /* 1835008 */ 
 1966079, /* 1966080 */ 
 2097143, /* 2097152 */ 
 2228221, /* 2228224 */ 
 2359267, /* 2359296 */ 
 2490337, /* 2490368 */ 
 2621431, /* 2621440 */ 
 2752499, /* 2752512 */ 
 2883577, /* 2883584 */ 
 3014653, /* 3014656 */ 
 3145721, /* 3145728 */ 
 3276799, /* 3276800 */ 
 3407857, /* 3407872 */ 
 3538933, /* 3538944 */ 
 3670013, /* 3670016 */ 
 3801073, /* 3801088 */ 
 3932153, /* 3932160 */ 
 4063217, /* 4063232 */ 
 4194301, /* 4194304 */ 
 4325359, /* 4325376 */ 
 4456433, /* 4456448 */ 
 4587503, /* 4587520 */ 
 4718579, /* 4718592 */ 
 4849651, /* 4849664 */ 
 4980727, /* 4980736 */ 
 5111791, /* 5111808 */ 
 5242877, /* 5242880 */ 
 5373931, /* 5373952 */ 
 5505023, /* 5505024 */ 
 5636077, /* 5636096 */ 
 5767129, /* 5767168 */ 
 5898209, /* 5898240 */ 
 6029299, /* 6029312 */ 
 6160381, /* 6160384 */ 
 6291449, /* 6291456 */ 
 6422519, /* 6422528 */ 
 6553577, /* 6553600 */ 
 6684659, /* 6684672 */ 
 6815741, /* 6815744 */ 
 6946813, /* 6946816 */ 
 7077883, /* 7077888 */ 
 7208951, /* 7208960 */ 
 7340009, /* 7340032 */ 
 7471099, /* 7471104 */ 
 7602151, /* 7602176 */ 
 7733233, /* 7733248 */ 
 7864301, /* 7864320 */ 
 7995391, /* 7995392 */ 
 8126453, /* 8126464 */ 
 8257531, /* 8257536 */ 
 8388593, /* 8388608 */ 
 8519647, /* 8519680 */ 
 8650727, /* 8650752 */ 
 8781797, /* 8781824 */ 
 8912887, /* 8912896 */ 
 9043967, /* 9043968 */ 
 9175037, /* 9175040 */ 
 9306097, /* 9306112 */ 
 9437179, /* 9437184 */ 
 9568219, /* 9568256 */ 
 9699323, /* 9699328 */ 
 9830393, /* 9830400 */ 
 9961463, /* 9961472 */ 
 10092539, /* 10092544 */ 
 10223593, /* 10223616 */ 
 10354667, /* 10354688 */ 
 10485751, /* 10485760 */ 
 10616831, /* 10616832 */ 
 10747903, /* 10747904 */ 
 10878961, /* 10878976 */ 
 11010037, /* 11010048 */ 
 11141113, /* 11141120 */ 
 11272181, /* 11272192 */ 
 11403247, /* 11403264 */ 
 11534329, /* 11534336 */ 
 11665403, /* 11665408 */ 
 11796469, /* 11796480 */ 
 11927551, /* 11927552 */ 
 12058621, /* 12058624 */ 
 12189677, /* 12189696 */ 
 12320753, /* 12320768 */ 
 12451807, /* 12451840 */ 
 12582893, /* 12582912 */ 
 12713959, /* 12713984 */ 
 12845033, /* 12845056 */ 
 12976121, /* 12976128 */ 
 13107197, /* 13107200 */ 
 13238263, /* 13238272 */ 
 13369333, /* 13369344 */ 
 13500373, /* 13500416 */ 
 13631477, /* 13631488 */ 
 13762549, /* 13762560 */ 
 13893613, /* 13893632 */ 
 14024671, /* 14024704 */ 
 14155763, /* 14155776 */ 
 14286809, /* 14286848 */ 
 14417881, /* 14417920 */ 
 14548979, /* 14548992 */ 
 14680063, /* 14680064 */ 
 14811133, /* 14811136 */ 
 14942197, /* 14942208 */ 
 15073277, /* 15073280 */ 
 15204349, /* 15204352 */ 
 15335407, /* 15335424 */ 
 15466463, /* 15466496 */ 
 15597559, /* 15597568 */ 
 15728611, /* 15728640 */ 
 15859687, /* 15859712 */ 
 15990781, /* 15990784 */ 
 16121849, /* 16121856 */ 
 16252919, /* 16252928 */ 
 16383977, /* 16384000 */ 
 16515067, /* 16515072 */ 
 16646099, /* 16646144 */ 
 16777213, /* 16777216 */ 
 16908263, /* 16908288 */ 
 17039339, /* 17039360 */ 
 17170429, /* 17170432 */ 
 17301463, /* 17301504 */ 
 17432561, /* 17432576 */ 
 17563633, /* 17563648 */ 
 17694709, /* 17694720 */ 
 17825791, /* 17825792 */ 
 17956849, /* 17956864 */ 
 18087899, /* 18087936 */ 
 18219001, /* 18219008 */ 
 18350063, /* 18350080 */ 
 18481097, /* 18481152 */ 
 18612211, /* 18612224 */ 
 18743281, /* 18743296 */ 
 18874367, /* 18874368 */ 
 19005433, /* 19005440 */ 
 19136503, /* 19136512 */ 
 19267561, /* 19267584 */ 
 19398647, /* 19398656 */ 
 19529717, /* 19529728 */ 
 19660799, /* 19660800 */ 
 19791869, /* 19791872 */ 
 19922923, /* 19922944 */ 
 20054011, /* 20054016 */ 
 20185051, /* 20185088 */ 
 20316151, /* 20316160 */ 
 20447191, /* 20447232 */ 
 20578297, /* 20578304 */ 
 20709347, /* 20709376 */ 
 20840429, /* 20840448 */ 
 20971507, /* 20971520 */ 
 21102583, /* 21102592 */ 
 21233651, /* 21233664 */ 
 21364727, /* 21364736 */ 
 21495797, /* 21495808 */ 
 21626819, /* 21626880 */ 
 21757951, /* 21757952 */ 
 21889019, /* 21889024 */ 
 22020091, /* 22020096 */ 
 22151167, /* 22151168 */ 
 22282199, /* 22282240 */ 
 22413289, /* 22413312 */ 
 22544351, /* 22544384 */ 
 22675403, /* 22675456 */ 
 22806521, /* 22806528 */ 
 22937591, /* 22937600 */ 
 23068667, /* 23068672 */ 
 23199731, /* 23199744 */ 
 23330773, /* 23330816 */ 
 23461877, /* 23461888 */ 
 23592937, /* 23592960 */ 
 23724031, /* 23724032 */ 
 23855101, /* 23855104 */ 
 23986159, /* 23986176 */ 
 24117217, /* 24117248 */ 
 24248299, /* 24248320 */ 
 24379391, /* 24379392 */ 
 24510463, /* 24510464 */ 
 24641479, /* 24641536 */ 
 24772603, /* 24772608 */ 
 24903667, /* 24903680 */ 
 25034731, /* 25034752 */ 
 25165813, /* 25165824 */ 
 25296893, /* 25296896 */ 
 25427957, /* 25427968 */ 
 25559033, /* 25559040 */ 
 25690097, /* 25690112 */ 
 25821179, /* 25821184 */ 
 25952243, /* 25952256 */ 
 26083273, /* 26083328 */ 
 26214379, /* 26214400 */ 
 26345471, /* 26345472 */ 
 26476543, /* 26476544 */ 
 26607611, /* 26607616 */ 
 26738687, /* 26738688 */ 
 26869753, /* 26869760 */ 
 27000817, /* 27000832 */ 
 27131903, /* 27131904 */ 
 27262931, /* 27262976 */ 
 27394019, /* 27394048 */ 
 27525109, /* 27525120 */ 
 27656149, /* 27656192 */ 
 27787213, /* 27787264 */ 
 27918323, /* 27918336 */ 
 28049407, /* 28049408 */ 
 28180459, /* 28180480 */ 
 28311541, /* 28311552 */ 
 28442551, /* 28442624 */ 
 28573673, /* 28573696 */ 
 28704749, /* 28704768 */ 
 28835819, /* 28835840 */ 
 28966909, /* 28966912 */ 
 29097977, /* 29097984 */ 
 29229047, /* 29229056 */ 
 29360087, /* 29360128 */ 
 29491193, /* 29491200 */ 
 29622269, /* 29622272 */ 
 29753341, /* 29753344 */ 
 29884411, /* 29884416 */ 
 30015481, /* 30015488 */ 
 30146531, /* 30146560 */ 
 30277627, /* 30277632 */ 
 30408701, /* 30408704 */ 
 30539749, /* 30539776 */ 
 30670847, /* 30670848 */ 
 30801917, /* 30801920 */ 
 30932987, /* 30932992 */ 
 31064063, /* 31064064 */ 
 31195117, /* 31195136 */ 
 31326181, /* 31326208 */ 
 31457269, /* 31457280 */ 
 31588351, /* 31588352 */ 
 31719409, /* 31719424 */ 
 31850491, /* 31850496 */ 
 31981567, /* 31981568 */ 
 32112607, /* 32112640 */ 
 32243707, /* 32243712 */ 
 32374781, /* 32374784 */ 
 32505829, /* 32505856 */ 
 32636921, /* 32636928 */ 
 32767997, /* 32768000 */ 
 32899037, /* 32899072 */ 
 33030121, /* 33030144 */ 
 33161201, /* 33161216 */ 
 33292283, /* 33292288 */ 
 33423319, /* 33423360 */ 
 33554393, /* 33554432 */ 
 33685493, /* 33685504 */ 
 33816571, /* 33816576 */ 
 33947621, /* 33947648 */ 
 34078699, /* 34078720 */ 
 34209787, /* 34209792 */ 
 34340861, /* 34340864 */ 
 34471933, /* 34471936 */ 
 34602991, /* 34603008 */ 
 34734079, /* 34734080 */ 
 34865141, /* 34865152 */ 
 34996223, /* 34996224 */ 
 35127263, /* 35127296 */ 
 35258347, /* 35258368 */ 
 35389423, /* 35389440 */ 
 35520467, /* 35520512 */ 
 35651579, /* 35651584 */ 
 35782613, /* 35782656 */ 
 35913727, /* 35913728 */ 
 36044797, /* 36044800 */ 
 36175871, /* 36175872 */ 
 36306937, /* 36306944 */ 
 36438013, /* 36438016 */ 
 36569083, /* 36569088 */ 
 36700159, /* 36700160 */ 
 36831227, /* 36831232 */ 
 36962291, /* 36962304 */ 
 37093373, /* 37093376 */ 
 37224437, /* 37224448 */ 
 37355503, /* 37355520 */ 
 37486591, /* 37486592 */ 
 37617653, /* 37617664 */ 
 37748717, /* 37748736 */ 
 37879783, /* 37879808 */ 
 38010871, /* 38010880 */ 
 38141951, /* 38141952 */ 
 38273023, /* 38273024 */ 
 38404081, /* 38404096 */ 
 38535151, /* 38535168 */ 
 38666219, /* 38666240 */ 
 38797303, /* 38797312 */ 
 38928371, /* 38928384 */ 
 39059431, /* 39059456 */ 
 39190519, /* 39190528 */ 
 39321599, /* 39321600 */ 
 39452671, /* 39452672 */ 
 39583727, /* 39583744 */ 
 39714799, /* 39714816 */ 
 39845887, /* 39845888 */ 
 39976939, /* 39976960 */ 
 40108027, /* 40108032 */ 
 40239103, /* 40239104 */ 
 40370173, /* 40370176 */ 
 40501231, /* 40501248 */ 
 40632313, /* 40632320 */ 
 40763369, /* 40763392 */ 
 40894457, /* 40894464 */ 
 41025499, /* 41025536 */ 
 41156569, /* 41156608 */ 
 41287651, /* 41287680 */ 
 41418739, /* 41418752 */ 
 41549803, /* 41549824 */ 
 41680871, /* 41680896 */ 
 41811949, /* 41811968 */ 
 41943023, /* 41943040 */ 
 42074101, /* 42074112 */ 
 42205183, /* 42205184 */ 
 42336253, /* 42336256 */ 
 42467317, /* 42467328 */ 
 42598397, /* 42598400 */ 
 42729437, /* 42729472 */ 
 42860537, /* 42860544 */ 
 42991609, /* 42991616 */ 
 43122683, /* 43122688 */ 
 43253759, /* 43253760 */ 
 43384813, /* 43384832 */ 
 43515881, /* 43515904 */ 
 43646963, /* 43646976 */ 
 43778011, /* 43778048 */ 
 43909111, /* 43909120 */ 
 44040187, /* 44040192 */ 
 44171261, /* 44171264 */ 
 44302303, /* 44302336 */ 
 44433391, /* 44433408 */ 
 44564461, /* 44564480 */ 
 44695549, /* 44695552 */ 
 44826611, /* 44826624 */ 
 44957687, /* 44957696 */ 
 45088739, /* 45088768 */ 
 45219827, /* 45219840 */ 
 45350869, /* 45350912 */ 
 45481973, /* 45481984 */ 
 45613039, /* 45613056 */ 
 45744121, /* 45744128 */ 
 45875191, /* 45875200 */ 
 46006249, /* 46006272 */ 
 46137319, /* 46137344 */ 
 46268381, /* 46268416 */ 
 46399471, /* 46399488 */ 
 46530557, /* 46530560 */ 
 46661627, /* 46661632 */ 
 46792699, /* 46792704 */ 
 46923761, /* 46923776 */ 
 47054809, /* 47054848 */ 
 47185907, /* 47185920 */ 
 47316991, /* 47316992 */ 
 47448061, /* 47448064 */ 
 47579131, /* 47579136 */ 
 47710207, /* 47710208 */ 
 47841257, /* 47841280 */ 
 47972341, /* 47972352 */ 
 48103417, /* 48103424 */ 
 48234451, /* 48234496 */ 
 48365563, /* 48365568 */ 
 48496639, /* 48496640 */ 
 48627697, /* 48627712 */ 
 48758783, /* 48758784 */ 
 48889837, /* 48889856 */ 
 49020913, /* 49020928 */ 
 49151987, /* 49152000 */ 
 49283063, /* 49283072 */ 
 49414111, /* 49414144 */ 
 49545193, /* 49545216 */ 
 49676267, /* 49676288 */ 
 49807327, /* 49807360 */ 
 49938431, /* 49938432 */ 
 50069497, /* 50069504 */ 
 50200573, /* 50200576 */ 
 50331599, /* 50331648 */ 
 50462683, /* 50462720 */ 
 50593783, /* 50593792 */ 
 50724859, /* 50724864 */ 
 50855899, /* 50855936 */ 
 50987003, /* 50987008 */ 
 51118069, /* 51118080 */ 
 51249131, /* 51249152 */ 
 51380179, /* 51380224 */ 
 51511277, /* 51511296 */ 
 51642341, /* 51642368 */ 
 51773431, /* 51773440 */ 
 51904511, /* 51904512 */ 
 52035569, /* 52035584 */ 
 52166641, /* 52166656 */ 
 52297717, /* 52297728 */ 
 52428767, /* 52428800 */ 
 52559867, /* 52559872 */ 
 52690919, /* 52690944 */ 
 52821983, /* 52822016 */ 
 52953077, /* 52953088 */ 
 53084147, /* 53084160 */ 
 53215229, /* 53215232 */ 
 53346301, /* 53346304 */ 
 53477357, /* 53477376 */ 
 53608441, /* 53608448 */ 
 53739493, /* 53739520 */ 
 53870573, /* 53870592 */ 
 54001663, /* 54001664 */ 
 54132721, /* 54132736 */ 
 54263789, /* 54263808 */ 
 54394877, /* 54394880 */ 
 54525917, /* 54525952 */ 
 54656983, /* 54657024 */ 
 54788089, /* 54788096 */ 
 54919159, /* 54919168 */ 
 55050217, /* 55050240 */ 
 55181311, /* 55181312 */ 
 55312351, /* 55312384 */ 
 55443433, /* 55443456 */ 
 55574507, /* 55574528 */ 
 55705589, /* 55705600 */ 
 55836659, /* 55836672 */ 
 55967701, /* 55967744 */ 
 56098813, /* 56098816 */ 
 56229881, /* 56229888 */ 
 56360911, /* 56360960 */ 
 56491993, /* 56492032 */ 
 56623093, /* 56623104 */ 
 56754167, /* 56754176 */ 
 56885219, /* 56885248 */ 
 57016319, /* 57016320 */ 
 57147379, /* 57147392 */ 
 57278461, /* 57278464 */ 
 57409529, /* 57409536 */ 
 57540599, /* 57540608 */ 
 57671671, /* 57671680 */ 
 57802739, /* 57802752 */ 
 57933817, /* 57933824 */ 
 58064861, /* 58064896 */ 
 58195939, /* 58195968 */ 
 58327039, /* 58327040 */ 
 58458091, /* 58458112 */ 
 58589161, /* 58589184 */ 
 58720253, /* 58720256 */ 
 58851307, /* 58851328 */ 
 58982389, /* 58982400 */ 
 59113469, /* 59113472 */ 
 59244539, /* 59244544 */ 
 59375587, /* 59375616 */ 
 59506679, /* 59506688 */ 
 59637733, /* 59637760 */ 
 59768831, /* 59768832 */ 
 59899901, /* 59899904 */ 
 60030953, /* 60030976 */ 
 60162029, /* 60162048 */ 
 60293119, /* 60293120 */ 
 60424183, /* 60424192 */ 
 60555227, /* 60555264 */ 
 60686321, /* 60686336 */ 
 60817397, /* 60817408 */ 
 60948479, /* 60948480 */ 
 61079531, /* 61079552 */ 
 61210603, /* 61210624 */ 
 61341659, /* 61341696 */ 
 61472753, /* 61472768 */ 
 61603811, /* 61603840 */ 
 61734899, /* 61734912 */ 
 61865971, /* 61865984 */ 
 61997053, /* 61997056 */ 
 62128127, /* 62128128 */ 
 62259193, /* 62259200 */ 
 62390261, /* 62390272 */ 
 62521331, /* 62521344 */ 
 62652407, /* 62652416 */ 
 62783477, /* 62783488 */ 
 62914549, /* 62914560 */ 
 63045613, /* 63045632 */ 
 63176693, /* 63176704 */ 
 63307763, /* 63307776 */ 
 63438839, /* 63438848 */ 
 63569917, /* 63569920 */ 
 63700991, /* 63700992 */ 
 63832057, /* 63832064 */ 
 63963131, /* 63963136 */ 
 64094207, /* 64094208 */ 
 64225267, /* 64225280 */ 
 64356349, /* 64356352 */ 
 64487417, /* 64487424 */ 
 64618493, /* 64618496 */ 
 64749563, /* 64749568 */ 
 64880587, /* 64880640 */ 
 65011703, /* 65011712 */ 
 65142769, /* 65142784 */ 
 65273851, /* 65273856 */ 
 65404909, /* 65404928 */ 
 65535989, /* 65536000 */ 
 65667067, /* 65667072 */ 
 65798137, /* 65798144 */ 
 65929211, /* 65929216 */ 
 66060277, /* 66060288 */ 
 66191351, /* 66191360 */ 
 66322427, /* 66322432 */ 
 66453479, /* 66453504 */ 
 66584561, /* 66584576 */ 
 66715643, /* 66715648 */ 
 66846709, /* 66846720 */ 
 66977767, /* 66977792 */ 
 67108859, /* 67108864 */ 
 67239883, /* 67239936 */ 
 67370999, /* 67371008 */ 
 67502063, /* 67502080 */ 
 67633127, /* 67633152 */ 
 67764223, /* 67764224 */ 
 67895251, /* 67895296 */ 
 68026363, /* 68026368 */ 
 68157433, /* 68157440 */ 
 68288503, /* 68288512 */ 
 68419567, /* 68419584 */ 
 68550631, /* 68550656 */ 
 68681719, /* 68681728 */ 
 68812769, /* 68812800 */ 
 68943851, /* 68943872 */ 
 69074933, /* 69074944 */ 
 69205987, /* 69206016 */ 
 69337087, /* 69337088 */ 
 69468151, /* 69468160 */ 
 69599221, /* 69599232 */ 
 69730303, /* 69730304 */ 
 69861331, /* 69861376 */ 
 69992443, /* 69992448 */ 
 70123513, /* 70123520 */ 
 70254563, /* 70254592 */ 
 70385641, /* 70385664 */ 
 70516729, /* 70516736 */ 
 70647793, /* 70647808 */ 
 70778861, /* 70778880 */ 
 70909933, /* 70909952 */ 
 71041021, /* 71041024 */ 
 71172091, /* 71172096 */ 
 71303153, /* 71303168 */ 
 71434229, /* 71434240 */ 
 71565283, /* 71565312 */ 
 71696363, /* 71696384 */ 
 71827423, /* 71827456 */ 
 71958521, /* 71958528 */ 
 72089573, /* 72089600 */ 
 72220663, /* 72220672 */ 
 72351733, /* 72351744 */ 
 72482807, /* 72482816 */ 
 72613861, /* 72613888 */ 
 72744937, /* 72744960 */ 
 72876031, /* 72876032 */ 
 73007089, /* 73007104 */ 
 73138171, /* 73138176 */ 
 73269247, /* 73269248 */ 
 73400311, /* 73400320 */ 
 73531379, /* 73531392 */ 
 73662461, /* 73662464 */ 
 73793521, /* 73793536 */ 
 73924583, /* 73924608 */ 
 74055637, /* 74055680 */ 
 74186747, /* 74186752 */ 
 74317801, /* 74317824 */ 
 74448877, /* 74448896 */ 
 74579951, /* 74579968 */ 
 74711027, /* 74711040 */ 
 74842099, /* 74842112 */ 
 74973181, /* 74973184 */ 
 75104243, /* 75104256 */ 
 75235327, /* 75235328 */ 
 75366397, /* 75366400 */ 
 75497467, /* 75497472 */ 
 75628513, /* 75628544 */ 
 75759613, /* 75759616 */ 
 75890653, /* 75890688 */ 
 76021661, /* 76021760 */ 
 76152821, /* 76152832 */ 
 76283897, /* 76283904 */ 
 76414973, /* 76414976 */ 
 76546039, /* 76546048 */ 
 76677113, /* 76677120 */ 
 76808119, /* 76808192 */ 
 76939253, /* 76939264 */ 
 77070317, /* 77070336 */ 
 77201347, /* 77201408 */ 
 77332471, /* 77332480 */ 
 77463541, /* 77463552 */ 
 77594599, /* 77594624 */ 
 77725691, /* 77725696 */ 
 77856767, /* 77856768 */ 
 77987821, /* 77987840 */ 
 78118903, /* 78118912 */ 
 78249973, /* 78249984 */ 
 78381047, /* 78381056 */ 
 78512101, /* 78512128 */ 
 78643199, /* 78643200 */ 
 78774259, /* 78774272 */ 
 78905303, /* 78905344 */ 
 79036411, /* 79036416 */ 
 79167479, /* 79167488 */ 
 79298543, /* 79298560 */ 
 79429619, /* 79429632 */ 
 79560673, /* 79560704 */ 
 79691761, /* 79691776 */ 
 79822829, /* 79822848 */ 
 79953901, /* 79953920 */ 
 80084969, /* 80084992 */ 
 80216063, /* 80216064 */ 
 80347103, /* 80347136 */ 
 80478199, /* 80478208 */ 
 80609279, /* 80609280 */ 
 80740339, /* 80740352 */ 
 80871419, /* 80871424 */ 
 81002489, /* 81002496 */ 
 81133567, /* 81133568 */ 
 81264587, /* 81264640 */ 
 81395683, /* 81395712 */ 
 81526763, /* 81526784 */ 
 81657841, /* 81657856 */ 
 81788923, /* 81788928 */ 
 81919993, /* 81920000 */ 
 82051043, /* 82051072 */ 
 82182137, /* 82182144 */ 
 82313213, /* 82313216 */ 
 82444279, /* 82444288 */ 
 82575331, /* 82575360 */ 
 82706431, /* 82706432 */ 
 82837501, /* 82837504 */ 
 82968563, /* 82968576 */ 
 83099641, /* 83099648 */ 
 83230717, /* 83230720 */ 
 83361781, /* 83361792 */ 
 83492863, /* 83492864 */ 
 83623931, /* 83623936 */ 
 83754997, /* 83755008 */ 
 83886053, /* 83886080 */ 
 84017117, /* 84017152 */ 
 84148213, /* 84148224 */ 
 84279277, /* 84279296 */ 
 84410353, /* 84410368 */ 
 84541421, /* 84541440 */ 
 84672487, /* 84672512 */ 
 84803581, /* 84803584 */ 
 84934621, /* 84934656 */ 
 85065719, /* 85065728 */ 
 85196789, /* 85196800 */ 
 85327849, /* 85327872 */ 
 85458929, /* 85458944 */ 
 85589989, /* 85590016 */ 
 85721081, /* 85721088 */ 
 85852147, /* 85852160 */ 
 85983217, /* 85983232 */ 
 86114279, /* 86114304 */ 
 86245343, /* 86245376 */ 
 86376443, /* 86376448 */ 
 86507507, /* 86507520 */ 
 86638577, /* 86638592 */ 
 86769647, /* 86769664 */ 
 86900731, /* 86900736 */ 
 87031759, /* 87031808 */ 
 87162857, /* 87162880 */ 
 87293939, /* 87293952 */ 
 87425021, /* 87425024 */ 
 87556087, /* 87556096 */ 
 87687167, /* 87687168 */ 
 87818239, /* 87818240 */ 
 87949307, /* 87949312 */ 
 88080359, /* 88080384 */ 
 88211449, /* 88211456 */ 
 88342519, /* 88342528 */ 
 88473569, /* 88473600 */ 
 88604653, /* 88604672 */ 
 88735721, /* 88735744 */ 
 88866797, /* 88866816 */ 
 88997827, /* 88997888 */ 
 89128939, /* 89128960 */ 
 89260027, /* 89260032 */ 
 89391103, /* 89391104 */ 
 89522171, /* 89522176 */ 
 89653217, /* 89653248 */ 
 89784313, /* 89784320 */ 
 89915383, /* 89915392 */ 
 90046441, /* 90046464 */ 
 90177533, /* 90177536 */ 
 90308599, /* 90308608 */ 
 90439667, /* 90439680 */ 
 90570751, /* 90570752 */ 
 90701797, /* 90701824 */ 
 90832871, /* 90832896 */ 
 90963967, /* 90963968 */ 
 91095013, /* 91095040 */ 
 91226101, /* 91226112 */ 
 91357177, /* 91357184 */ 
 91488251, /* 91488256 */ 
 91619321, /* 91619328 */ 
 91750391, /* 91750400 */ 
 91881443, /* 91881472 */ 
 92012537, /* 92012544 */ 
 92143609, /* 92143616 */ 
 92274671, /* 92274688 */ 
 92405723, /* 92405760 */ 
 92536823, /* 92536832 */ 
 92667863, /* 92667904 */ 
 92798969, /* 92798976 */ 
 92930039, /* 92930048 */ 
 93061117, /* 93061120 */ 
 93192191, /* 93192192 */ 
 93323249, /* 93323264 */ 
 93454307, /* 93454336 */ 
 93585379, /* 93585408 */ 
 93716471, /* 93716480 */ 
 93847549, /* 93847552 */ 
 93978559, /* 93978624 */ 
 94109681, /* 94109696 */ 
 94240733, /* 94240768 */ 
 94371833, /* 94371840 */ 
 94502899, /* 94502912 */ 
 94633963, /* 94633984 */ 
 94765039, /* 94765056 */ 
 94896119, /* 94896128 */ 
 95027197, /* 95027200 */ 
 95158249, /* 95158272 */ 
 95289329, /* 95289344 */ 
 95420401, /* 95420416 */ 
 95551487, /* 95551488 */ 
 95682541, /* 95682560 */ 
 95813621, /* 95813632 */ 
 95944691, /* 95944704 */ 
 96075739, /* 96075776 */ 
 96206839, /* 96206848 */ 
 96337919, /* 96337920 */ 
 96468979, /* 96468992 */ 
 96600041, /* 96600064 */ 
 96731101, /* 96731136 */ 
 96862169, /* 96862208 */ 
 96993269, /* 96993280 */ 
 97124347, /* 97124352 */ 
 97255409, /* 97255424 */ 
 97386467, /* 97386496 */ 
 97517543, /* 97517568 */ 
 97648637, /* 97648640 */ 
 97779701, /* 97779712 */ 
 97910759, /* 97910784 */ 
 98041831, /* 98041856 */ 
 98172887, /* 98172928 */ 
 98303999, /* 98304000 */ 
 98435063, /* 98435072 */ 
 98566121, /* 98566144 */ 
 98697187, /* 98697216 */ 
 98828281, /* 98828288 */ 
 98959337, /* 98959360 */ 
 99090427, /* 99090432 */ 
 99221489, /* 99221504 */ 
 99352567, /* 99352576 */ 
 99483647, /* 99483648 */ 
 99614689, /* 99614720 */ 
 99745787, /* 99745792 */ 
 99876851, /* 99876864 */ 
 100007927, /* 100007936 */ 
 100138979, /* 100139008 */ 
 100270069, /* 100270080 */ 
 100401139, /* 100401152 */ 
 100532207, /* 100532224 */ 
 100663291, /* 100663296 */ 
 100794319, /* 100794368 */ 
 100925431, /* 100925440 */ 
 101056507, /* 101056512 */ 
 101187577, /* 101187584 */ 
 101318647, /* 101318656 */ 
 101449717, /* 101449728 */ 
 101580793, /* 101580800 */ 
 101711839, /* 101711872 */ 
 101842931, /* 101842944 */ 
 101974009, /* 101974016 */ 
 102105049, /* 102105088 */ 
 102236149, /* 102236160 */ 
 102367189, /* 102367232 */ 
 102498301, /* 102498304 */ 
 102629369, /* 102629376 */ 
 102760387, /* 102760448 */ 
 102891499, /* 102891520 */ 
 103022537, /* 103022592 */ 
 103153649, /* 103153664 */ 
 103284733, /* 103284736 */ 
 103415791, /* 103415808 */ 
 103546879, /* 103546880 */ 
 103677949, /* 103677952 */ 
 103809011, /* 103809024 */ 
 103940093, /* 103940096 */ 
 104071157, /* 104071168 */ 
 104202233, /* 104202240 */ 
 104333311, /* 104333312 */ 
 104464369, /* 104464384 */ 
 104595397, /* 104595456 */ 
 104726527, /* 104726528 */ 
 104857589, /* 104857600 */ 
 104988641, /* 104988672 */ 
 105119741, /* 105119744 */ 
 105250811, /* 105250816 */ 
 105381841, /* 105381888 */ 
 105512951, /* 105512960 */ 
 105644029, /* 105644032 */ 
 105775079, /* 105775104 */ 
 105906167, /* 105906176 */ 
 106037237, /* 106037248 */ 
 106168319, /* 106168320 */ 
 106299379, /* 106299392 */ 
 106430449, /* 106430464 */ 
 106561523, /* 106561536 */ 
 106692601, /* 106692608 */ 
 106823677, /* 106823680 */ 
 106954747, /* 106954752 */ 
 107085799, /* 107085824 */ 
 107216891, /* 107216896 */ 
 107347943, /* 107347968 */ 
 107479033, /* 107479040 */ 
 107610079, /* 107610112 */ 
 107741167, /* 107741184 */ 
 107872249, /* 107872256 */ 
 108003323, /* 108003328 */ 
 108134393, /* 108134400 */ 
 108265459, /* 108265472 */ 
 108396521, /* 108396544 */ 
 108527603, /* 108527616 */ 
 108658681, /* 108658688 */ 
 108789727, /* 108789760 */ 
 108920831, /* 108920832 */ 
 109051903, /* 109051904 */ 
 109182947, /* 109182976 */ 
 109314043, /* 109314048 */ 
 109445107, /* 109445120 */ 
 109576189, /* 109576192 */ 
 109707253, /* 109707264 */ 
 109838293, /* 109838336 */ 
 109969403, /* 109969408 */ 
 110100409, /* 110100480 */ 
 110231531, /* 110231552 */ 
 110362559, /* 110362624 */ 
 110493661, /* 110493696 */ 
 110624753, /* 110624768 */ 
 110755793, /* 110755840 */ 
 110886883, /* 110886912 */ 
 111017983, /* 111017984 */ 
 111148963, /* 111149056 */ 
 111280121, /* 111280128 */ 
 111411173, /* 111411200 */ 
 111542261, /* 111542272 */ 
 111673343, /* 111673344 */ 
 111804389, /* 111804416 */ 
 111935459, /* 111935488 */ 
 112066553, /* 112066560 */ 
 112197629, /* 112197632 */ 
 112328683, /* 112328704 */ 
 112459751, /* 112459776 */ 
 112590839, /* 112590848 */ 
 112721893, /* 112721920 */ 
 112852981, /* 112852992 */ 
 112984061, /* 112984064 */ 
 113115133, /* 113115136 */ 
 113246183, /* 113246208 */ 
 113377279, /* 113377280 */ 
 113508319, /* 113508352 */ 
 113639419, /* 113639424 */ 
 113770457, /* 113770496 */ 
 113901553, /* 113901568 */ 
 114032599, /* 114032640 */ 
 114163703, /* 114163712 */ 
 114294721, /* 114294784 */ 
 114425807, /* 114425856 */ 
 114556913, /* 114556928 */ 
 114687977, /* 114688000 */ 
 114819031, /* 114819072 */ 
 114950131, /* 114950144 */ 
 115081189, /* 115081216 */ 
 115212287, /* 115212288 */ 
 115343341, /* 115343360 */ 
 115474417, /* 115474432 */ 
 115605467, /* 115605504 */ 
 115736539, /* 115736576 */ 
 115867627, /* 115867648 */ 
 115998719, /* 115998720 */ 
 116129789, /* 116129792 */ 
 116260849, /* 116260864 */ 
 116391917, /* 116391936 */ 
 116523007, /* 116523008 */ 
 116654077, /* 116654080 */ 
 116785133, /* 116785152 */ 
 116916223, /* 116916224 */ 
 117047291, /* 117047296 */ 
 117178367, /* 117178368 */ 
 117309421, /* 117309440 */ 
 117440509, /* 117440512 */ 
 117571523, /* 117571584 */ 
 117702649, /* 117702656 */ 
 117833711, /* 117833728 */ 
 117964793, /* 117964800 */ 
 118095853, /* 118095872 */ 
 118226893, /* 118226944 */ 
 118358003, /* 118358016 */ 
 118489081, /* 118489088 */ 
 118620143, /* 118620160 */ 
 118751207, /* 118751232 */ 
 118882279, /* 118882304 */ 
 119013347, /* 119013376 */ 
 119144447, /* 119144448 */ 
 119275511, /* 119275520 */ 
 119406587, /* 119406592 */ 
 119537653, /* 119537664 */ 
 119668723, /* 119668736 */ 
 119799803, /* 119799808 */ 
 119930873, /* 119930880 */ 
 120061951, /* 120061952 */ 
 120193019, /* 120193024 */ 
 120324077, /* 120324096 */ 
 120455147, /* 120455168 */ 
 120586231, /* 120586240 */ 
 120717307, /* 120717312 */ 
 120848353, /* 120848384 */ 
 120979447, /* 120979456 */ 
 121110523, /* 121110528 */ 
 121241597, /* 121241600 */ 
 121372649, /* 121372672 */ 
 121503737, /* 121503744 */ 
 121634801, /* 121634816 */ 
 121765871, /* 121765888 */ 
 121896949, /* 121896960 */ 
 122028019, /* 122028032 */ 
 122159101, /* 122159104 */ 
 122290171, /* 122290176 */ 
 122421241, /* 122421248 */ 
 122552317, /* 122552320 */ 
 122683391, /* 122683392 */ 
 122814463, /* 122814464 */ 
 122945527, /* 122945536 */ 
 123076601, /* 123076608 */ 
 123207677, /* 123207680 */ 
 123338737, /* 123338752 */ 
 123469783, /* 123469824 */ 
 123600857, /* 123600896 */ 
 123731963, /* 123731968 */ 
 123863023, /* 123863040 */ 
 123994099, /* 123994112 */ 
 124125161, /* 124125184 */ 
 124256243, /* 124256256 */ 
 124387321, /* 124387328 */ 
 124518397, /* 124518400 */ 
 124649449, /* 124649472 */ 
 124780531, /* 124780544 */ 
 124911601, /* 124911616 */ 
 125042663, /* 125042688 */ 
 125173759, /* 125173760 */ 
 125304787, /* 125304832 */ 
 125435897, /* 125435904 */ 
 125566963, /* 125566976 */ 
 125698021, /* 125698048 */ 
 125829103, /* 125829120 */ 
 125960189, /* 125960192 */ 
 126091241, /* 126091264 */ 
 126222293, /* 126222336 */ 
 126353407, /* 126353408 */ 
 126484469, /* 126484480 */ 
 126615551, /* 126615552 */ 
 126746623, /* 126746624 */ 
 126877693, /* 126877696 */ 
 127008733, /* 127008768 */ 
 127139833, /* 127139840 */ 
 127270849, /* 127270912 */ 
 127401947, /* 127401984 */ 
 127533047, /* 127533056 */ 
 127664113, /* 127664128 */ 
 127795181, /* 127795200 */ 
 127926263, /* 127926272 */ 
 128057327, /* 128057344 */ 
 128188409, /* 128188416 */ 
 128319469, /* 128319488 */ 
 128450533, /* 128450560 */ 
 128581631, /* 128581632 */ 
 128712691, /* 128712704 */ 
 128843761, /* 128843776 */ 
 128974841, /* 128974848 */ 
 129105901, /* 129105920 */ 
 129236959, /* 129236992 */ 
 129368051, /* 129368064 */ 
 129499129, /* 129499136 */ 
 129630199, /* 129630208 */ 
 129761273, /* 129761280 */ 
 129892333, /* 129892352 */ 
 130023407, /* 130023424 */ 
 130154483, /* 130154496 */ 
 130285567, /* 130285568 */ 
 130416631, /* 130416640 */ 
 130547621, /* 130547712 */ 
 130678781, /* 130678784 */ 
 130809853, /* 130809856 */ 
 130940911, /* 130940928 */ 
 131071987, /* 131072000 */ 
 131203069, /* 131203072 */ 
 131334131, /* 131334144 */ 
 131465177, /* 131465216 */ 
 131596279, /* 131596288 */ 
 131727359, /* 131727360 */ 
 131858413, /* 131858432 */ 
 131989477, /* 131989504 */ 
 132120557, /* 132120576 */ 
 132251621, /* 132251648 */ 
 132382717, /* 132382720 */ 
 132513781, /* 132513792 */ 
 132644851, /* 132644864 */ 
 132775931, /* 132775936 */ 
 132907007, /* 132907008 */ 
 133038053, /* 133038080 */ 
 133169137, /* 133169152 */ 
 133300207, /* 133300224 */ 
 133431293, /* 133431296 */ 
 133562329, /* 133562368 */ 
 133693433, /* 133693440 */ 
 133824503, /* 133824512 */ 
 133955581, /* 133955584 */ 
 134086639, /* 134086656 */ 
};

int sf_nearest_prime( int n )
{
    if( n < 0 )
        n = -n;

    if( n < 8192 )
    {
        return prime_table0[(n>>3)& 1023];
    }
    else if( n < 64*1024 )
    {
        return prime_table1[(n>>6)&1023];
    }
    else if( n < 1024*1024 )
    {
        return prime_table2[(n>>10)&1023];
    }
    else if( n < 128*1024*1024 )
    {
        return prime_table3[(n>>17)&1023];
    }
    else if( n < 1024*1024*1024 )
    {
        return prime_table3[(n>>20)&1023];
    }
  
    return 134086639; /* too big for table, just use a big prime */ 
}

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
  sfmemcap.c

  These functions wrap the alloc & free functions. They enforce a memory cap using
  the MEMCAP structure.  The MEMCAP structure tracks memory usage.  Each allocation
  has 4 bytes added to it so we can store the allocation size.  This allows us to 
  free a block and accurately track how much memory was recovered.
  
  Marc Norton  
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

/*
*   Set max # bytes & init other variables.
*/
void sfmemcap_init( MEMCAP * mc, unsigned nbytes )
{
	mc->memcap = nbytes;
	mc->memused= 0;
	mc->nblocks= 0;
}

/*
*   Create and Init a MEMCAP -  use free to release it
*/
MEMCAP * sfmemcap_new( unsigned nbytes )
{
	 MEMCAP * mc;

	 mc = (MEMCAP*)calloc(1,sizeof(MEMCAP));

         if( mc ) sfmemcap_init( mc, nbytes );
	 
	 return mc;
}

/*
*  Release the memcap structure
*/
void sfmemcap_delete( MEMCAP * p )
{
     if(p)free( p );
}

/*
*  Allocate some memory
*/
void * sfmemcap_alloc( MEMCAP * mc, unsigned nbytes )
{
   long * data;

   //printf("sfmemcap_alloc: %d bytes requested, memcap=%d, used=%d\n",nbytes,mc->memcap,mc->memused);

   nbytes += sizeof(long);


   /* Check if we are limiting memory use */
   if( mc->memcap > 0 )
   {
      /* Check if we've maxed out our memory - if we are tracking memory */
      if( (mc->memused + nbytes) > mc->memcap )
      {
	      return 0;
      }
   }

   //data = (long *) malloc( nbytes );
   data = (long *)SnortAlloc( nbytes );

   if( data == NULL )
   {
        return 0;
   }

   *data++ = (long)nbytes;

   mc->memused += nbytes;
   mc->nblocks++;

   return data;
}

/*
*   Free some memory
*/
void sfmemcap_free( MEMCAP * mc, void * p )
{
   long * q;

   q = (long*)p;
   q--;
   mc->memused -= (unsigned)(*q);
   mc->nblocks--;

   free(q);
}

/*
*   For debugging.
*/
void sfmemcap_showmem( MEMCAP * mc )
{
     fprintf(stderr, "memcap: memcap = %u bytes,",mc->memcap);
     fprintf(stderr, " memused= %u bytes,",mc->memused);
     fprintf(stderr, " nblocks= %d blocks\n",mc->nblocks);
}

/*
*  String Dup Some memory.
*/
char * sfmemcap_strdup( MEMCAP * mc, const char *str )
{
    char *data = NULL;
    int data_size;

    data_size = strlen(str) + 1;
    data = (char *)sfmemcap_alloc(mc, data_size);

    if(data == NULL)
    {
        return  0 ;
    }

    SnortStrncpy(data, str, data_size);

    return data;
}

/*
*  Dup Some memory.
*/
void * sfmemcap_dupmem( MEMCAP * mc, void * src, int n )
{
    void * data = (char *)sfmemcap_alloc( mc, n );
    if(data == NULL)
    {
        return  0;
    }

    memcpy( data, src, n );

    return data;
}
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
*   sflsq.c    
*
*   Simple list, stack, queue, and dictionary implementations 
*   ( most of these implementations are list based - not performance monsters,
*     and they all use alloc via s_alloc/s_free )
*   Stack based Ineteger and Pointer Stacks, these are for performance.(inline would be better)
*
*   11/05/2005 - man - Added sflist_firstx() and sflist_nextx() with user
*   provided SF_NODE inputs for tracking the list position.  This allows
*   multiple readers to traverse a list. The built in 'cur' field does not 
*   wrok for multiple readers.
*
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
*  private alloc
*/ 
static void * sf_alloc (int n) 
{
  void *p=0;
  if( n > 0 )p = (void*) calloc( 1,n );
  return p;
}

/*
*  private free
*/ 

/*
*   INIT - called by the NEW functions
*/ 
void sflist_init ( SF_LIST * s) 
{
  s->count=0; 
  s->head = s->tail = s->cur = 0;
}

/*
*    NEW
*/
SF_LIST * sflist_new(void) 
{
   SF_LIST * s;
   s = (SF_LIST*)sf_alloc( sizeof(SF_LIST) );
   if( s )sflist_init( s );
   return s;
}

SF_STACK * sfstack_new(void) 
{
   return (SF_STACK*)sflist_new();
}

SF_QUEUE * sfqueue_new(void) 
{
   return (SF_QUEUE*)sflist_new();
}
/*
*  Add-before Item 
*/ 
int sflist_add_before ( SF_LIST* s, SF_LNODE * lnode, NODE_DATA ndata )
{
  SF_LNODE * q;

  if( !lnode )
      return 0;

  /* Add to head of list */
  if( s->head == lnode )
  {
      return sflist_add_head ( s, ndata );
  }
  else
  {
      q = (SF_LNODE *) sf_alloc ( sizeof (SF_LNODE) );
      if( !q )
      {
          return -1;
      }
      q->ndata = (NODE_DATA)ndata;

      q->next = lnode;
      q->prev = lnode->prev;
      lnode->prev->next = q;
      lnode->prev       = q;
  }
  s->count++;

  return 0;
}

/*
*     ADD to List/Stack/Queue/Dictionary
*/
/*
*  Add-Head Item 
*/ 
int 
sflist_add_head ( SF_LIST* s, NODE_DATA ndata )
{
  SF_LNODE * q;
  if (!s->head)
    {
      q = s->tail = s->head = (SF_LNODE *) sf_alloc (sizeof (SF_LNODE));
      if(!q)return -1;
      q->ndata = (NODE_DATA)ndata;
      q->next = 0;
      q->prev = 0;
    }
  else
    {
      q = (SF_LNODE *) sf_alloc (sizeof (SF_LNODE));
      if(!q)return -1;
      q->ndata = ndata;
      q->next = s->head;
      q->prev = 0;
      s->head->prev = q;
      s->head = q;

    }
  s->count++;

  return 0;
}

/*
*  Add-Tail Item 
*/ 
int 
sflist_add_tail ( SF_LIST* s, NODE_DATA ndata )
{
  SF_LNODE * q;
  if (!s->head)
    {
      q = s->tail = s->head = (SF_LNODE *) sf_alloc (sizeof (SF_LNODE));
      if(!q)return -1;
      q->ndata = (NODE_DATA)ndata;
      q->next = 0;
      q->prev = 0;
    }
  else
    {
      q = (SF_LNODE *) sf_alloc (sizeof (SF_LNODE));
      if(!q)return -1;
      q->ndata = ndata;
      q->next = 0;
      q->prev = s->tail;
      s->tail->next = q;
      s->tail = q;
    }
  s->count++;

  return 0;
}

int sfqueue_add(SF_QUEUE * s, NODE_DATA ndata ) 
{
  return sflist_add_tail ( s, ndata );
}

int sfstack_add( SF_STACK* s, NODE_DATA ndata ) 
{
  return sflist_add_tail ( s, ndata );
}

/* 
*   List walk - First/Next - return the node data or NULL
*/
NODE_DATA sflist_first( SF_LIST * s )
{
    if(!s) 
        return 0;

    s->cur = s->head;
    if( s->cur ) 
        return s->cur->ndata;
    return 0;
}
NODE_DATA sflist_next( SF_LIST * s )
{
    if(!s)
        return 0;

    if( s->cur )
    {
        s->cur = s->cur->next;
        if( s->cur ) 
            return s->cur->ndata;
    }
    return 0;
}
NODE_DATA sflist_firstpos( SF_LIST * s, SF_LNODE ** v )
{
    if(!s)
        return 0;
    
    *v = s->head;
    
    if( *v )
        return (*v)->ndata;

    return 0;
}
NODE_DATA sflist_nextpos( SF_LIST * s,  SF_LNODE ** v )
{
    if(!s)
        return 0;
    
    if(v)
    {
       if(*v)
       {
          *v = (*v)->next;
          if( *v ) 
              return (*v)->ndata;
       }
    }
    return 0;
}
/* 
*   List walk - First/Next - return the node data or NULL
*/
SF_LNODE * sflist_first_node( SF_LIST * s )
{
    if(!s)
        return 0;

    s->cur = s->head;
    if( s->cur ) 
        return s->cur;
    return 0;
}
SF_LNODE * sflist_next_node( SF_LIST * s )
{
    if(!s)
        return 0;
    if( s->cur )
    {
        s->cur = s->cur->next;
        if( s->cur ) 
            return s->cur;
    }
    return 0;
}

/*
*  Remove Head Item from list
*/ 
NODE_DATA sflist_remove_head (SF_LIST * s) 
{
  NODE_DATA ndata = 0;
  SF_QNODE * q;
  if ( s && s->head  )
    {
      q = s->head;
      ndata = q->ndata;
      s->head = s->head->next;
      s->count--;
      if( !s->head  )
	  {
	    s->tail = 0;
	    s->count = 0;
	  }
      sf_free( q );
    }
  return (NODE_DATA)ndata;
}

/*
*  Remove tail Item from list
*/
NODE_DATA sflist_remove_tail (SF_LIST * s)
{
  NODE_DATA ndata = 0;
  SF_QNODE * q;
  if (s && s->tail)
    {
      q = s->tail;

      ndata = q->ndata;
      s->count--;
      s->tail = q->prev;
      if (!s->tail)
      {
	s->tail = 0;
        s->head = 0;
	s->count = 0;
      }
      else
      {
        if( q->prev ) q->prev->next = 0;
      }
      sf_free (q);
    }
  return (NODE_DATA)ndata;
}

void sflist_remove_node (SF_LIST * s, SF_LNODE * n, void(*nfree)(void*) ) 
{
 // NODE_DATA ndata = 0;
  SF_LNODE * cur;
      
  if( n == s->head )
  {
        s->head = s->head->next;
        s->count--;
        if (!s->head)
	    {
	      s->tail = 0;
	      s->count = 0;
	    }
        if( nfree ) nfree( n->ndata );
        sf_free( n );
        return ;
  }
  else if( n == s->tail )
  {
        s->tail = s->tail->prev;
        s->count--;
        if (!s->tail )
	    {
	      s->head = 0;
	      s->count = 0;
	    }
        if( nfree ) nfree( n->ndata );
        sf_free( n );
        return ;
  }

  for(cur = s->head;
      cur!= NULL;
      cur = cur->next )
  {
    if( n == cur )
    {
       /* unlink a middle node */
       n->next->prev = n->prev;
       n->prev->next = n->next;
	   s->count--;
       if( nfree ) nfree( n->ndata );
       sf_free(n);
       return ;
     }
  }
}

/*
*  Remove Head Item from queue
*/ 
NODE_DATA sfqueue_remove (SF_QUEUE * s) 
{
  return (NODE_DATA)sflist_remove_head( s );
}

/*
*  Remove Tail Item from stack
*/ 
NODE_DATA sfstack_remove (SF_QUEUE * s) 
{
  return (NODE_DATA)sflist_remove_tail( s );
}

/*
*  COUNT
*/ 
int sfqueue_count (SF_QUEUE * s) 
{
  if(!s)return 0;
  return s->count;
}
int sflist_count ( SF_LIST* s) 
{
  if(!s)return 0;
  return s->count;
}
int sfstack_count ( SF_STACK * s) 
{
  if(!s)return 0;
  return s->count;
}


/*
*   Free List + Free it's data nodes using 'nfree' 
*/
void sflist_free_all( SF_LIST * s, void (*nfree)(void*) ) 
{
  void * p;
  
  if(!s)
      return;
  
  while( s->count > 0 )
  {
     p = sflist_remove_head (s);
    
	 if( p && nfree ) 
         nfree( p );
  }
  sf_free(s);
}
void sfqueue_free_all(SF_QUEUE * s,void (*nfree)(void*) ) 
{
  sflist_free_all( s, nfree ); 
}
void sfstack_free_all(SF_STACK * s,void (*nfree)(void*) ) 
{
  sflist_free_all( s, nfree ); 
}
/*
*  FREE List/Queue/Stack/Dictionary
*
*  This does not free a nodes data
*/ 
void sflist_free (SF_LIST * s)
{
  while( sflist_count(s) )
  {
    sflist_remove_head(s);
  }
  sf_free(s);
}
void sfqueue_free (SF_QUEUE * s) 
{
  sflist_free ( s ); 
}
void sfstack_free (SF_STACK * s)
{
  sflist_free ( s ); 
}

/*
*   Integer stack functions - for performance scenarios
*/
int sfistack_init( SF_ISTACK * s, unsigned * a,  int n  )
{
   s->imalloc=0;
   if( a ) s->stack = a;
   else
   {
      s->stack = (unsigned*) calloc( n, sizeof(unsigned) );
      s->imalloc=1;
   }
   if( !s->stack ) return -1;
   s->nstack= n;
   s->n =0;
   return 0;
}
int sfistack_push( SF_ISTACK *s, unsigned value)
{
   if( s->n < s->nstack )
   {
       s->stack[s->n++] = value;
       return 0;
   }
   return -1;
}
int sfistack_pop( SF_ISTACK *s, unsigned * value)
{
   if( s->n > 0 )
   {
       s->n--;
       *value = s->stack[s->n];
       return 0;
   }
   return -1;
}
/*
*  Pointer Stack Functions - for performance scenarios
*/
int sfpstack_init( SF_PSTACK * s, void ** a,  int n  )
{
   s->imalloc=0;
   if( a ) s->stack = a;
   else
   {
      s->stack = (void**) calloc( n , sizeof(void*) );
      s->imalloc=1;
   }

   if( !s->stack ) return -1;
   s->nstack= n;
   s->n =0;
   return 0;
}
int sfpstack_push( SF_PSTACK *s, void * value)
{
   if( s->n < s->nstack )
   {
       s->stack[s->n++] = value;
       return 0;
   }
   return -1;
}
int sfpstack_pop( SF_PSTACK *s, void ** value)
{
   if( s->n > 0 )
   {
       s->n--;
       *value = s->stack[s->n];
       return 0;
   }
   return -1;
}
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
*  sfghash.c
*
*  Generic hash table library.
*
*  This hash table maps unique keys to void data pointers.
*
*  Features:
*    1) Keys may be ascii strings of variable size, or
*       fixed length (per table) binary byte sequences.  This
*       allows use as a Mapping for String+Data pairs, or a 
*       generic hashing.
*    2) User can allocate keys, or pass copies and we can 
*       allocate space and save keys.
*    3) User can pass a free function to free up user data
*       when the table is deleted.
*    4) Table rows sizes can be automatically adjusted to
*       the nearest prime number size.
*
*  6/10/03 - man - Upgraded the hash function to a Hardened hash function,
*      it has no predictable cycles, and each hash table gets a different
*      randomized hashing function. So even with the source code, you cannot predict 
*      anything with this function.  If an  attacker can can setup a feedback
*      loop he might gain some knowledge of how to muck with us, but even in that case
*      his odds are astronomically skinny.  This is actually the same problem as solved
*      early on with hashing functions where degenerate data with close keys could
*      produce very long bucket chains.
*
*  8/31/06 - man - Added prime tables to speed up prime number lookup.
* 
* Author: Marc Norton
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/*
*  Private Malloc
*/

/*
*  Private Free
*/
static 
void sf_free( void * p )
{
   if( p )free( p );
}

/*
*
*    Create a new hash table
*
*    nrows    : number of rows in hash table, primes are best.
*               > 0  => we use the nearest prime internally
*               < 0  => we use the magnitude as nrows.
*    keysize  : > 0 => bytes in each key, keys are binary bytes,
*               all keys are the same size.
*               ==0 => keys are strings and are null terminated, 
*               allowing random key lengths. 
*    userkeys : > 0 => indicates user owns the key data
*               and we should not allocate or free space for it,
*               nor should we attempt to free the user key. We just
*               save the pointer to the key. 
*               ==0 => we should copy the keys and manage them internally
*    userfree : routine to free users data, null if we should not 
*               free user data in sfghash_delete(). The routine
*               should be of the form 'void userfree(void * userdata)',
*               'free' works for simple allocations.
*/
SFGHASH * sfghash_new( int nrows, int keysize, int userkeys, void (*userfree)(void*p) )
{
   int    i;
   SFGHASH * h;

   if( nrows > 0 ) /* make sure we have a prime number */
   {
      nrows = sf_nearest_prime( nrows );
   }
   else   /* use the magnitude or nrows as is */
   { 
      nrows = -nrows;
   }

   h = (SFGHASH*)sf_alloc( sizeof(SFGHASH) );
   if( !h ) 
	   return 0;

   memset( h, 0, sizeof(SFGHASH) );

   h->sfhashfcn = sfhashfcn_new( nrows );
   if( !h->sfhashfcn ) 
   {
       free(h);
	   return 0;
   }

   h->table = (SFGHASH_NODE**) sf_alloc( sizeof(SFGHASH_NODE*) * nrows );
   if( !h->table ) 
   {
       free(h->sfhashfcn);
       free(h);
	   return 0;
   }

   for( i=0; i<nrows; i++ )
   {
      h->table[i] = 0;
   }

   h->userkey = userkeys;

   h->keysize = keysize;

   h->nrows = nrows;

   h->count = 0;

   h->userfree = userfree;

   h->crow = 0; // findfirst/next current row

   h->cnode = 0; // findfirst/next current node ptr

   return h;
}

/*
*  Set Splay mode : Splays nodes to front of list on each access
*/
void sfghash_splaymode( SFGHASH * t, int n )
{
   t->splay = n;
}

/*
*  Delete the hash Table 
*
*  free key's, free node's, and free the users data, if they
*  supply a free function 
*/
void sfghash_delete( SFGHASH * h )
{
  int            i;
  SFGHASH_NODE * node, * onode;

  if( !h ) return;
 
  sfhashfcn_free( h->sfhashfcn );

  if( h->table )
  {  
    for(i=0;i<h->nrows;i++)
    {
      for( node=h->table[i]; node;  )
      {
        onode = node;
        node  = node->next;

        if( !h->userkey && onode->key ) 
            sf_free( onode->key );

        if( h->userfree && onode->data )
            h->userfree( onode->data ); /* free users data, with users function */

        sf_free( onode );
      }
    }
    sf_free( h->table );
    h->table = 0;
  }

  sf_free( h );
}

/*
*  Get the # of Nodes in HASH the table
*/
int sfghash_count( SFGHASH * t )
{
  return t->count;
}



/*
*  Add a key + data pair
*  ---------------------
*
*  key + data should both be non-zero, although data can be zero
*
*  t    - hash table
*  key  - users key data (should be unique in this table)
*         may be ascii strings or fixed size binary keys
*  data - users data pointer
*
*  returns  SF_HASH_NOMEM: alloc error
*           SF_HASH_INTABLE : key already in table (t->cnode points to the node)
*           SF_OK: added a node for this key + data pair
*
*  Notes:
*  If the key node already exists, then t->cnode points to it on return,
*  this allows you to do something with the node - like add the data to a 
*  linked list of data items held by the node, or track a counter, or whatever.
*
*/
int sfghash_add( SFGHASH * t, void * key, void * data )
{
    unsigned    hashkey;
	int         klen;
    int         index;
    SFGHASH_NODE  *hnode;

    /*
    *   Get proper Key Size
    */  
    if( t->keysize > 0  )
    {
        klen = t->keysize;
    }
    else
    {
	     /* need the null byte for strcmp() in sfghash_find() */
        klen = strlen( (char*)key ) + 1;
    }
    
    hashkey = t->sfhashfcn->hash_fcn(  t->sfhashfcn, (unsigned char*) key, klen );
    
    index = hashkey % t->nrows;

    /*
    *  Uniqueness: 
    *  Check 1st to see if the key is already in the table
    *  Just bail if it is.
    */
    for( hnode=t->table[index]; hnode; hnode=hnode->next )
    {
       if( t->keysize > 0 )
       {
          if( !t->sfhashfcn->keycmp_fcn(hnode->key,key,klen) )
          {
              t->cnode = hnode; /* save pointer to the node */
              return SFGHASH_INTABLE; /* found it */
          }
       }
       else
       {
         if( !strcmp((const char *)hnode->key,(const char*)key) )
         {
             t->cnode = hnode; /* save pointer to the node */
             return SFGHASH_INTABLE; /* found it */
         }
       }
    }

    /* 
    *  Create new node 
    */
    hnode = (SFGHASH_NODE*)sf_alloc(sizeof(SFGHASH_NODE));
    if( !hnode )
         return SFGHASH_NOMEM;
    
    /* Add the Key */
    if( t->userkey )
    {
      /* Use the Users key */
      hnode->key = key;
    }
    else
    {
      /* Create new key */
      hnode->key = sf_alloc( klen );
      if( !hnode->key )
      {
           free(hnode);
           return SFGHASH_NOMEM;
      }

      /* Copy key  */
      memcpy(hnode->key,key,klen);
    }
    
    /* Add The Node */
    if( t->table[index] ) /* add the node to the existing list */
    {
        hnode->prev = 0;  // insert node as head node
        hnode->next=t->table[index];
        hnode->data=data;
        t->table[index]->prev = hnode;
        t->table[index] = hnode;
    }
    else /* 1st node in this list */
    {
        hnode->prev=0;
        hnode->next=0;
        hnode->data=data;
        t->table[index] = hnode;
    }

    t->count++;

    return SFGHASH_OK;
}

/*
*  move a node to the front of the list
*/
static void movetofront( SFGHASH *t , int index, SFGHASH_NODE * n )
{
    if( t->table[index] != n ) // if not at front of list already...
    {
      /* Unlink the node */
      if( n->prev ) n->prev->next = n->next;
      if( n->next ) n->next->prev = n->prev;
      
      /* Link at front of list */
      n->prev=0;
      n->next=t->table[index];
      t->table[index]->prev=n;
    }
}

/*
*  Find a Node based on the key, return users data.
*/
static SFGHASH_NODE * sfghash_find_node( SFGHASH * t, void * key)
{
    unsigned    hashkey;
    int         index, klen;
    SFGHASH_NODE  *hnode;

    if( t->keysize  )
    {
	klen = t->keysize;
    }
    else
    {
	klen = strlen( (char*) key ) + 1;
    }

    hashkey = t->sfhashfcn->hash_fcn(  t->sfhashfcn, (unsigned char*) key, klen );
    
    index = hashkey % t->nrows;
   
    for( hnode=t->table[index]; hnode; hnode=hnode->next )
    {
        if( t->keysize == 0 )
        {
           if( !strcmp((char*)hnode->key,(char*)key) )
           {
               if( t->splay  > 0 )
                   movetofront(t,index,hnode);

               return hnode;
           }
        }
        else
        {
           if( !t->sfhashfcn->keycmp_fcn(hnode->key,key,t->keysize) )
           {
               if( t->splay  > 0 )
                   movetofront(t,index,hnode);

               return hnode;
           }
        }
    }

   return NULL;
}

/*
*  Find a Node based on the key, return users data.
*/
void * sfghash_find( SFGHASH * t, void * key)
{
    SFGHASH_NODE * hnode;

    hnode = sfghash_find_node( t, key );

    if( hnode ) return hnode->data;

    return NULL;
}

/*
*  Unlink and free the node
*/
static int sfghash_free_node( SFGHASH * t, unsigned index, SFGHASH_NODE * hnode )
{
    if( !t->userkey && hnode->key ) 
        sf_free( hnode->key );
    hnode->key = 0;

    if( t->userfree && hnode->data )
        t->userfree( hnode->data ); /* free users data, with users function */

    if( hnode->prev )  // not the 1st node
    {
          hnode->prev->next = hnode->next;
          if( hnode->next ) hnode->next->prev = hnode->prev;
    }
    else if( t->table[index] )  // 1st node
    {
           t->table[index] = t->table[index]->next;
           if( t->table[index] )t->table[index]->prev = 0;
    }

    sf_free( hnode );

    t->count--;

    return SFGHASH_OK;
}

/*
*  Remove a Key/Data Pair from the table - find it, unlink it, and free the memory for it.
*
*  returns : 0 - OK
*           -1 - node not found
*/
int sfghash_remove( SFGHASH * t, void * key)
{
    SFGHASH_NODE * hnode;
    int klen;
    unsigned hashkey, index;

    if( t->keysize > 0 )
    {
       klen = t->keysize;
    }
    else
    {
       klen = strlen((char*)key) + 1;
    }

    hashkey = t->sfhashfcn->hash_fcn(  t->sfhashfcn, (unsigned char*) key, klen );
    
    index = hashkey % t->nrows;

    for( hnode=t->table[index]; hnode; hnode=hnode->next )
    {
       if( t->keysize > 0 )
       {
         if( !t->sfhashfcn->keycmp_fcn(hnode->key,key,klen) )
         {
             return sfghash_free_node( t, index, hnode );
         }
       }
       else
       {
         if( !strcmp((const char *)hnode->key,(const char*)key) )
         {
             return sfghash_free_node( t, index, hnode );
         }
       }
    }

   return SFGHASH_ERR;  
}


/* Internal use only */
static void sfghash_next( SFGHASH * t )
{
    if( !t->cnode )
        return ;
 
    /* Next node in current node list */
    t->cnode = t->cnode->next;
    if( t->cnode )
    {
        return;
    }

    /* Next row */ 
    /* Get 1st node in next non-emtoy row/node list */
    for( t->crow++; t->crow < t->nrows; t->crow++ )
    {    
       t->cnode = t->table[ t->crow ];
       if( t->cnode ) 
       {
           return;
       }
    }
}
/*
*   Get First Hash Table Node
*/
SFGHASH_NODE * sfghash_findfirst( SFGHASH * t )
{
    SFGHASH_NODE * n;

    /* Start with 1st row */
    for( t->crow=0; t->crow < t->nrows; t->crow++ )
    {    
       /* Get 1st Non-Null node in row list */
       t->cnode = t->table[ t->crow ];

       if( t->cnode )
       {
         n = t->cnode;

         sfghash_next( t ); // load t->cnode with the next entry

         return n;
       }
    }
  return NULL;
}

/*
*   Get Next Hash Table Node
*/
SFGHASH_NODE * sfghash_findnext( SFGHASH * t )
{
    SFGHASH_NODE * n;

    n = t->cnode;

    if( !n ) /* Done, no more entries */
    {
        return NULL;
    }

    /*
       Preload next node into current node 
    */
    sfghash_next( t ); 

    return  n;
}


/** 
 * Make sfhashfcn use a separate set of operators for the backend.
 *
 * @param h sfhashfcn ptr
 * @param hash_fcn user specified hash function
 * @param keycmp_fcn user specified key comparisoin function
 */

int sfghash_set_keyops( SFGHASH *h ,
                        unsigned (*hash_fcn)( SFHASHFCN * p,
                                              unsigned char *d,
                                              int n),
                        int (*keycmp_fcn)( const void *s1,
                                           const void *s2,
                                           size_t n))
{
    if(h && hash_fcn && keycmp_fcn)
    {
        return sfhashfcn_set_keyops(h->sfhashfcn, hash_fcn, keycmp_fcn);
    }

    return -1;
}


/*
*
*   Test Driver for Hashing
*  
*/

#ifdef SFGHASH_MAIN 

void myfree ( void * p )
{
	printf("freeing '%s'\n",p);
	free(p);
}

/*
*       Hash test program  
*/
int main ( int argc, char ** argv )
{
   int         i;
   SFGHASH      * t;
   SFGHASH_NODE * n, *m;
   char str[256],*p;
   int  num=100;

   if( argc > 1 )
       num = atoi(argv[1]);

   sfatom_init();

   /* Create a Hash Table */
   t = sfghash_new( 1000, 0 , GH_COPYKEYS , myfree  );

   /* Add Nodes to the Hash Table */
   for(i=0;i<num;i++) 
   {
       snprintf(str, sizeof(str), "KeyWord%d",i+1);
       str[sizeof(str) - 1] = '\0';
       sfghash_add( t, str,  strupr(strdup(str)) );

       sfatom_add( str,  strupr(strdup(str)) );
   }  

   /* Find and Display Nodes in the Hash Table */
   printf("\n** FIND KEY TEST\n");

   for(i=0;i<num;i++) 
   {
      snprintf(str, sizeof(str), "KeyWord%d",i+1);
      str[sizeof(str) - 1] = '\0';

      p = (char*) sfghash_find( t, str );

      printf("Hash-key=%*s, data=%*s\n", strlen(str),str, strlen(str), p );

      p = (char*) sfatom_find( str );

      printf("Atom-key=%*s, data=%*s\n", strlen(str),str, strlen(str), p );
   }  

   /* Display All Nodes in the Hash Table */
   printf("\n** FINDFIRST / FINDNEXT TEST\n");

   for( n = sfghash_findfirst(t); n; n = sfghash_findnext(t) )
   {
      printf("hash-findfirst/next: key=%s, data=%s\n", n->key, n->data );

      // hashing code frees user data using 'myfree' above ....
      if( sfghash_remove(t,n->key) ) 
            printf("Could not remove the key node\n");
      else  
            printf("key node removed\n");
   }

   for( n = sfatom_findfirst(); n; n = sfatom_findnext() )
   {
      printf("atom-findfirst/next: key=%s, data=%s\n", n->key, n->data );

      free( n->data );  //since atom data is not freed automatically
   }

   /* Free the table and it's user data */
   printf("****sfghash_delete\n");
   sfghash_delete( t );

   printf("****sfatom_reset\n");
   sfatom_reset();

   printf("\nnormal pgm finish\n\n");

   return 0;
}



#endif


