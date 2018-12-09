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
		// Return the text from the stats file
		Stats *s = (Stats*) f->aux;
		int i, naccts = 0, ntrans = 0;
		
		for(i = 0; i < stats->nbanks; i++){
			naccts += banks[i]->stats->naccts;
			ntrans += banks[i]->stats->ntrans;
		}	
		
		snprint(buf, BUFSIZE*BUFSIZE, "nbanks: %ud naccts: %ud ntrans: %ud\n", s->nbanks, naccts, ntrans);
	}else if(cmp(name, "stats")){
		// Individual bank statistics
		Bank* b = f->parent->aux;
		snprint(buf, BUFSIZE*BUFSIZE, "naccts: %ud ntrans: %ud\n", b->stats->naccts, b->stats->ntrans);
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
		if(b->stats->ntrans > 0){
			int i;
			for(i = 0; i < b->stats->ntrans; i++){
				// Achtung! this is unsafe -- find a better way -- TODO
				Transaction *t = b->transactions[i];
				char trans[128];
				snprint(trans, 128, "%ud → %ud of %ud at %ld\n", t->from, t->to, t->amt, t->stamp);
				strcat(buf, trans);
			}
		}else{
			sprint(buf, "");
		}
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
		
		// TODO
		
		// Success response
		return nil;
	}else if(cmp(name, "ctl")){
		// Bank ctl file
		uint bankid = atoi(f->parent->name);
		
		fprint(2, "user sent to bank %d: %s", bankid, str);
		
		// TODO
		
		// Success
		return nil;
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
