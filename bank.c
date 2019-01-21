#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include <ndb.h>
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

void
initbankfs(File *root, int bankid, char *user, Bank *bank)
{
	int i;

	banks[bankid] = bank;	
	root->aux = bank;
	
	createfile(root, "transactions", user, OREADALL, bank->transactions);
	createfile(root, "stats", user, OREADALL, bank->stats);
	createfile(root, "ctl", user, 0220, nil);
	// Makes accounts/ folder
	File *acctf = createfile(root, "accounts", user, DMDIR|ORDEXALL, bank->accounts);

	int n = 0;
	for(i = 0; i < MAXACCTS && n < bank->stats->naccts; i++)
		if(bank->accounts[i] != nil){
			initacctfs(acctf, i, user, bank->accounts[i]);
			n++;
		}
}

Bank*
initbank(void)
{
	// Allocate bank
	Bank		*bank 	= mallocz(sizeof(Bank), 1);
	Transaction **trans	= mallocz(MAXTRANS * sizeof(Transaction*), 1);
	Stats		*s		= mallocz(sizeof(Stats), 1);
	Account		**accts	= mallocz(MAXACCTS * sizeof(Account*), 1);
	bank->transactions	= trans;
	bank->stats			= s;
	bank->accounts		= accts;

	return bank;
}

/*Initialize Account struct */
Account*
initacct(char *owner, uint bankid, int balance, uint pin)
{
	Account *acct = mallocz(sizeof(Account), 1);

	acct->name = mallocz(strlen(owner)+1 * sizeof(char), 1);
	strcpy(acct->name, owner);
	acct->balance = balance;
	acct->bank = bankid;
	acct->pin = pin;

	return acct;
}

/*Initalize Account file tree*/
void
initacctfs(File *root, int acctid, char* user, Account *acct)
{
	char id[BUFSIZE];
	snprint(id, BUFSIZE, "%d", acctid);
	
	if(root == nil){
		fprint(2, "%r\n");
		fprint(2, "Error: You gave me a bad root for %s's %s.", user, id);
	}

	File *acctdir = createfile(root, id, user, DMDIR|ORDEXALL, acct);
	
	if(acctdir == nil){
		fprint(2, "%r\n");
		fprint(2, "Error: Creating acct dir failed for %s's %s.", user, id);
	}

	File *err = createfile(acctdir, "name", user, OREADALL, nil);
	
	if(err == nil){
		fprint(2, "%r\n");
		fprint(2, "Error: Creating name failed for %s's %s.", user, id);
	}

	err = createfile(acctdir, "balance", user, OREADALL, nil);
	
	if(err == nil){
		fprint(2, "%r\n");
		fprint(2, "Error: Creating balance failed for %s's %s.", user, id);
	}
}

/* Functions called by ctl files */

// Deletes a bank from the registry, makes its pointer nil
char*
delbank(Bank *b, uint bankid)
{
	if(b == nil)
		return "err: bank is nil";

	File *f;
	char fpath[BUFSIZE];
	snprint(fpath, BUFSIZE, "%ud", bankid);
	f = walkfile(banksf, fpath);
	
	if(f == nil)
		return "err: walking to f returned a nil";
	
	rmdir(f);
	Bankdestroy(b);
	banks[bankid] = nil;
	stats->nbanks--;

	return nil;
}

// Creates a bank under a given user name, this is for team usage
char*
mkbank(char *user)
{
	File	*f;
	int		i, bankid;
	Bank	*b;
	
	bankid = -1;
	for(i = 0; i < MAXBANKS; i++)
		if(banks[i] == nil){
			bankid = i;
			break;
		}
	
	if(bankid < 0)
		return "err: out of bankid's; no bank made.";
	
	f = createfile(banksf, itoa(bankid), user, DMDIR|ORDEXALL|OWRITE, nil);
	b = initbank();
	initbankfs(f, bankid, user, b);
	stats->nbanks++;
	
	return nil;
}

// Transfer *amount* from n₀/from to n₁/to
char*
trans(uint n₀, uint from, uint n₁, uint to, uint amount)
{
	if(banks[n₀] == nil)
		return "err: n₀ is nil";

	if(banks[n₁] == nil)
		return "err: n₁ is nil";

	Account *a₀ = banks[n₀]->accounts[from];
	Account *a₁ = banks[n₁]->accounts[to];

	if(a₀ == nil)
		return "err: a₀ is nil";
	
	if(a₁ == nil)
		return "err: a₁ is nil";
	
	a₀->balance -= amount;
	a₁->balance += amount;
	
	// Create transaction log
	
	Transaction *t₀ = emalloc(sizeof(Transaction));
	
	t₀->from	= from;
	t₀->to		= to;
	t₀->n₀		= n₀;
	t₀->n₁		= n₁;
	t₀->amt		= amount;
	t₀->stamp	= time(0);

	t₀->memo	= emalloc(BUFSIZE * sizeof(char));
	strncpy(t₀->memo, "FORCED ☹", BUFSIZE);
	
	banks[n₀]->transactions[banks[n₀]->stats->ntrans] = t₀;
	banks[n₀]->stats->ntrans++;

	if(n₀ != n₁){
		Transaction *t₁ = emalloc(sizeof(Transaction));
		t₁->from	= from;
		t₁->to		= to;
		t₁->n₁		= n₁;
		t₁->n₁		= n₁;
		t₁->amt		= amount;
		t₁->stamp	= time(0);

		t₁->memo	= emalloc(BUFSIZE * sizeof(char));
		strncpy(t₁->memo, "FORCED ☹", BUFSIZE);
	
		banks[n₁]->transactions[banks[n₁]->stats->ntrans] = t₁;
		banks[n₁]->stats->ntrans++;
	}
	
	return nil;
}

// Dump everything to bankfs.ndb, backing up existing files to ./dumps/ if necessary
void
dump()
{
	int fd, dirfd, i, n = 0;
	Biobuf *bp;
	
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
	
	bp = Bfdopen(fd, OWRITE);
	
	// Print master stats
	Bprint(bp, "%ω\n", masterstats());
	
	// Print banks to file -- fmtinstall for β should handle calling other fmt 
	for(i = 0; i < MAXBANKS && n < stats->nbanks; i++){
		if(banks[i] == nil)
			continue;
		
		// TODO -- notarize bankid in some better manner?
		Bprint(bp, "bankid=%d\n%β\n", i, banks[i]);
		n++;
	}

	// Tear down
	Bflush(bp);
	Bterm(bp);
	close(fd);
	close(dirfd);
}


// Individual bank ctl logic

// Create an account for a bank using a given pin and owner name, initial balance is 0
void
mkacct(File *af, uint pin, char *owner)
{
	Account *a;
	uint bankid = atoi(af->parent->name);
	Bank *b = banks[bankid];

	a = initacct(owner, bankid, 0, pin);

	int i, acctid = -1;
	for(i = 0; i < MAXACCTS; i++)
		if(b->accounts[i] == nil){
			acctid = i;
			break;
		}
	
	if(acctid >= MAXACCTS || acctid < 0)
		sysfatal("Error: out of accounts.");

	b->accounts[acctid] = a;

	initacctfs(af, acctid, owner, a);
	b->stats->naccts++;
}

// Delete an account by id -- removes both file tree and data structure
char*
delacct(File *af, Bank *b, uint acctid)
{
	char buf[BUFSIZE];
	
	if(b == nil)
		return "err: bank is nil";
	
	if(b->accounts[acctid] == nil)
		return "err: account is nil";

	Acctdestroy(b->accounts[acctid]);
	
	b->accounts[acctid] = nil;
	
	sprint(buf, "%ud", acctid);
	
	File *f = walkfile(af, buf);
	rmdir(f);
	
	b->stats->naccts--;
	
	return nil;
}

// Perform modifications on an account -- if non-nil, value is set
char*
modacct(Bank *b, uint acctid, uint *pin, char *name)
{
	if(b == nil)
		return "err: bank is nil";

	Account *a = b->accounts[acctid];

	if(a == nil)
		return "err: account is nil";

	if(pin)
		a->pin = *pin;
	if(name)
		strcpy(a->name, name);
	
	return nil;
}

// Perform authorized transactions
char*
atrans(uint n₀, uint from, uint n₁, uint to, uint amount, uint pin, char *memo)
{
	// n₀/from → n₁/to of amount with pin and memo…
	
	if(banks[n₀] == nil)
		return "err n₀ is nil";
	
	if(banks[n₁] == nil)
		return "err n₁ is nil";
	
	if(banks[n₀]->accounts[from] == nil)
		return "err m₀ is nil";
	
	if(banks[n₁]->accounts[to] == nil)
		return "err m₁ is nil";

	if(!(pin == banks[n₀]->accounts[from]->pin))
		return "err: pin does not match";
	
	banks[n₀]->accounts[from]->balance	-= amount;
	banks[n₁]->accounts[to]->balance	+= amount;
	
	// Create transaction log
	
	Transaction *t₀ = emalloc(sizeof(Transaction));
	
	t₀->from	= from;
	t₀->to		= to;
	t₀->n₀		= n₀;
	t₀->n₁		= n₁;
	t₀->amt		= amount;
	t₀->stamp	= time(0);

	t₀->memo	= emalloc(BUFSIZE * sizeof(char));
	strncpy(t₀->memo, memo, BUFSIZE);
	
	banks[n₀]->transactions[banks[n₀]->stats->ntrans] = t₀;
	banks[n₀]->stats->ntrans++;

	if(n₀ != n₁){
		Transaction *t₁ = emalloc(sizeof(Transaction));
	
		t₁->from	= from;
		t₁->to		= to;
		t₁->n₁		= n₁;
		t₁->n₁		= n₁;
		t₁->amt		= amount;
		t₁->stamp	= time(0);
	
		t₁->memo	= emalloc(BUFSIZE * sizeof(char));
		strncpy(t₁->memo, memo, BUFSIZE);

		banks[n₁]->transactions[banks[n₁]->stats->ntrans] = t₁;
		banks[n₁]->stats->ntrans++;
	}
	
	return nil;
}

/* Cleanup functionality */

// Free's the elements within a Bank
void
Bankdestroy(Bank *b)
{
	Transendestroy(b->transactions, b->stats->ntrans);
	Acctsdestroy(b->accounts, b->stats->naccts);
	Statsdestroy(b->stats);
	free(b);
}

// Destroy a Stats
void
Statsdestroy(Stats* s)
{
	free(s);
}

// Destroy a Transaction
void
Transdestroy(Transaction* t)
{
	free(t->memo);
	free(t);
}

// Destroy an Account
void
Acctdestroy(Account* a)
{
	free(a->name);
	free(a);
}

// Destroy a set of transactions
void
Transendestroy(Transaction** transen, uint ntrans)
{
	int i;
	for(i = 0; i < ntrans; i++){
		// TODO -- might be wrong?
		// stats->ntrans--;
		Transdestroy(transen[i]);
	}
}

// Destroy a set of accounts
void
Acctsdestroy(Account** accts, uint naccts)
{
	int i;
	for(i = 0; i < naccts; i++)
		Acctdestroy(accts[i]);
}
