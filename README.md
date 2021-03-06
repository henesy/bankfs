# Central Bank

The software that powers the world. 

Exposes a bank over a 9p connection.

The default port is `tcp/3656`, which happens to be `(int)(√(1337)*100)`.

## Dependencies

None.

## Build

	mk

## Install

	mk install

## Usage

An example session:

	cpu% ./6.out
	Listening on: tcp!*!3656
	Serving on: <nil> and mounting to: /mnt/bankfs
	cpu% lc /mnt/bankfs
	bank
	cpu% cat /mnt/bankfs/bank
	cat: can't open /mnt/bankfs/bank: '/mnt/bankfs/bank' permission denied
	cpu% ll /mnt/bankfs
	---w--w--w- M 415 sys sys 0 Dec  7 01:51 bank
	cpu% 9fs tcp!10.0.2.15!3656
	post...
	cpu% lc /n
	10.0.2.15!3656/	arrietty/
	cpu% lc /n/10.0.2.15!3656/
	bank
	cpu% cd /n/10.0.2.15!3656/
	cpu% lc
	bank
	cpu% touch test
	touch: test: cannot create: 'test' mounted directory forbids creation
	cpu% netstat -n | grep -i listen
	tcp  0    glenda     Listen       564        0          ::
	tcp  1    glenda     Listen       17019      0          ::
	tcp  8    glenda     Listen       3656       0          ::
	cpu% kill 6.out
	cpu% netstat -n | grep -i listen
	tcp  0    glenda     Listen       564        0          ::
	tcp  1    glenda     Listen       17019      0          ::
	cpu% lc /mnt/bankfs
	ls: /mnt/bankfs: '/mnt/bankfs' clone failed

Once a bankfs is started and mounted somewhere, say at `/mnt/bankfs`, one can export the filesystem with authentication required as per:

	bankfs -s bankfs -u glenda -g sys
	#	chmod a+rw /srv/bankfs # should not be necessary
	aux/listen1 -tv tcp!*!3656 /bin/exportfs -a

Note that authentication won't work unless this is performed from a system with auth available. 

To mount from Linux:

	factotum -n
	9import -s bank 'tcp!bank.iseage.local!3656' /mnt/bankfs ~/n/bank

If you want to read and write from Linux and can't from the mount, do:

	echo hi | 9p write bank/ctl
	9p read bank/stats

To mount from Plan 9:

	import tcp!bank.isucdc.net!3656 /mnt/bankfs /n/bankfs

## Testing

Rudimentary tests are contained in the `tests/` directory. 

They are intended to be run as per `tests/demo.rc`.

Note: at this time, tests do not actually test and are more to make sure nothing induces a segfault.

## References

- [ndb(2)](http://man.postnix.us/9front/2/ndb)
- [9p(2)](http://man.postnix.us/9front/2/9p)
- [9pfile(2)](http://man.postnix.us/9front/2/9pfile)
- [auth(2)](http://man.postnix.us/9front/2/auth)
- [9p(5)](http://man.postnix.us/9front/5/intro)
- [simplefs](https://bitbucket.org/henesy/simplefs/src/default/)
- [asemfs](https://bitbucket.org/henesy/9intro/src/default/ch14/asemfs/)
- [ctlfs](http://contrib.9front.org/mischief/sys/src/cmd/proc/src/core/ctlfs.c)
- [lib9p/ramfs](http://mirror.postnix.us/plan9front/sys/src/lib9p/ramfs.c)

