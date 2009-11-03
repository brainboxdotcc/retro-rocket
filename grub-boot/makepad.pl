my $s1 = -s 'stage1';
my $s2 = -s 'stage2';
my $num = 102400 - $s1 - $s2;
system("dd if=/dev/zero of=pad bs=1 count=$num");
system("cat  stage1 stage2 pad kernel.bin > floppy.img");
my $s3 = -s 'floppy.img';
$num = 1474560 - $s3;
system("dd if=/dev/zero of=pad bs=1 count=$num");
system("cat pad >>floppy.img");
