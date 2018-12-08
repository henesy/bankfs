/* constant definitions */

#define	UINT_MAX	8192

#define	BUFSIZE		1024		// Maximum size of a text buffer
#define MAXTRANS	UINT_MAX-1	// Maximum number of transactions recordable within a bank
#define	MAXACCTS	MAXTRANS

// File perm shortcuts
#define OREADALL	0444
#define ORDEXALL	0555

// Macros

// Test two strings are equal, shortcut
#define cmp(a, b) strcmp(a, b) == 0
// Test if top-level folder
#define pisroot(f) strcmp(f->parent->name, "/") == 0

/* type definitions */

// Track bank transactions ;; from → to
typedef struct Transaction Transaction;
struct Transaction {
	uint	from;
	uint	to;
	uint	amt;
};

// Allows easy tracking of bank statistics
typedef struct Stats Stats;
struct Stats {
	uint	nbanks;		// number of banks in the db
	uint	naccts;		// number of accounts in the db
	uint	ntrans;		// number of transactions since reboot
};


// Represents an account file
typedef struct Account Account;
struct Account {
	uint	bank;		// which bank we belong to
	char*	name;		// account owner name
	int		balance;	// balance in USD of account
	uint	pin;		// PIN to access account
};

// Represents a Bank file
typedef struct Bank Bank;
struct Bank {
	Stats			*stats;
	Transaction		*transactions;
	Account			*accounts;
};

/* horrible, horrible global variables */

extern	Tree*	banktree;
extern	Stats*	stats;

/* function prototypes */

