#include <u.h>
#include <libc.h>
#include <bio.h>
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
trans(uint n₀, uint from, uint n₁, uint to, uint amount)
{
	Account *a₀ = banks[n₀]->accounts[from];
	Account *a₁ = banks[n₁]->accounts[to];
	
	a₀->balance -= amount;
	a₁->balance += amount;
	
	// Create transaction log
	
	Transaction *t = emalloc(sizeof(Transaction));
	
	t->from		= from;
	t->to		= to;
	t->n₀		= n₀;
	t->n₁		= n₁;
	t->amt		= amount;
	t->stamp	= time(0);

	t->memo	= emalloc(BUFSIZE * sizeof(char));
	strncpy(t->memo, "FORCED ☹", BUFSIZE);
	
	banks[n₀]->transactions[banks[n₀]->stats->ntrans] = t;
	banks[n₀]->stats->ntrans++;

	if(n₀ != n₁){
		banks[n₁]->transactions[banks[n₁]->stats->ntrans] = t;
		banks[n₁]->stats->ntrans++;
	}
}

// Dump everything to bankfs.ndb, backing up existing files to ./dumps/ if necessary
void
dump()
{
	int fd, dirfd, i, n = 0;
	
	// Open ./dumps/, create if needed
	dirfd = open("./dumps", OREAD);
	if(dirfd < 0){
		dirfd = create("./dumps", OREAD, DMDIR | 0770);
		// print("err is %r\n");
		if(dirfd < 0)
			sysfatal("err: failed to create dumps folder!");
	}
	
	// Test for an existing ./bankfs.ndb
	fd = open("./bankfs.ndb", OWRITE);
	if(fd > 0){
		// File exists, move it to ./dumps/bankfs.ndb.$time
		close(fd);

		char to[35]; // chars of ./dumps/1545865537.bankfs.ndb
		
		snprint(to, 35, "./dumps/%ld.bankfs.ndb", time(0));
	
		char *argv[] = {"/bin/mv", "bankfs.ndb", to, nil};
		
		// fprint(2, "{%s, %s, %s}\n", argv[0], argv[1], argv[2]);

		switch(fork()){
		case 0:
			// Child
			exec(argv[0], argv);
			break;
		default:
			// TODO -- some kind of race condition, bad fix
			sleep(250);
		}
	}

	fd = create("./bankfs.ndb", OWRITE, 0660);
	if(fd < 0)
		sysfatal("err: failed to create bankfs.ndb!");
	
	// Print master stats
	fprint(fd, "%ω\n", masterstats());
	
	// Print banks to file -- fmtinstall for β should handle calling other fmt 
	for(i = 0; i < MAXBANKS && n < stats->nbanks; i++){
		if(banks[i] == nil)
			continue;
		
		// TODO -- notarize bankid in some better manner?
		fprint(fd, "bankid=%d\n%β\n", i, banks[i]);
		n++;
	}

	// Tear down
	close(fd);
	close(dirfd);
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
	banks[bankid]->accounts[b->stats->naccts] = a;
	initacct(af, nil, acctid, owner, bankid, a);
	a->pin = pin;
}

// Delete an account by id -- removes both file tree and data structure
void
delacct(File *af, Bank *b, uint acctid)
{
	char buf[BUFSIZE];

	Acctdestroy(b->accounts[acctid]);
	
	b->accounts[acctid] = nil;
	
	sprint(buf, "%ud", acctid);
	
	File *f = walkfile(af, buf);
	rmdir(f);
	
	b->stats->naccts--;
}

// Perform modifications on an account -- if non-nil, value is set
void
modacct(Bank *b, uint acctid, uint *pin, char *name)
{
	Account *a = b->accounts[acctid];

	if(pin)
		a->pin = *pin;
	if(name)
		strcpy(a->name, name);
}

// Perform authorized transactions -- TODO -- error checking and returning?
void
atrans(uint n₀, uint from, uint n₁, uint to, uint amount, uint pin, char *memo)
{
	// n₀/from → n₁/to of amount with pin and memo…

	if(!(pin == banks[n₀]->accounts[from]->pin))
		return;
	
	banks[n₀]->accounts[from]->balance	-= amount;
	banks[n₁]->accounts[to]->balance	+= amount;
	
	// Create transaction log
	
	Transaction *t = emalloc(sizeof(Transaction));
	
	t->from		= from;
	t->to		= to;
	t->n₀		= n₀;
	t->n₁		= n₁;
	t->amt		= amount;
	t->stamp	= time(0);

	t->memo	= emalloc(BUFSIZE * sizeof(char));
	strncpy(t->memo, memo, BUFSIZE);
	
	banks[n₀]->transactions[banks[n₀]->stats->ntrans] = t;
	banks[n₀]->stats->ntrans++;

	if(n₀ != n₁){
		banks[n₁]->transactions[banks[n₁]->stats->ntrans] = t;
		banks[n₁]->stats->ntrans++;
	}
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
