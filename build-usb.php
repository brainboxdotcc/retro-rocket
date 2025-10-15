<?php
declare(strict_types=1);

echo "Building USB image...\n";

define('BUILD_DIR', getcwd());
const ISO_DIR = BUILD_DIR . '/iso';
const RR_ISO = BUILD_DIR . '/rr.iso';
const KERNEL_BIN = ISO_DIR . '/kernel.bin';
const KERNEL_SYM = ISO_DIR . '/kernel.sym';
const LIMINE_DIR = BUILD_DIR . '/../limine';
const LIMINE_BIN = LIMINE_DIR . '/limine';
const LIMINE_CONF = BUILD_DIR . '/../limine-usb.conf';
const OUT_IMG = BUILD_DIR . '/usb.img';
const VOL_LABEL = 'RETROROCKET';
const HEADROOM_MB = 64;

chdir(LIMINE_DIR);
run('env -u MAKEFLAGS -u MFLAGS -u MAKELEVEL make -j1 CC="gcc -B/usr/bin/" >/dev/null');
chdir(BUILD_DIR);

if (!is_file(LIMINE_BIN) || !is_executable(LIMINE_BIN)) {
    throw new RuntimeException("limine build failed: missing or non-executable '" . LIMINE_BIN . "'");
}
function run(string $cmd, array $env = []): void {
    $proc = proc_open($cmd, [1=>['pipe','w'], 2=>['pipe','w']], $pipes, null, $env + $_ENV);
    if (!is_resource($proc)) {
        throw new RuntimeException("failed to start: $cmd");
    }
    $out = stream_get_contents($pipes[1]); fclose($pipes[1]);
    $err = stream_get_contents($pipes[2]); fclose($pipes[2]);
    $rc = proc_close($proc);
    if ($rc !== 0) {
        throw new RuntimeException("Command failed ($rc): $cmd\n$err$out");
    }
}

function ensure_file(string $path): void {
    if (!is_file($path)) {
        throw new RuntimeException("Missing file: $path");
    }
}

function align_up(int $x, int $a): int {
    return ($x + ($a - 1)) & ~($a - 1);
}

ensure_file(RR_ISO);
ensure_file(KERNEL_BIN);
ensure_file(KERNEL_SYM);
ensure_file(LIMINE_DIR . '/BOOTX64.EFI');
ensure_file(LIMINE_DIR . '/limine-bios.sys');
ensure_file(LIMINE_BIN);
ensure_file(LIMINE_CONF);

$root_gz = BUILD_DIR . '/root.iso.gz';
run(sprintf("gzip -9 -n -c %s > %s", escapeshellarg(RR_ISO), escapeshellarg($root_gz)));

$bytes_kernel = filesize(KERNEL_BIN);
$bytes_sym = filesize(KERNEL_SYM);
$bytes_cfg = filesize(LIMINE_CONF);
$bytes_rootgz = filesize($root_gz);
$bytes_overhead = (2 * 1024 * 1024) + (HEADROOM_MB * 1024 * 1024);
$partition_payload_bytes = $bytes_kernel + $bytes_sym + $bytes_cfg + $bytes_rootgz + $bytes_overhead;
$partition_size = align_up($partition_payload_bytes, 4 * 1024 * 1024);
$img_bytes = (1024 * 1024) + $partition_size;
$sector_size = 512;
$start_lba = 2048;
$offset_bytes = $start_lba * $sector_size;
$total_sectors = intdiv($img_bytes, $sector_size);
$part_sectors = $total_sectors - $start_lba;

if (is_file(OUT_IMG)) {
    unlink(OUT_IMG);
}
run(sprintf("truncate -s %d %s", $img_bytes, escapeshellarg(OUT_IMG)));

$sfdisk_spec = <<<EOT
label: dos
unit: sectors

${start_lba} ${part_sectors} ef *
EOT;

$spec_tmp = tempnam(sys_get_temp_dir(), 'sfdisk_');
file_put_contents($spec_tmp, $sfdisk_spec);
run(sprintf("sfdisk %s < %s", escapeshellarg(OUT_IMG), escapeshellarg($spec_tmp)));
@unlink($spec_tmp);
$env = ['MTOOLS_SKIP_CHECK' => '1'];
run(sprintf("mformat -i %s@@%d -F -v %s ::", escapeshellarg(OUT_IMG), $offset_bytes, escapeshellarg(VOL_LABEL)), $env);
$img_spec = escapeshellarg(OUT_IMG . '@@' . (string)$offset_bytes);
run("mmd -i $img_spec ::/EFI", $env);
run("mmd -i $img_spec ::/EFI/BOOT", $env);
run(sprintf("mcopy -i %s %s ::/EFI/BOOT/BOOTX64.EFI", $img_spec, escapeshellarg(LIMINE_DIR . '/BOOTX64.EFI')), $env);
run(sprintf("mcopy -i %s %s ::/limine-bios.sys", $img_spec, escapeshellarg(LIMINE_DIR . '/limine-bios.sys')), $env);
run(sprintf("mcopy -i %s %s ::/kernel.bin",  $img_spec, escapeshellarg(KERNEL_BIN)), $env);
run(sprintf("mcopy -i %s %s ::/kernel.sym",  $img_spec, escapeshellarg(KERNEL_SYM)), $env);
run(sprintf("mcopy -i %s %s ::/root.iso.gz", $img_spec, escapeshellarg($root_gz)),  $env);
run(sprintf("mcopy -i %s %s ::/limine.conf", $img_spec, escapeshellarg(LIMINE_CONF)), $env);
run(sprintf("%s bios-install %s", escapeshellarg(LIMINE_BIN), escapeshellarg(OUT_IMG)));

$mb = (int) round($img_bytes / (1024 * 1024));
echo "USB image created: " . OUT_IMG . " (" . $mb . " MB)\n";
