#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include "bankfs.h"

/* for printing over file-reads */

// Print some kind of statistics(?)
static int 
printstats(Fmt *f, Stats *s)
{
	return fmtprint(f, "naccts=%ud\n\tntrans=%ud\n", s->naccts, s->ntrans);
}

// Print individual bank statistics(?) -- what did you do moody
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

// Print a transaction
int
Transfmtr(Fmt *f)
{
	Bank *b;
	int r, i;
	b = va_arg(f->args, Bank*);
	r = fmtprint(f, "");
	for(i=0; i < b->stats->ntrans; i++){
		//TODO: Make safer
		Transaction *t = b->transactions[i];
		r += fmtprint(f, "from=%ud/%ud\n\tamount=%ud\n\tto=%ud/%ud\n\tmemo=%s\n\tstamp=%ld\n", t->n₀, t->from, t->amt, t->n₁, t->to, t->memo, t->stamp);
	}
	return r;
}

// Print master (sum) statistics
int
Masterfmtr(Fmt *f)
{
	int r;

	return r;
}

/* for printing to bankfs.ndb */

// Print 
int
Statsfmtw(Fmt *f)
{
	int r;

	return r;
}

// Print 
int
Masterfmtw(Fmt *f)
{
	int r;

	return r;
}

/* utilities */

// Install formatters for printing bank data structures -- capital for reads on files, lower for writes to ndb
void
bankfsfmtinstall(void)
{
	// TODO -- this throws type mismatch warning

	// Print master stats -- alt + *W
	fmtinstall(L'Ω', Masterfmtr);
	// For ndb -- alt + *w -- no hidden fields
	fmtinstall(L'ω', Masterfmtr);

	// This naming sucks.
	// Print bank for reads -- alt + *B
	fmtinstall(L'Β', Bankfmtr);
	// For bankfs.ndb -- alt + *b
	// fmtinstall(L'β', Bankfmtw);

	// Print statistics for reads -- alt + *S
	fmtinstall(L'Σ', Statsfmtr);
	// For ndb -- alt + *s
	fmtinstall(L'σ', Statsfmtw);

	// Print transactions for read -- alt + *T
	fmtinstall(L'Τ', Transfmtr);
	// For ndb -- alt + *t 
	// There is actually no hidden information, so we can re-use this, new name for consistency
	fmtinstall(L'τ', Transfmtr);
}
