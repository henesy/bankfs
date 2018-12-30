#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "bankfs.h"


void
readndb(char* file)
{
	Ndbtuple *t;
	Ndb *n;
	int i = 0;

	n = ndbopen(file);
	if(n == nil)
		sysfatal("Could not open %s", file);

	t = ndbparse(n);
	
	stats = readstats(t);
	
	Bank *b = nil;
	while((t = ndbparse(n)) != nil){
		if(ndbfindattr(t,t,"bankid")){
			if(b != nil)
				banks[i++] = b;
			b = malloc(sizeof(Bank));
			b->stats = readstats(t);
		}else if(ndbfindattr(t,t,"acctid")){
			
		}

	}

}

//Returns Stats object for master from bankfs.ndb
//n will be filled with ndb fd at to master stats tuple
Stats*
readstats(Ndbtuple *t)
{
	Stats *stats = malloc(sizeof(Stats));

	t = ndbfindattr(t, t, "nbanks");
	//only master has nbanks attribute
	if(t != nil)
		stats->nbanks = atoi(t->val);
	
	t = ndbfindattr(t, t, "naccts");
	if(t == nil)		
		sysfatal("Could not find naccts entry in tuple");
	stats->naccts = atoi(t->val);
	
	t = ndbfindattr(t, t, "ntrans");
	if(t == nil)
		sysfatal("Could not find ntrans in tuple");
	stats->ntrans = atoi(t->val);
	
	return stats;
}

Account*
readaccount(Ndbtuple *t)
{
	return nil;
}