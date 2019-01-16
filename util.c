#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include <bio.h>
#include <ndb.h>
#include "bankfs.h"

/* utility functions */

// Read from a file, if able
char*
readfile(Fid* fid)
{
	Qid		q		=	fid->qid;
	File	*f		=	fid->file;
	char	*name	=	f->name;
	char	buf[BUFSIZE*BUFSIZE];
	
	if(chatty9p)
		print("Read⇒ File->name: %s ¦ Qid.path: %ulld ¦ Parent->name: %s\n", name, q.path, f->parent->name);

	if(cmp(name, "stats") && pisroot(f)){
		// Return the text from the master stats file	
		snprint(buf, BUFSIZE*BUFSIZE, "%Ω", masterstats());

	}else if(cmp(name, "stats")){
		// Individual bank statistics
		Bank* b = f->parent->aux;
		snprint(buf, BUFSIZE*BUFSIZE, "bank=%s\n\t%Β", f->parent->name, b);

	}else if(cmp(name, "name")){
		// Account owner name
		Account *a = f->parent->aux;
		snprint(buf, BUFSIZE*BUFSIZE, "%s\n", a->name);

	}else if(cmp(name, "balance")){
		// Account balance
		Account *a = f->parent->aux;
		snprint(buf, BUFSIZE*BUFSIZE, "%d\n", a->balance);

	}else if(cmp(name, "transactions")){
		// Transaction history for a bank
		Bank* b = f->parent->aux;
		snprint(buf, BUFSIZE*BUFSIZE, "%Τ", b);

	}else{
		// Return catch-all
		snprint(buf, BUFSIZE*BUFSIZE, "err: nothing to see here\n");
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
		fprint(2, "user sent to master: %s", str);

		return mastercmd(str);

	}else if(cmp(name, "ctl")){
		// Bank ctl file
		uint bankid = atoi(f->parent->name);
		// Bank *b = f->parent->aux;
		
		fprint(2, "user sent to bank %d: %s", bankid, str);

		return bankcmd(f->parent, str);

	}else{
		// Return catch-all
		snprint(buf, BUFSIZE, "err: nothing to write here\n");
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

// Process individual bank input command language ;; returns nil on success
char*
bankcmd(File *f, char *str)
{
	uint		bankid = atoi(f->name);
	int			nfields;//, i;
	char		*buf[MAXARGS];
	char		*cmd;

	nfields = getfields(str, buf, MAXARGS, 1, " 	\n");
	
	// Debug output
	/*
	fprint(2, "%d fields to bank %d cmd:\n", nfields, bankid);
	for(i = 0; i < nfields; i++)
		fprint(2, "%s\n", buf[i]);
	*/
	
	if(nfields < 2)
		return "err: each command requires at least 1 arg";

	cmd = buf[0];
	if(cmp(cmd, "mkacct")){
		// Create an account
		// mkacct pin name…
		if(nfields < 3)
			return "err: incorrect arg count to mkacct";
		
		File *af = walkfile(f, "accounts");
		// fprint(2, "parent: %s us: %s\n", f->parent->name, f->name);
		uint pin = atoi(buf[1]);
		
		// TODO -- use all fields past and including 2 as name...
		mkacct(af, pin, buf[2]);
	
	}else if(cmp(cmd, "delacct")){
		// Delete an account
		// delacct id
		if(nfields != 2)
			return "err: incorrect arg count to delacct";
		
		File *af = walkfile(f, "accounts");
		uint acctid = atoi(buf[1]);
		
		return delacct(af, banks[bankid], acctid);
	
	}else if(cmp(cmd, "modacct")){
		// Modify an account
		// modacct id pin name…
		// Pass a paramiter as <nil> to indicate a nil entry
		if(nfields < 4)
			return "err: incorrect arg count to modacct";
		
		uint acctid = atoi(buf[1]);
		uint *pin = nil;
		char *name = nil;
		
		if(!cmp(buf[2], "<nil>")){
			pin = emalloc(sizeof(uint));
			*pin = atoi(buf[2]);
		}
		
		if(!cmp(buf[3], "<nil>")){
			name = emalloc(BUFSIZE * sizeof(char));
			strncpy(name, buf[3], BUFSIZE);
		}
		
		char *msg = modacct(banks[bankid], acctid, pin, name);
		
		if(pin)
			free(pin);
		if(name)
			free(name);
		
		return msg;
	
	}else if(cmp(cmd, "atrans")){
		// Perform an authorized transfer
		// atrans from n to amount pin memo…
		if(nfields < 7)
			return "err: incorrect arg count to atrans";
		
		uint from	= atoi(buf[1]);
		uint n₁		= atoi(buf[2]);
		uint to		= atoi(buf[3]);
		uint amount	= atoi(buf[4]);
		uint pin	= atoi(buf[5]);

		char memo[BUFSIZE];
		
		// TODO -- use all fields after and including 6 for memo
		strncpy(memo, buf[6], BUFSIZE);
		
		return atrans(bankid, from, n₁, to, amount, pin, memo);
	
	}else
		return "err: unknown cmd";

	return nil;
}

// Process master input command language ;; returns nil on success
char*
mastercmd(char *str)
{
	int			nfields;	//, i;
	char		*buf[MAXARGS];
	char		*cmd;

	nfields = getfields(str, buf, MAXARGS, 1, " 	\n");
	
	// Debug output
	/*
	fprint(2, "%d fields to master cmd:\n", nfields);
	for(i = 0; i < nfields; i++)
		fprint(2, "%s\n", buf[i]);
	*/
	
	if(nfields < 2)
		return "err: each command requires at least 1 arg";
	
	cmd = buf[0];
	if(cmp(cmd, "mkbank")){
		// Create a new bank
		// mkbank user
		int i;
		if(nfields != 2)
			return "err: incorrect arg count to mkbank";
			
		// Check for available bankid
		for(i = 0; i < MAXBANKS; i++)
			if(banks[i] == nil)
				break;

		if(i >= MAXBANKS)
			return "err: no bank slots left";
		
		// Should pass bankid
		return mkbank(buf[1]);

	}else if(cmp(cmd, "delbank")){
		// Delete a bank
		// delbank id
		if(nfields != 2)
			return "err: incorrect arg count to delbank";

		uint bankid = atoi(buf[1]);

		if(bankid <= 0)
			return "err: invalid bank ID range";
		if(banks[bankid] == nil)
			return "err: bank is nil";
		
		return delbank(banks[bankid], bankid);

	}else if(cmp(cmd, "trans")){
		// Transfer funds
		// trans n₀ from n₁ to amt
		if(nfields != 6)
			return "err: incorrect arg count to trans";
		
		uint n₀		= atoi(buf[1]);
		uint from	= atoi(buf[2]);
		uint n₁		= atoi(buf[3]);
		uint to		= atoi(buf[4]);
		uint amt	= atoi(buf[5]);
		
		return trans(n₀, from, n₁, to, amt);

	}else if(cmp(cmd, "dump")){
		/*	
			Dumps the bank to bankfs.ndb, copying the existing file, if any to
			./dump/bankfs.ndb.$date
			where $date is time since the epoch in the local time zone, as per `date -n`
			Since one arg is required, the arg is ignored. Do: dump <nil> to emphasize
		*/
		
		dump();
	
	}else
		return "err: unknown cmd";

	// Success
	return nil;
}

/* File cleanup */

// Recursively clean up a file tree from File *f
void
rmdir(File *froot)
{
	int err;
	int nchildren = froot->nchild;
	
	fprint(2, "attempting to delete: %s\n", froot->name);

	incref(froot);
	err = removefile(froot);
	if(err >= 0){
		fprint(2, "deleted %s\n", froot->name);
		return;
	}

	// Directory is not empty
	Readdir *dir = opendirfile(froot);

	uchar *buf = emalloc(nchildren * DIRMAX);
	Dir d;
	char *strs = emalloc(STATMAX);
			
	// o=0 if a dir, o=1 if not ;; n is number of bytes in buf
	long dirsize = readdirfile(dir, buf, nchildren * DIRMAX, 0);
	
	int i;
	// Delete all the children
	for(i = 0; i < nchildren; i++){
		incref(froot);
		uint bytes = convM2D(buf, dirsize, &d, strs);
		buf += bytes;
		
		//fprint(2, "bytes: %ud on %s\n", bytes, d.name);
		
		File *ftorm = walkfile(froot, d.name);
		if(ftorm == nil){
			fprint(2, "ftorm is nil!\n");
		}

		//fprint(2, "parent refs: %ld\n", froot->Ref.ref);
		rmdir(ftorm);
		//fprint(2, "parent refs: %ld\n", froot->Ref.ref);
	}
	
	// Clean up once children gone		
	closedirfile(dir);

	// Remove the file once all children are gone
	removefile(froot);
}

// Panickable mallocz
void*
emalloc(ulong sz)
{
	void *ptr;

	ptr = mallocz(sz, 1);
	if(!ptr)
		sysfatal("ON NO MALLOC FAILED FUCK");
	return ptr;
}

// Generates, on demand, the master statistics for the given instant
Stats
masterstats(void)
{
	Stats s = {stats->nbanks, 0, 0};
	int i;
	
	for(i = 0; i < MAXBANKS; i++){
		if(banks[i] != nil){
			s.naccts += banks[i]->stats->naccts;
			s.ntrans += banks[i]->stats->ntrans;
		}
	}

	return s;
}
