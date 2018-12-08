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
initacct(File *root, char *user, char *name, char *owner, uint bank, Account *acct)
{
	acct->name = mallocz(strlen(owner)+1 * sizeof(char), 1);
	strcpy(acct->name, owner);
	acct->balance = 0;
	acct->bank = bank;
	acct->pin = 0;

	banks[bank]->stats->naccts++;
	// Maybe stop maintaining two sets of records
	stats->naccts++;
	
	File *acctdir = createfile(root, name, user, DMDIR|ORDEXALL, acct);

	createfile(acctdir, "name", user, OREADALL, nil);
	createfile(acctdir, "balance", user, OREADALL, nil);
}
