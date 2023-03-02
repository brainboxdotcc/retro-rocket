#/usr/bin/perl

print "elf_i386_fbsd" if `uname` =~ /FreeBSD/i;
print "elf_i386" if `uname` =~ /Linux/i;
