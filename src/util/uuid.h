/*
** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
** Digital Equipment Corporation, Maynard, Mass.
** Copyright (c) 1998 Microsoft.
** To anyone who acknowledges that this file is provided "AS IS"
** without any express or implied warranty: permission to use, copy,
** modify, and distribute this file for any purpose is hereby
** granted without fee, provided that the above copyright notices and
** this notice appears in all source code copies, and that none of
** the names of Open Software Foundation, Inc., Hewlett-Packard
** Company, Microsoft, or Digital Equipment Corporation be used in
** advertising or publicity pertaining to distribution of the software
** without specific, written prior permission. Neither Open Software
** Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital
** Equipment Corporation makes any representations about the
** suitability of this software for any purpose.
*/

#ifndef _UUID_H_
#define _UUID_H_

#undef uuid_t
typedef struct {
    unsigned int time_low;
    unsigned short time_mid;
    unsigned short time_hi_and_version;
    unsigned char clock_seq_hi_and_reserved;
    unsigned char clock_seq_low;
    char node[6];
} uuid_t;

/* uuid_create -- generate a UUID */
int uuid_create(uuid_t * uuid);

/* uuid_create_md5_from_name -- create a version 3 (MD5) UUID using a "name" from a "name space" */
void uuid_create_md5_from_name(
    uuid_t *uuid, /* resulting UUID */
    uuid_t nsid, /* UUID of the namespace */
    void *name, /* the name from which to generate a UUID */
    int namelen /* the length of the name */
);

/* uuid_create_sha1_from_name -- create a version 5 (SHA-1) UUID using a "name" from a "name space" */
void uuid_create_sha1_from_name(
    uuid_t *uuid, /* resulting UUID */
    uuid_t nsid, /* UUID of the namespace */
    void *name, /* the name from which to generate a UUID */
    int namelen /* the length of the name */
);

/* uuid_compare -- Compare two UUID’s "lexically" and return
    -1 u1 is lexically before u2
    0 u1 is equal to u2
    1 u1 is lexically after u2
    Note that lexical ordering is not temporal ordering!
*/
int uuid_compare(uuid_t *u1, uuid_t *u2);

#endif /* _UUID_H_ */