.TL
Biggen Banken Blocken Programmer's Manual
.AU
N3st0r M4hkn0, et al.
.AI
© 1917 Communist Depository Corpus

.NH
Introduction

.PP
The bank service provided is the most advanced technology of the era, perfect for competition and integration with rival financial forces!

The banking server is running on the research operating system from Bell Labs, Plan 9 and it serves itself over the 9p protocol. 

.NH
Design

.KS
.PP
All supported operations for financial transactory services are provided via the latest technology, filesystems. The layout of the bankfs™ filesystem is as follows:

.B1
.P1
cpu% du -a /mnt/bankfs | awk '{print $2}'
/mnt/bankfs/stats
/mnt/bankfs/ctl
/mnt/bankfs/banks/0/transactions
/mnt/bankfs/banks/0/stats
/mnt/bankfs/banks/0/ctl
/mnt/bankfs/banks/0/accounts/0/name
/mnt/bankfs/banks/0/accounts/0/balance
/mnt/bankfs/banks/0/accounts/0
/mnt/bankfs/banks/0/accounts
/mnt/bankfs/banks/0
/mnt/bankfs/banks
/mnt/bankfs
.P2
.B2
.KE

.R
Where
.CW /mnt/bankfs
is the point at which the bank filesystem is mounted.

The
.CW banks/
folder represents an array of banks numbered from 0 to
.CW n
and the accounts folder within each bank represents bank
.CW n \fR's
array of accounts 0 through
.CW m \fR. 

Note that the
.CW n
value within
.CW banks/n
is equivocal to the
.I teamN
nomenclature found within the scenario. 

.KS
Queries to the
.CW bank/account
structure typically take the form of:

.B1
.P1
% (echo cat) /mnt/bankfs/bankfs/n/accounts/m/(name balance)
.P2
.B2
.KE

.KS
For example, to query the balance of account 5 on bank 6 you would do:

.B1
.P1
% cat /mnt/bankfs/banks/6/accounts/5/balance
.P2
.B2
.KE

Control over the bank filesystem is restricted to the user who started the filesystem (or one specified as such) and no others. Individual bank control (
.CW ctl
) files are modifiable by their respective representative users. 

.KS
To send commands to the master control file, perform something in the format of:

.B1
.P1
% echo 'my command in single quotes' > /mnt/bankfs/ctl
.P2
.B2
.KE

.KS
To send commands to team 3's bank control file, which can only be done by the host account and team 3's account:

.B1
.P1
% echo 'command specific to team 3' > /mnt/bankfs/banks/3/ctl
.P2
.B2
.KE

Note that any write or read operation is sufficient to satisfy commands in the above format. All that matters is that bytes are written and read. 

The bank 0 and account 0 are special. All
.CW n \fR's
≥ 1 are owned by their respective teams. All
.CW m \fR's
≥ 0 are owned by the respective bank owner in filesystem terms, though the account may technically be under the name of a person. 

In the case of the special account in the form
.CW n/m \fR,
.CW 0/0 \fR,
a reserve account is allocated as the master currency pool from which all other accounts are seeded. Excess currency cannot be generated and all account initialisations must come from the established economic pool, initialized by the
.CW 0/0
account. 

.NH 
Protocol Specification

.PP
Output format on reads is provided in the
.I ndb(2)
format.

Inputs take the form of the format where
.CW n
refers to a bank numeric ID and
.CW m
refers to a bank account numeric ID. This may be expressed in
.CW n/m
format. 

Master ctl refers to the master control file, that is, 
.I /mnt/bankfs/ctl \fR. 

Individual bank ctl refers to the individual bank control file, that is, 
.I /mnt/bankfs/banks/n/ctl \fR.

Synchronization routines are performed locally as the central bank is not aware of other entities.

Grammars will be presented with an initial general format and then an example which is possible from a shell, prefixed with a
.I % \fR.
When ellipses (…) are present, they indicate that a minimum of 1 field is required, but further may be provided. 

All writes are expected to be terminated with a newline (and a gratuitous null-terminator), echo-style. 

Fields server-side are sensitive to tabs and spaces as separators, that is all. All provided fields are required. Note that the maximum number of fields is determined by
.CW MAXARGS
and is presently set to 10.

.NH 2
Master Ctl

.NH 3
Create Bank

.PP
Create a bank owned by user. The index will be the next available ID.

.P1
mkbank user
.P2

.B1
.P1
% echo mkbank bob > /mnt/bankfs/ctl
.P2
.B2


.NH 3
Delete Bank

.PP
Delete a bank by id. The index must be one which exists and cannot be 0. When a bank is destroyed, all remaining funds in its accounts are transferred to the reserve account. 

.P1
delbank id
.P2

.B1
.P1
% echo delbank 3 > /mnt/bankfs/ctl
.P2
.B2

.NH 3
Unauthorized Transfer

.PP
Forcefully transfers funds from a managed bank account to another. 

.P1
trans n₀ from n₁ to amt
.P2

.KS
Transfer from bank 2, account 1, 45 currency to bank 4, account 4:

.B1
.P1
% echo trans 2 1 4 3 45
.P2
.B2
.KE

.NH 2
Individual Bank Ctl

.NH 3
Create Account

.PP
Create an account for the given bank. Account ID will be the next available ID. 

.P1
mkacct pin name…
.P2

.B1
.P1
% echo mkacct 1337 presi Plain Jane > /mnt/bankfs/banks/2/ctl
.P2
.B2

.NH 3
Delete Account

.PP
Delete an account for the given bank by numeric ID. When an account is deleted, all its remaining funds are returned to the reserve account.

.P1
delacct id
.P2

.B1
.P1
% echo delacct 5 > /mnt/bankfs/banks/2/ctl
.P2
.B2

.NH 3
Configure Account

.PP
Set properties for an account, this cannot be undone. Note that balance cannot be set.

.P1
modacct id pin name…
.P2

.B1
.P1
% echo modacct 13 1234 Yamborghini High > /mnt/bankfs/banks/4/ctl
.P2
.B2

.NH 3
Authorized Transfer

.PP
Authorized transfers require the pin of the from account to be provided account to be correct. The from account must be a valid ID on the current bank. 

.P1
atrans from n to amount pin memo…
.P2

.KS
Transfer $420 from bank 4's account 2 to bank 5's account 3. Note that
.CW 4/2 \fR's
PIN is 9338 and the message is\fI Paycheck for work \fR:

.B1
.P1
% echo atrans 2 5 3 420 9338 Paycheck for work > /mnt/bankfs/banks/4/ctl
.P2
.B2
.KE

.KS
.NH 3
Deposit

.PP
.B [REDACTED]
.KE

.KS
.NH
Storage

.PP
The ndb format is used for dumping the state of the central bank. The format is as per
.I ndb(2) \fR:

.B1
.P1
nbanks=1 naccts=2 ntrans=2

bankid=0
	naccts=2
	ntrans=2

acctid=0
	bank=0
	name=reserve
	balance=4999925
	pin=0

acctid=1
	bank=0
	name=seh
	balance=75
	pin=1234

transid=0
	from=0/0
	amount=100
	to=0/1
	memo=FORCED :(
	stamp=1546213215

transid=1
	from=0/1
	amount=25
	to=0/0
	memo=ducks
	stamp=1546213215

.P2
.B2
.KE

.NH
Acknowledgments

.PP
Major thanks to Mycroftiv, Mveety, and Kvik for helping out with designing and debugging the banking filesystem. Without them, we would never have cracked
.I 9pfile(2) \fR.

