/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* Dynamic hashing of record with different key-length */

#ifndef _hash_h
#define _hash_h
#ifdef	__cplusplus
extern "C" {
#endif

/*
  Overhead to store an element in hash
  Can be used to approximate memory consumption for a hash
 */
#define HASH_OVERHEAD (sizeof(char*)*2)

/* flags for hash_init */
#define HASH_UNIQUE     1       /* hash_insert fails on duplicate key */

typedef uchar *(*hash_get_key)(const uchar *,size_t*,my_bool);
typedef void (*hash_free_key)(void *);
typedef my_bool (*hash_walk_action)(void *,void *);

typedef struct st_hash {
  size_t key_offset,key_length;		/* Length of key if const length */
  size_t blength;
  ulong records;
  uint flags;
  DYNAMIC_ARRAY array;				/* Place for hash_keys */
  hash_get_key get_key;
  void (*free)(void *);
  CHARSET_INFO *charset;
} HASH;

/* A search iterator state */
typedef uint HASH_SEARCH_STATE;

#define hash_init(A,B,C,D,E,F,G,H) _hash_init(A,0,B,C,D,E,F,G,H CALLER_INFO)
#define hash_init2(A,B,C,D,E,F,G,H,I) _hash_init(A,B,C,D,E,F,G,H,I CALLER_INFO)
my_bool _hash_init(HASH *hash, uint growth_size,CHARSET_INFO *charset,
		   ulong default_array_elements, size_t key_offset,
		   size_t key_length, hash_get_key get_key,
		   void (*free_element)(void*), uint flags CALLER_INFO_PROTO);
void hash_free(HASH *tree);
void my_hash_reset(HASH *hash);
uchar *hash_element(HASH *hash,ulong idx);
uchar *hash_search(const HASH *info, const uchar *key, size_t length);
uchar *hash_first(const HASH *info, const uchar *key, size_t length,
                HASH_SEARCH_STATE *state);
uchar *hash_next(const HASH *info, const uchar *key, size_t length,
                 HASH_SEARCH_STATE *state);
my_bool my_hash_insert(HASH *info,const uchar *data);
my_bool hash_delete(HASH *hash,uchar *record);
my_bool hash_update(HASH *hash,uchar *record,uchar *old_key,size_t old_key_length);
void hash_replace(HASH *hash, HASH_SEARCH_STATE *state, uchar *new_row);
my_bool hash_check(HASH *hash);			/* Only in debug library */
my_bool hash_iterate(HASH *hash, hash_walk_action action, void *argument);

#define hash_clear(H) bzero((char*) (H),sizeof(*(H)))
#define hash_inited(H) ((H)->array.buffer != 0)
#define hash_init_opt(A,B,C,D,E,F,G,H) \
          (!hash_inited(A) && _hash_init(A,0,B,C,D,E,F,G, H CALLER_INFO))

#ifdef	__cplusplus
}
#endif
#endif
