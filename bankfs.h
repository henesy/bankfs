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

// Track bank transactions ;; from → to ;; works across banks
typedef struct Transaction Transaction;
struct Transaction {
	uint	n₀;		// from bank
	uint	n₁;		// to bank
	uint	from;	// $ from → to $
	uint	to;
	uint	amt;	// amount transferred
	long	stamp;	// timestamp of transaction
	char*	memo;	// memo for transaction
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
	char			*user;			//Owner of bank
	uint			id;			// index ID of bank
};

/* let the compiler properly check our new fmt's */

// Lowers are for writing to ndb file, capitals are for printing for users
#pragma varargck type "Β" Bank*
#pragma varargck type "β" Bank*
#pragma varargck type "Σ" Stats*
#pragma varargck type "σ" Stats*
#pragma varargck type "Ω" Stats
#pragma varargck type "ω" Stats
#pragma varargck type "Τ" Bank*
#pragma varargck type "τ" Transaction*
#pragma varargck type "Α" Account*
#pragma varargck type "α" Account*

/* horrible, horrible global variables */

extern	Tree	*banktree;	// Filesystem tree for bankfs
extern	Stats	*stats;		// Global stats
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
Stats	masterstats(void);

// File cleanup routines
void	rmdir(File*);

// Prototypes for bank calls
Bank*		initbank(char *);
Account* 	initacct(char*, uint, int, uint);
void		initbankfs(File*, int, Bank*);
void		initacctfs(File*, int, char*, Account*);

// Prototypes for NDB functions
Stats* 			readstats(Ndbtuple*);
void 			readndb(File*, char*);
Account*		readacct(Ndbtuple*);
Transaction*	readtrans(Ndbtuple*);
Bank*			readbank(Ndbtuple*);

// Master ctl logic
char*	delbank(Bank*, uint);
char*	mkbank(char*);
char*	trans(uint, uint, uint, uint, uint);
void	dump(void);

// Individual bank ctl logic
char*	mkacct(File*, Bank*, uint, char*);
char*	delacct(File*, Bank*, uint);
char*	modacct(Bank*, uint, uint*, char*);
char*	atrans(uint, uint, uint, uint, uint, uint, char*);

// Prototypes for cleanup functions
void	Bankdestroy(Bank*);
void	Statsdestroy(Stats*);
void	Transdestroy(Transaction*);
void	Acctdestroy(Account*);
void	Transendestroy(Transaction**, uint);
void	Acctsdestroy(Account**, uint);

// Prototypes for fmtinstall routines
void	bankfsfmtinstall(void);
