#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
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
		Stats s = {((Stats*) f->aux)->nbanks, 0, 0};
		int i;
		
		for(i = 0; i < s.nbanks; i++){
			s.naccts += banks[i]->stats->naccts;
			s.ntrans += banks[i]->stats->ntrans;
		}	
		
		snprint(buf, BUFSIZE*BUFSIZE, "%Σ", &s);

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
		snprint(buf, BUFSIZE*BUFSIZE, "%Τ\n", b);

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
		
		// Success response
		return nil;
	}else if(cmp(name, "ctl")){
		// Bank ctl file
		uint bankid = atoi(f->parent->name);
		// Bank *b = f->parent->aux;
		
		fprint(2, "user sent to bank %d: %s", bankid, str);

		return bankcmd(f, str);

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
	int			nfields, i;
	char		*buf[MAXARGS];

	nfields = getfields(str, buf, MAXARGS, 1, " 	");
	
	fprint(2, "%d fields to bank %d cmd:\n", nfields, bankid);
	for(i = 0; i < nfields; i++)
		fprint(2, "%s\n", buf[i]);

	// TODO

	return nil;
}

// Process master input command language ;; returns nil on success
char*
mastercmd(char *str)
{
	int			nfields, i;
	char		*buf[MAXARGS];

	nfields = getfields(str, buf, MAXARGS, 1, " 	");
	
	fprint(2, "%d fields to master cmd:\n", nfields);
	for(i = 0; i < nfields; i++)
		fprint(2, "%s\n", buf[i]);

	// TODO

	return nil;
}
