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

	exportfs -a -r /mnt/bankfs -e sha1 -A 'tcp!*!3656'

Note that authentication won't work unless this is performed from a system with auth available. 

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
