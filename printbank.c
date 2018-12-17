#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <auth.h>
#include <9p.h>
#include "bankfs.h"

static int 
printstats(Fmt *f, Stats *s)
{
	return fmtprint(f, "naccts=%ud\n\tntrans=%ud\n", s->naccts, s->ntrans);
}

int
Bankfmtr(Fmt *f)
{
	Bank *b;
	b = va_arg(f->args, Bank*);
	return printstats(f, b->stats);
}

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
		r += fmtprint(f, "from=%ud\n\tamount=%ud\n\tto=%ud\n\tstamp=%ld\n", t->from, t->amt, t->to, t->stamp);
	}
	return r;
}

// Install formatters for printing bank data structures
void
bankfsfmtinstall(void)
{
	// TODO -- this throws type mismatch warning

	// Print master stats -- alt + *W
	// fmtinstall(L'Ω', Masterfmtr);

	// Print bank for reads -- alt + *B
	fmtinstall(L'Β', Bankfmtr);
	// For bankfs.ndb -- alt + *b
	// fmtinstall(L'β', Bankfmtn);

	// Print statistics for reads -- alt + *S
	fmtinstall(L'Σ', Statsfmtr);
	// For ndb -- alt + *s
	// fmtinstall(L'σ', Transnfmt);

	// Print transactions for read -- alt + *T
	fmtinstall(L'Τ', Transfmtr);
	// For ndb -- alt + *t
	// fmtinstall(L'τ', Transfmtn);
}
