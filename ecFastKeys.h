/* $Id$ */
#ifndef EC_FAST_KEY_TILL_H
#define EC_FAST_KEY_TILL_H

/* fast key implementation */
#include "drvrEcdr814.h"

#ifndef DO_GEN_FAST_KEYS
#include "fastKeyDefs.h"
#endif

#define FASTKEY_PREPEND( prevkey, key) (((key)<<FKEY_LEN) | prevkey)
#define BUILD_FKEY1(a) 		(a)
#define BUILD_FKEY2(a,b) 	FASTKEY_PREPEND( a, b)
#define BUILD_FKEY3(a,b,c)	FASTKEY_PREPEND( a, FASTKEY_PREPEND(b, c))
#define BUILD_FKEY4(a,b,c,d) 	FASTKEY_PREPEND( a, FASTKEY_PREPEND( b, FASTKEY_PREPEND(c, d)))

#define EMPTY_FKEY		((EcFKey)0)

/* prepend an fkey to a path of fkeys */
extern inline EcFKey 
ecAddFKeyToPath(EcFKey path, EcFKey fkey)
{
register EcFKey mask;
for (mask = (1<<FKEY_LEN)-1; mask & path; mask <<= FKEY_LEN, fkey<<=FKEY_LEN);
return path | fkey;
}

extern inline EcFKey
ecStripFKeyFromPath(EcFKey path)
{
register  EcFKey mask;
for (mask = (1<<FKEY_LEN)-1; mask & path; mask <<= FKEY_LEN);
return path & (mask>>FKEY_LEN);
}

/* convert a NodeList to a fastkey describing
 * the same path
 */
extern inline EcFKey
ecNodeList2FKey(EcNodeList l)
{
EcFKey rval = EMPTY_FKEY;
while (l->p) {
	rval = (rval << FKEY_LEN) | ((l->n - l->p->n->u.d.n->nodes) + 1);
	l=l->p;
}
return rval;
}

#endif
