#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include "bankfs.h"

/* bank functions */

/* Bankfs layout:

	/
		ctl
		stats
		banks/
						0/
							accounts/
								0/
									name
									balance
								n…/
							transactions
							ctl
							stats
						n…/
*/

// Initialize a bank filetree -- acctnames must be size naccts -- requires an existing directory, root
void
initbank(File* root, char *user, uint naccts, char **acctnames)
{
	// Allocate bank
	Bank		*bank 	= mallocz(sizeof(Bank), 1);
	Transaction **trans	= mallocz(MAXTRANS * sizeof(Transaction*), 1);
	Stats		*s		= mallocz(sizeof(Stats), 1);
	Account		**accts	= mallocz(MAXACCTS * sizeof(Account*), 1);
	bank->transactions	= trans;
	bank->stats			= s;
	bank->accounts		= accts;

	// TODO -- might truncate, don't use atoi for uint
	uint bankid = atoi(root->name);

	stats->nbanks++;
	banks[bankid] = bank;

	// Create transactions & ctl files

	root->aux = bank;
	createfile(root, "transactions", user, OREADALL, trans);
	createfile(root, "stats", user, OREADALL, stats);
	createfile(root, "ctl", user, 0200, nil);
	// Makes accounts/ folder
	File *acctf = createfile(root, "accounts", user, DMDIR|ORDEXALL, accts);

	int i;
	for(i = 0; i < naccts; i++){
		// Make accounts/N for each desired
		accts[i] = mallocz(sizeof(Account), 1);
		initacct(acctf, user, itoa(i), acctnames[i], bankid, accts[i]);
	}
}

// Initialize an account filetree -- makes itself a directory
void
initacct(File *root, char *user, char *name, char *owner, uint bankid, Account *acct)
{
	acct->name = mallocz(strlen(owner)+1 * sizeof(char), 1);
	strcpy(acct->name, owner);
	acct->balance = 0;
	acct->bank = bankid;
	acct->pin = 0;

	banks[bankid]->stats->naccts++;
	
	File *acctdir = createfile(root, name, user, DMDIR|ORDEXALL, acct);

	createfile(acctdir, "name", user, OREADALL, nil);
	createfile(acctdir, "balance", user, OREADALL, nil);
}

/* Functions called by ctl files */

// Deletes a bank from the registry, makes its pointer nil
void
delbank(Bank *b, uint bankid)
{
	File *f;
	// TODO -- search for directory that holds this bank -- or have banks track their ID(?)
	char fpath[BUFSIZE];
	snprint(fpath, BUFSIZE, "%ud", bankid);
	f = walkfile(banksf, fpath);
	rmdir(f);
	Bankdestroy(b);
	banks[bankid] = nil;
	stats->nbanks--;
}

// Creates a bank under a given user name, this is for team usage -- TODO, all need error handling
void
mkbank(char *user)
{
	// TODO
	File	*f;
	char*	bankid = nil;
	int		i;
	
	for(i = 0; i < MAXBANKS; i++)
		if(banks[i] == nil){
			bankid = itoa(i);
			break;
		}
	
	if(bankid == nil){
		werrstr("err: out of bankid's; no bank made.");
		return;
	}
	
	f = createfile(banksf, bankid, user, DMDIR|ORDEXALL|OWRITE, nil);
	initbank(f, user, 0, nil);
}

// Transfer *amount* from n₀/from to n₁/to
void
trans(Bank *n₀, uint from, Bank *n₁, uint to, uint amount)
{
	n₀->accounts[from]->balance -= amount;
	n₁->accounts[to]->balance += amount;
}

// Dump everything to bankfs.ndb, backing up existing files to ./dumps/ if necessary
void
dump()
{
	// TODO

}


// Individual bank ctl logic

// Create an account for a bank using a given pin and owner name, initial balance is 0
void
mkacct(File *af, uint pin, char *owner)
{
	Account *a = emalloc(sizeof(Account));

	// Create account filesystem
	uint bankid = atoi(af->name);
	Bank *b = banks[bankid];
	char *acctid = itoa(b->stats->naccts);
	initacct(af, nil, acctid, owner, bankid, a);
	a->pin = pin;
}

// Delete an account by id -- removes both file tree and data structure
void
delacct(File *af, Bank *b, uint acctid)
{
	char buf[BUFSIZE];

	// Should be garbage collected…
	b->accounts[acctid] = nil;
	
	sprint(buf, "%ud", acctid);
	
	File *f = walkfile(af, buf);
	rmdir(f);
	
	b->stats->naccts--;
}

// 
void
modacct(Bank*, uint, uint, char*)
{
	// TODO

}

// 
void
atrans(Bank*, uint, Bank*, uint, uint, uint, char*)
{
	// TODO

}

/* Cleanup functionality */

// Free's the elements within a Bank
void
Bankdestroy(Bank *b)
{
	Statsdestroy(b->stats);
	Transendestroy(b->transactions);
	Acctsdestroy(b->accounts);
}

// Destroy a Stats
void	Statsdestroy(Stats*)
{
	// TODO

}

// Destroy a Transaction
void	Transdestroy(Transaction*)
{
	// TODO

}

// Destroy an Account
void	Acctdestroy(Account*)
{
	// TODO

}

// Destroy a set of transactions
void	Transendestroy(Transaction**)
{
	// TODO

}

// Destroy a set of accounts
void	Acctsdestroy(Account**)
{
	// TODO

}
