#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include "bankfs.h"


// Global variables -- icky
Tree	*banktree;
Stats	*stats;


// Prototypes for 9p handler functions
static void		fsattach(Req *r);
static int		getdirent(int n, Dir *d, void *);
static void		fsread(Req *r);
static void		fswrite(Req *r);
static char*	fswalk1(Fid * fid, char *name, Qid *qid);
static char*	fsclone(Fid *fid, Fid *newfid);
static void		fsstat(Req *r);
static void		freefid(Fid *fid);

// Prototypes for auth routines
void	becomenone(void);

// Prototypes for utilities
char*	readfile(Fid*);
char*	writefile(Fid*, char*);
char*	uitoa(uint);
char*	itoa(int);

// Prototypes for bank calls
void	initbank(File*, char*, uint, char**);
void	initacct(File*, char*, char*, char*, uint, Account*);


// Srv structure to handle incoming 9p communications
Srv fs = 
{
	.auth		=	auth9p,
	.attach		=	fsattach,
	.read		=	fsread,
	.write		=	fswrite,
	.create		=	nil,
	.remove		=	nil,
	.walk1		=	fswalk1,
	.clone		= 	fsclone,
	.stat		=	nil,
//	.destroyfid	=	freefid,
	.destroyfid	=	nil,
	.keyspec	=	"proto=p9any role=server",
};


// Usage output
void
usage(void)
{
	fprint(2, "usage: %s [-D] [-s srv] [-m mnt] [-a address]\n", argv0);
	exits("usage");
}


/* A 9p fileserver to serve a bank over an unsecured 9p connection */
void
main(int argc, char *argv[])
{
	char	*mnt, *srv, *addr, *ndb, *hostuser, *hostgrp;

	ndb			=	nil;
	srv			=	nil;
	hostuser	=	"glenda";
	hostgrp		=	"sys";
	mnt			=	"/mnt/bankfs";
	// Disable for development, listener works
	addr		=	nil;
	//addr		=	"tcp!*!3656";

	ARGBEGIN{
	case 'D':
		// Debug chatter
		chatty9p++;
		break;
	case 's':
		// #s name to post
		srv = EARGF(usage());
		break;
	case 'm':
		// Directory path to mount to
		mnt = EARGF(usage());
		break;
	case 'a':
		// Dial string address to listen on
		addr = EARGF(usage());
		break;
	case 'd':
		// .ndb file to load from (if any)
		ndb = EARGF(usage());
		break;
	case 'u':
		// User to root as
		hostuser = EARGF(usage());
		break;
	case 'g':
		// Group to root as
		hostgrp = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if(argc)
		usage();

	// Setup filesystem
	stats = mallocz(sizeof(Stats), 1);
	Stats s = {
		.nbanks = 0,	// We are bank #0
		.naccts = 0,	// One master reserve account #0
		.ntrans = 0,	// No transactions initially
	};
	*stats = s;

	// see: 9pfile(2)
	banktree = fs.tree = alloctree(hostuser, hostgrp, DMDIR|0775, nil);

	createfile(fs.tree->root, "stats", nil, OREADALL, stats);

	createfile(fs.tree->root, "ctl", nil, 0200, nil);

	File *banksf = createfile(fs.tree->root, "banks", nil, DMDIR|ORDEXALL, nil);

	// Load from database here

	if(ndb){
		// We are loading from database
		// TODO
	}else{
		// We are not loading from database -- initialize
		// Add root as /banks/0
		File *bankroot = createfile(banksf, "0", hostuser, DMDIR|ORDEXALL, nil);
		char *rname[] = {"reserve"};
		initbank(bankroot, hostuser, 1, rname);
	}

	// Start listening
	if(addr){
		fprint(2, "Listening on: %s\n", addr);
		listensrv(&fs, addr);
	}

	fprint(2, "Serving on: %s and mounting to: %s\n", srv, mnt);
	postmountsrv(&fs, srv, mnt, MREPL|MCREATE);
	
	exits(nil);
}


/* Prototyped functions */

// Handle 9p attach -- independent implementation
static void
fsattach(Req *r)
{
	r->fid->qid = (Qid) { 0, 0, QTDIR };
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

// Handle 9p read
static void
fsread(Req *r)
{
	Fid		*fid;
	Qid		q;
	File	*f;
	char	readmsg[BUFSIZE];

	fid = r->fid;
	q = fid->qid;
	f = fid->file;
	
	if(f)
		strncpy(readmsg, readfile(fid), BUFSIZE);
	else
		strcpy(readmsg, "fsread: invalid read attempt\n");
	
	// Set the read reply string
	readstr(r, readmsg);
	
	// Respond to the 9p request
	respond(r, nil);
}

// Handle 9p write
static void
fswrite(Req *r)
{
	Fid		*fid;
	Qid		q;
	File	*f;
	char	str[BUFSIZE];

	fid = r->fid;
	q = fid->qid;
	f = fid->file;

	// TODO -- maybe remove DIR stuff
	if(q.type & QTDIR){
		respond(r, "permission denied from fswrite");
		return;
	}

	if(r->ifcall.count > sizeof(str) - 1){
		respond(r, "string too large");
		return;
	}
	memmove(str, r->ifcall.data, r->ifcall.count);
	str[r->ifcall.count] = 0;
	
	// At this point, str contains the bytes written to our file

	if(f){
		// Determine if a valid write and write if so
		respond(r, writefile(fid, str));
		return;
	}else{
		respond(r, "fswrite: invalid write attempt");
		return;
	}
	
	// Should never be reached
}

// Handle 9p walk -- independent implementation
static char *
fswalk1(Fid * fid, char *name, Qid *qid)
{
	Qid q;
//	int i;

	q = fid->qid;
	if(!(q.type && QTDIR)){
		if(!strcmp(name, "..")){
			fid->qid = (Qid) {0, 0, QTDIR};
			*qid = fid->qid;
			fid->aux = nil;
			return nil;
		}
	}else{
		// Add walk logic
	}
	return "no such directory.";
}

// Handle 9p clone -- independent implementation
static char *
fsclone(Fid *fid, Fid *newfid)
{
	File *f;
	
	f = fid->aux;
	if(f != nil)
		incref(f);
	newfid->aux = f;
	return nil;
}

static void
freefid(Fid *fid)
{
	File *f;

	if(fid->qid.type & QTAUTH)
		authdestroy(fid);
	else{
		f = fid->aux;
		fid->aux = nil;
		// ↓ causes a segfault, naughty naughty
		closefile(f);
	}
}

/* auth functions */

// TODO

/* utility functions */

// Read from a file, if able
char*
readfile(Fid* fid)
{
	Qid		q		=	fid->qid;
	File	*f		=	fid->file;
	char	*name	=	f->name;
	char	buf[BUFSIZE];
	
	if(chatty9p)
		print("Read⇒ File->name: %s ¦ Qid.path: %ulld ¦ Parent->name: %s\n", name, q.path, f->parent->name);

	if(cmp(name, "stats") && pisroot(f)){
		// Return the text from the stats file
		Stats *s = (Stats*) fid->file->aux;
		snprint(buf, BUFSIZE, "nbanks: %ud naccts: %ud ntrans: %ud\n", s->nbanks, s->naccts, s->ntrans);
		
	}else{
		// Return catch-all
		snprint(buf, BUFSIZE, "err: readfile says no ☹");
	}
	
	return buf;
}

// Write to a file, if able
char*
writefile(Fid* fid, char* str)
{
	Qid		q		=	fid->qid;
	File	*f		=	fid->file;
	char	*name	=	f->name;
	char	buf[BUFSIZE];
	
	if(chatty9p)
		print("Write⇒ File->name: %s ¦ Qid.path: %ulld ¦ Parent->name: %s\n", name, q.path, f->parent->name);

	if(cmp(name, "ctl") && pisroot(f)){
		// Do something with the master control file commands
		fprint(2, "user sent: %s", str);
		
		// TODO
		
		// Success response
		return nil;
	}else{
		// Return catch-all
		snprint(buf, BUFSIZE, "err: writefile says no ☹\n");
	}

	return buf;
}

// Convert a signed integer to a string
char*
itoa(int n)
{
	char buf[BUFSIZE];
	snprint(buf, BUFSIZE, "%d", n);
	return buf;
}

// Convert an unsigned integer to a string
char*
uitoa(uint n)
{
	char buf[BUFSIZE];
	snprint(buf, BUFSIZE, "%ud", n);
	return buf;
}


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
	Transaction *trans	= mallocz(MAXTRANS * sizeof(Transaction), 1);
	Stats		*s		= mallocz(sizeof(Stats), 1);
	Account		*accts	= malloc(MAXACCTS * sizeof(Account));
	bank->transactions = trans;
	bank->stats = s;
	bank->accounts = accts;

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
		
		// TODO -- might truncate, don't use atoi for uint
		initacct(acctf, user, itoa(i), acctnames[i], atoi(root->name), &accts[i]);
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
	File *acctdir = createfile(root, name, user, DMDIR|ORDEXALL, acct);

	createfile(acctdir, "name", user, OREADALL, nil);
	createfile(acctdir, "balance", user, OREADALL, nil);
}