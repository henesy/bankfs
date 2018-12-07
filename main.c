#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include "bankfs.h"

// Global variables -- icky
Tree	*banktree;
Stats	stats;

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
void becomenone(void);

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
	char	*mnt, *srv, *addr;

	srv = nil;
	// Disable for development, it works
	// addr = "tcp!*!3656";
	addr = nil;
	mnt = "/mnt/bankfs";

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 's':
		srv = EARGF(usage());
		break;
	case 'm':
		mnt = EARGF(usage());
		break;
	case 'a':
		// You can nil-out addr if desired
		addr = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if(argc)
		usage();

	// Setup filesystem
	banktree = fs.tree = alloctree("sys", "sys", DMDIR|0775, nil);
	createfile(fs.tree->root, "stats", nil, OREADALL, nil);
	createfile(fs.tree->root, "banks", nil, DMDIR|ORDEXALL, nil);
	//createfile(fs.tree->root, "accounts", nil, DMDIR|ORDEXALL, nil);

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

	switch(q.path){
	case 0:
		// TODO
		break;
	default:
		strcpy(readmsg, "fsread: invalid read attempt\n");
	}
	
	if(f != nil)
		print("File->name: %s ¦ Qid.path: %ulld ¦ Parent->name: %s\n", f->name, q.path, f->parent->name);
	
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
	char	str[BUFSIZE];

	fid = r->fid;
	q = fid->qid;
	if(q.type & QTDIR){
		respond(r, "permission denied.");
		return;
	}
	if(r->ifcall.count > sizeof(str) - 1){
		respond(r, "string too large");
		return;
	}
	memmove(str, r->ifcall.data, r->ifcall.count);
	str[r->ifcall.count] = 0;
	
	// At this point, str contains the written bytes
	
	switch(q.path){
	case 0:
		// TODO
		break;
	default:
		respond(r, "fswrite: invalid write attempt");
		return;
	}
	
	respond(r, nil);
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

