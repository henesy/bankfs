/* constant definitions */

#define	BUFSIZE		1024
#define	MAXPATH		20

// File perm shortcuts
#define OREADALL	0444
#define ORDEXALL	0555

// Macros

// Test two strings are equal, shortcut
#define cmp(a, b) strcmp(a, b) == 0
// Test if top-level folder
#define pisroot(f) strcmp(f->parent->name, "/") == 0

/* type definitions */

// Allows easy tracking of bank statistics
typedef struct Stats Stats;
struct Stats {
	uint	nbanks;		// number of banks in the db
	uint	naccts;		// number of accounts in the db
	uint	ntrans;		// number of transactions since reboot
};

// Represents a Bank file
typedef struct Bankfile Bankfile;
struct Bankfile {
	// TODO
	int balance;
};

// Represents an account file
typedef struct Acctfile Acctfile;
struct Acctfile {
	// TODO
	int balance;
};

/* horrible, horrible global variables */

extern	Tree*	banktree;

/* function prototypes */

