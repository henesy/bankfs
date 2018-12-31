.TL
Bank2Node Programmer's Manual
.AU
C0r13 4nd3r, et al.
.AI
© 1917 Communist Depository Corpus

.NH
Introduction

.PP
Designed to be as transparent with bankfs as possible! Your mileage may vary immensly!

.NH
Setup

.PP
This project is targeting debian.

.NH 2
Plan9port

.PP
Russ Cox's port to make 9c work in user space. We use this for the ndb and json libraries.

.B1
.P1
cd /usr/local
git clone https://GitHub.com/9fans/plan9port plan9
cd plan9 && ./INSTALL
set PLAN9=/usr/local/plan9 and PATH=$PATH:$PLAN9/bin in /etc/profile 
.P2
.B2

.NH 2
Bank2Node

.KS
.PP
To perform a complete build and setup:

.B1
.P1
cd bank2node/ndb2json && mk install 
cp bank2node/cgi-bin/read.cgi cgiserver/pages/
chmod o+x cgiserver/pages/read.cgi
.P2
.B2
.KE

.NH
Talking with bankfs

.PP
These are steps for getting the cgi script to interact with bankfs.

This is written from perspective of src folder root.

.B1
.P1
mkdir cgiserver/pages/bank
srv tcp!$bankip!$bankport bank
9pfuse `namespace`/bank cgiserver/pages/bank
.P2
.B2

Everything is now set up and good to go, api reads can be performed from
.CW /read.cgi \fR.

.NH
API

.NH 2
Read Format

.R
.CW /read.cgi
returns global stats.

.B1
.P1
{
    “nbank” : 2 ,
    “naccnt”: 20,
    “ntrans”: 200
}
.P2
.B2

.CW /read.cgi?bank=N
returns stats for bank
.CW N \fR.

.B1
.P1
{
    “naccnt”: 20,
    “ntrans” : 200,
}
.P2
.B2

.CW /read.cgi?bank=N&accnt=M
reads info from bank
.CW N
about account
.CW M \fR.

.B1
.P1
{
    “name” : chris,
    “Balance” : 200
}
.P2
.B2

.NH 2
Transactions 

.PP
These are performed with
.CW /transfer.cgi \fR.

.I GET
is assumed to be read and
.I POST
is assumed write.

.CW /transfer.cgi?bank=N
gets all transactions of bank
.CW N
which will be list of transaction objects.

.B1
.P1
{
    “fbank” : 0, #from
    “faccnt : 1,
    “amount” : 420,
    “tbank” : 1, #to
    “taccnt” : 2,
    “tstamp” : 17028377 # Unix time 
}
.P2
.B2

.CW /transfer.cgi?pin=1234
will try to make a transaction and authorize it using the pin. Pin is for faccnt at fbank. If pin is omitted unauthorized transfer is attempted.

Accepts POST object of type transaction object, timestamp is ignored but kept in object for symmetry.
