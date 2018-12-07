# Central Bank

The software that powers the world. 

Exposes a bank over a 9p connection.

The default port is `tcp/3656`, which happens to be `(int)(√(1337)*100)`.

## Build

	mk

## Install

	mk install

## Usage

An example session:

	cpu% mk
	6c -FTVw main.c
	6l  -o 6.out main.6
	cpu% ./6.out
	cpu% netstat -n | grep -i listen
	tcp  0    glenda     Listen       564        0          ::
	tcp  1    glenda     Listen       17019      0          ::
	tcp  4    glenda     Listen       3656       0          ::
	cpu% kill 6.out
	@{echo kill>/proc/1556/note} # 6.out
	@{echo kill>/proc/1557/note} # 6.out
	cpu% @{echo kill>/proc/1556/note} # 6.out
	cpu% @{echo kill>/proc/1557/note} # 6.out
	cpu% netstat -n | grep -i listen
	tcp  0    glenda     Listen       564        0          ::
	tcp  1    glenda     Listen       17019      0          ::
	cpu% ./6.out
	cpu% netstat -n | grep -i listen
	tcp  0    glenda     Listen       564        0          ::
	tcp  1    glenda     Listen       17019      0          ::
	tcp  4    glenda     Listen       3656       0          ::
	cpu% lc /mnt/bankfs
	ctl	log
	cpu% 9fs tcp!10.0.2.15!3656
	post...
	cpu% lc /n
	10.0.2.15!3656/	arrietty/
	cpu% lc /n/10.0.2.15!3656/
	ctl	log
	cpu% 



## References

- [9p(5)](http://man.postnix.us/9front/5/intro)
- [simplefs](https://bitbucket.org/henesy/simplefs/src/default/)
- [semfs](https://bitbucket.org/henesy/9intro/src/default/ch13/semfs/)
- [ctlfs](http://contrib.9front.org/mischief/sys/src/cmd/proc/src/core/ctlfs.c)
- [lib9p/ramfs](http://mirror.postnix.us/plan9front/sys/src/lib9p/ramfs.c)
