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
	int i, j;
	i = j = 0;

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
			j = 0; //reset account count
		}else if(ndbfindattr(t,t,"acctid")){
			if(b == nil)
				sysfatal("Orphaned account tuple with no bank");
			//TODO: Realloc more efficently
			b->accounts = realloc(b->accounts, sizeof(Account*) * j+1);
			b->accounts[j++] = readaccount(t);
		}
	}
}

//Returns Stats object for master from bankfs.ndb
//n will be filled with ndb fd at to master stats tuple
Stats*
readstats(Ndbtuple *t)
{
	Stats *stats = malloc(sizeof(Stats));
	
	t = ndbfindattr(t, t, "naccts");
	if(t == nil)		
		sysfatal("Could not find naccts entry in tuple");
	stats->naccts = atoi(t->val);
	
	t = ndbfindattr(t, t, "ntrans");
	if(t == nil)
		sysfatal("Could not find ntrans in tuple");
	stats->ntrans = atoi(t->val);

	t = ndbfindattr(t, t, "nbanks");
	//only master has nbanks attribute
	if(t != nil)
		stats->nbanks = atoi(t->val);
	
	return stats;
}

//readaccount reads tuple entry into malloced Account object
Account*
readaccount(Ndbtuple *t)
{
	Account *acct = malloc(sizeof(Account));

	t = ndbfindattr(t, t, "bank");
	if(t == nil)
		sysfatal("Could not find bank(id) in account tuple");
	acct->bank = atoi(t->val);

	t = ndbfindattr(t, t, "name");
	if(t == nil)
		sysfatal("Could not find name in account tuple");
	acct->name = strdup(t->val);

	t = ndbfindattr(t, t, "balance");
	if(t == nil)
		sysfatal("Could not find balance in account tuple");
	acct->balance = atoi(t->val);

	t = ndbfindattr(t, t, "pin");
		sysfatal("Coudl not find pin in account tuple");
	acct->pin = atoi(t->val);

	return acct;
}
