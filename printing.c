#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include <bio.h>
#include <ndb.h>
#include "bankfs.h"

// The documentation for these methods is found in fmtinstall(2)

/* for printing over file-reads */

// Print some kind of statistics(?) -- what did you do moody
static int 
printstats(Fmt *f, Stats *s)
{
	return fmtprint(f, "naccts=%ud\n\tntrans=%ud\n", s->naccts, s->ntrans);
}

// Print individual bank statistics(?)
int
Bankfmtr(Fmt *f)
{
	Bank *b;

	b = va_arg(f->args, Bank*);

	return printstats(f, b->stats);
}

// Print some kind of statistics(?) -- see above
int
Statsfmtr(Fmt *f)
{
	Stats *s;
	int r;

	s = va_arg(f->args, Stats*);
	r = fmtprint(f, "nbanks=%ud\n\t", s->nbanks);
	r += printstats(f, s);

	return r;
}

// Τ, Print a transaction list for a bank
int
Transfmtr(Fmt *f)
{
	Bank *b;
	int r, i;

	b = va_arg(f->args, Bank*);
	r = fmtprint(f, "");
	for(i=0; i < b->stats->ntrans; i++){
		Transaction *t = b->transactions[i];
		
		r += fmtprint(f, "from=%ud/%ud\n\tamount=%ud\n\tto=%ud/%ud\n\tmemo=%s\n\tstamp=%ld\n", t->n₀, t->from, t->amt, t->n₁, t->to, t->memo, t->stamp);
	}

	return r;
}

// Ω, ω, Print master (sum) statistics
int
Masterfmtr(Fmt *f)
{
	Stats s;
	int r;
	
	s = va_arg(f->args, Stats);
	r = fmtprint(f, "");
	
	r += fmtprint(f, "nbanks=%ud naccts=%ud ntrans=%ud\n", s.nbanks, s.naccts, s.ntrans);

	return r;
}

/* for printing to bankfs.ndb */

// σ, Dump statistics for an individual bank
int
Statsfmtw(Fmt *f)
{
	Stats *s;
	int r;
	
	r = fmtprint(f, "");
	s = va_arg(f->args, Stats*);
	
	r += fmtprint(f, "\tnaccts=%ud\n\tntrans=%ud\n", s->naccts, s->ntrans);

	return r;
}

// α, Dump individual account details
int
Acctfmtw(Fmt *f)
{
	Account *a;
	int r;
	
	r = fmtprint(f, "");
	a = va_arg(f->args, Account*);
	
	r += fmtprint(f, "\tbank=%ud\n\tname=%s\n\tbalance=%d\n\tpin=%ud\n", a->bank, a->name, a->balance, a->pin);
	
	return r;
}

// τ, Dump individual transaction details
int
Transfmtw(Fmt *f)
{
	int r;
	Transaction *t;
	
	r = fmtprint(f, "");
	t = va_arg(f->args, Transaction*);
	
	r += fmtprint(f, "\tfrom=%ud/%ud\n\tamount=%ud\n\tto=%ud/%ud\n\tmemo=%s\n\tstamp=%ld\n", t->n₀, t->from, t->amt, t->n₁, t->to, t->memo, t->stamp);
	
	return r;
}

// β, Dump a bank and all its members, call relevant other prints that exist
int
Bankfmtw(Fmt *f)
{
	/*
		Format of the bank dump is as per:
		[bank statistics]
		[account list]
		[transaction list]
	*/

	int r, i, n = 0;
	Bank *b;
	
	b = va_arg(f->args, Bank*);
	r = fmtprint(f, "\tuser=%s\n", b->user);
	
	// Print statistics
	r += fmtprint(f, "%σ\n", b->stats);
	
	// Print account list
	for(i = 0; i < MAXACCTS && n < b->stats->naccts; i++){
		if(b->accounts[i] == nil)
			continue;

		r += fmtprint(f, "acctid=%d\n%α\n", i, b->accounts[i]);
		n++;
	}
	
	// Print transaction list
	for(i = 0, n = 0; i < MAXTRANS && n < b->stats->ntrans; i++){
		if(b->transactions[i] == nil)
			continue;

		r += fmtprint(f, "transid=%d\n%τ\n", i, b->transactions[i]);
		n++;
	}
	
	return r;
}

/* utilities */

// Install formatters for printing bank data structures
void
bankfsfmtinstall(void)
{
	/*	Typing a greek letter in 9 is as per `alt + *[letter]`.
		All keycodes are stored in /lib/keyboard, which is grep-able.
		Read functions are re-used if there are no hidden fields.
		Capital runes indicate use for reads, lower for ndb writes.
	*/

	// Print master statistics
	fmtinstall(L'Ω', Masterfmtr); // TODO -- unused atm
	fmtinstall(L'ω', Masterfmtr);

	// This naming sucks.
	fmtinstall(L'Β', Bankfmtr);
	fmtinstall(L'β', Bankfmtw);

	// This naming sucks
	// Print statistics for reads
	fmtinstall(L'Σ', Statsfmtr);
	fmtinstall(L'σ', Statsfmtw);

	// Print transaction list for reads
	fmtinstall(L'Τ', Transfmtr);
	// Only prints one transaction ↓
	fmtinstall(L'τ', Transfmtw);
	
	// Print account 
	// unused, but reserved: 'Α'
	fmtinstall(L'α', Acctfmtw);
}
