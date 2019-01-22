#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "bankfs.h"

void
readndb(File *root, char* file)
{
	Ndbtuple *t;
	Ndb *n;
	int id = 0;

	n = ndbopen(file);
	if(n == nil)
		sysfatal("Could not open %s", file);

	//Read in master stats
	t = ndbparse(n);
	stats = readstats(t);

	Bank *b = nil;
	while((t = ndbparse(n)) != nil){
		if(cmp(t->attr, "bankid")){
			if(b != nil)
				initbankfs(root, id, b);
			id = atoi(t->val);
			b = readbank(t);
		}else if(cmp(t->attr, "acctid")){
			if(b == nil)
				sysfatal("Orphaned account tuple with no bank");
			b->accounts[atoi(t->val)] = readacct(t);
		}else if(cmp(t->attr, "transid")){
			if(b == nil)
				sysfatal("Orphaned transaction tuple with no bank");
			b->transactions[atoi(t->val)] = readtrans(t);
		}	
	}

	//We order banks first, so we still have one left to add at EOF
	if(b != nil)
		initbankfs(root, id, b);
}

Bank*
readbank(Ndbtuple *t)
{
	Bank *b;
	t = ndbfindattr(t, t, "user");
	if(t == nil)
		sysfatal("Could not find user entry in tuple");
	b = initbank(smprint("%s", t->val));
	free(b->stats);
	b->stats = readstats(t);
	return b;
}

//Returns Stats object for master from bankfs.ndb
//n will be filled with ndb fd at to master stats tuple
Stats*
readstats(Ndbtuple *t)
{
	Stats *stats = emalloc(sizeof(Stats));
	
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
readacct(Ndbtuple *t)
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
	if(t == nil)
		sysfatal("Could not find pin in account tuple");
	acct->pin = atoi(t->val);

	return acct;
}

Transaction*
readtrans(Ndbtuple *t)
{
	Transaction *trans = mallocz(sizeof(Transaction), 1);
	char *buf[2];

	t = ndbfindattr(t, t, "from");
	if(t == nil)
		sysfatal("Could not find from in transaction tuple");
	if(getfields(t->val, buf, 2, 0, "/") != 2)
		sysfatal("Bad format of from entry in transaction tuple");
	trans->n₀ = atoi(buf[0]);
	trans->from = atoi(buf[1]);

	t = ndbfindattr(t, t, "amount");
	if(t == nil)
		sysfatal("Could not find amount in transaction tuple");
	trans->amt = atoi(t->val);

	t = ndbfindattr(t, t, "to");
	if(t == nil)
		sysfatal("Could not find to in transaction tuple");
	if(getfields(t->val, buf, 2, 0, "/") != 2)
		sysfatal("Bad format of to entry in transaction tuple");
	trans->n₀ = atoi(buf[0]);
	trans->to = atoi(buf[1]);

	t = ndbfindattr(t, t, "memo");
	if(t == nil)
		sysfatal("Could not find memo in transaction tuple");
	trans->memo = strdup(t->val);
	
	t = ndbfindattr(t, t, "stamp");
	if(t == nil)
		sysfatal("Could not find stamp in transaction tuple");
	trans->stamp = atol(t->val);

	return trans;
}
