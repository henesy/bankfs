/* constant definitions */

// This is because of stack allocations, it's not really UINT_MAX
#define	UINT_MAX	8192

#define	BUFSIZE		1024		// Maximum size of a text buffer
#define MAXTRANS	UINT_MAX-1	// Maximum number of transactions recordable within a bank
#define	MAXACCTS	MAXTRANS
#define MAXBANKS	1024
#define MAXARGS		10
extern const uint RESERVE;

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
	uint	from;	// $ from → to $
	uint	to;
	uint	amt;	// amount transferred
	long	stamp;	// timestamp of transaction
};

// Allows easy tracking of bank statistics
typedef struct Stats Stats;
struct Stats {
	uint	nbanks;		// number of banks in the db ;; do not iterate to this, use MAXBANKS and nil
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
	Stats			*stats;			// General bank statistics
	Transaction		**transactions;	// Transaction history for bank
	Account			**accounts;		// Account table for bank
};

/* Let the compiler properly check our new Fmts */
#pragma varargck type "Β" Bank*
#pragma varargck type "β" Bank*
#pragma varargck type "Σ" Stats*
#pragma varargck type "σ" Stats*
#pragma varargck type "Ω" Stats*
#pragma varargck type "Τ" Bank*
#pragma varargck type "τ" Bank*

/* horrible, horrible global variables */

extern	Tree	*banktree;	// Filesystem tree for bankfs
extern	Stats	*stats;		// Global stats, some TODO here
extern	Bank	**banks;	// Table of all bank data structures in fs
extern	File	*banksf;	// File pointer for banks root

/* function prototypes */

// Prototypes for auth routines -- deprecated(?)
void	becomenone(void);

// Prototypes for utilities
char*	readfile(Fid*);
char*	writefile(Fid*, char*);
char*	uitoa(uint);
char*	itoa(int);
void*	emalloc(ulong);
char*	bankcmd(File*, char*);
char*	mastercmd(char*);

// File cleanup routines
void	rmdir(File*);

// Prototypes for bank calls
void	initbank(File*, char*, uint, char**);
void	initacct(File*, char*, char*, char*, uint, Account*);

// Master ctl logic
void	delbank(Bank*, uint);
void	mkbank(char*);
void	trans(Bank*, uint, Bank*, uint, uint);
void	dump();

// Individual bank ctl logic
void	mkacct(File*, uint, char*);
void	delacct(File*, Bank*, uint);
void	modacct(Bank*, uint, uint, char*);
void	atrans(Bank*, uint, Bank*, uint, uint, uint, char*);
// void	dep(Bank*, uint, uint, uint, char*);

// Prototypes for cleanup functions
void	Bankdestroy(Bank*);
void	Statsdestroy(Stats*);
void	Transdestroy(Transaction*);
void	Acctdestroy(Account*);
void	Transendestroy(Transaction**);
void	Acctsdestroy(Account**);

// Prototypes for fmtinstall routines
void	bankfsfmtinstall(void);
