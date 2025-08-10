#!/usr/bin/env php
<?php
declare(strict_types=1);

/*
 * Create a raw FAT32 EFI partition image (non-root) containing:
 * - Limine UEFI bootloader from ./limine (or override via env)
 * - kernel.bin from ./build (or override via env)
 * - optional modules from ./build/modules
 *
 * Usage:
 *   php make_efi_fat32.php [SIZE_MIB] [OUTPUT_IMG]
 * Examples:
 *   php make_efi_fat32.php
 *   php make_efi_fat32.php 68 efi_68m.img
 *
 * Env overrides:
 *   LIMINE_DIR, BUILD_DIR, KERNEL_PATH, MODULES_DIR, LIMINE_BOOT_EFI, EFI_LABEL
 */

function fail(string $msg, int $code = 1): void {
    fwrite(STDERR, "Error: {$msg}\n");
    exit($code);
}

function need(string $bin): void {
    $path = trim(shell_exec("command -v " . escapeshellarg($bin) . " 2>/dev/null") ?? '');
    if ($path === '') {
        fail("required tool '{$bin}' not found in PATH");
    }
}

function run(string $cmd): void {
    passthru($cmd, $rc);
    if ($rc !== 0) {
        fail("command failed: {$cmd} (exit {$rc})");
    }
}

function has(string $bin): bool {
    $path = trim(shell_exec("command -v " . escapeshellarg($bin) . " 2>/dev/null") ?? '');
    return $path !== '';
}

function tmpdir(): string {
    $tmp = sys_get_temp_dir() . DIRECTORY_SEPARATOR . "efi_fat32_" . bin2hex(random_bytes(6));
    if (!mkdir($tmp, 0700, true)) {
        fail("could not create temp dir: {$tmp}");
    }
    return $tmp;
}

function write_file(string $path, string $data): void {
    if (file_put_contents($path, $data) === false) {
        fail("could not write file: {$path}");
    }
}

/* ----------------------------- config ----------------------------- */

$size_mib = $argv[1] ?? '128';
$out_img  = $argv[2] ?? 'efi_fat32.img';

if (!ctype_digit((string)$size_mib) || (int)$size_mib < 8) {
    fail("SIZE_MIB must be an integer >= 8");
}
$size_mib = (int)$size_mib;

$label         = getenv('EFI_LABEL') ?: 'RETROEFI';
$limine_dir    = rtrim(getenv('LIMINE_DIR') ?: './limine', '/');
$build_dir     = rtrim(getenv('BUILD_DIR') ?: './cmake-build-debug', '/');
$kernel_path   = getenv('KERNEL_PATH') ?: ($build_dir . '/kernel.bin');
$modules_dir   = getenv('MODULES_DIR') ?: ($build_dir . '/iso/fonts');
$limine_boot   = getenv('LIMINE_BOOT_EFI') ?: '';

/* -------------------------- preflight ----------------------------- */

need('mformat');
need('mcopy');
need('mmd');

$have_truncate = has('truncate');
if (!$have_truncate) {
    need('dd');
}

if ($limine_boot === '') {
    $candidate1 = $limine_dir . '/EFI/BOOT/BOOTX64.EFI';
    $candidate2 = $limine_dir . '/BOOTX64.EFI';
    if (is_file($candidate1)) {
        $limine_boot = $candidate1;
    } elseif (is_file($candidate2)) {
        $limine_boot = $candidate2;
    } else {
        fail("couldn't find BOOTX64.EFI in '{$limine_dir}/EFI/BOOT' or '{$limine_dir}'. Set LIMINE_BOOT_EFI if non-standard.");
    }
}

if (!is_file($kernel_path)) {
    fail("kernel not found at '{$kernel_path}'");
}

/* limine.cfg source or generate minimal */
$limine_cfg_src = '';
if (is_file($limine_dir . '/limine.cfg')) {
    $limine_cfg_src = $limine_dir . '/limine.cfg';
} elseif (is_file('./limine.cfg')) {
    $limine_cfg_src = './limine.cfg';
}

/* ----------------------- create and format ------------------------ */

$bytes = $size_mib * 1024 * 1024;

echo ">> Creating {$size_mib} MiB image: {$out_img}\n";
if ($have_truncate) {
    run("truncate -s " . escapeshellarg((string)$bytes) . " " . escapeshellarg($out_img));
} else {
    run("dd if=/dev/zero of=" . escapeshellarg($out_img) . " bs=1 count=0 seek=" . escapeshellarg((string)$bytes) . " status=none");
}

echo ">> Formatting FAT32 with label '{$label}'\n";
run("mformat -i " . escapeshellarg($out_img) . " -F -v " . escapeshellarg($label) . " ::");

/* ------------------------ populate files -------------------------- */

echo ">> Creating EFI directory structure\n";
run("mmd -i " . escapeshellarg($out_img) . " ::/EFI");
run("mmd -i " . escapeshellarg($out_img) . " ::/EFI/BOOT");
run("mmd -i " . escapeshellarg($out_img) . " ::/modules || true");

/* Limine UEFI binary */
echo ">> Copying Limine UEFI loader\n";
run("mcopy -i " . escapeshellarg($out_img) . " -sp " . escapeshellarg($limine_boot) . " ::/EFI/BOOT/BOOTX64.EFI");

/* limine.cfg */
if ($limine_cfg_src !== '') {
    echo ">> Copying limine.cfg\n";
    run("mcopy -i " . escapeshellarg($out_img) . " -sp " . escapeshellarg($limine_cfg_src) . " ::/limine.cfg");
} else {
    echo ">> Generating minimal limine.cfg\n";
    $td = tmpdir();
    $cfg = <<<CFG
# Minimal limine.cfg for UEFI
TIMEOUT=3
DEFAULT_ENTRY=Retro Rocket

:Retro Rocket
PROTOCOL=linux
KERNEL_PATH=\\kernel.bin

# Example modules (uncomment and adjust):
# MODULE_PATH=\\modules\\initrd.img
# MODULE_PATH=\\modules\\foo.mod
CFG;
    $cfg_path = $td . '/limine.cfg';
    write_file($cfg_path, $cfg);
    run("mcopy -i " . escapeshellarg($out_img) . " -sp " . escapeshellarg($cfg_path) . " ::/limine.cfg");
    @unlink($cfg_path);
    @rmdir($td);
}

/* kernel.bin */
echo ">> Copying kernel to \\kernel.bin\n";
run("mcopy -i " . escapeshellarg($out_img) . " -sp " . escapeshellarg($kernel_path) . " ::/kernel.bin");

/* modules (optional) */
if (is_dir($modules_dir)) {
    echo ">> Copying modules from '{$modules_dir}' to \\modules\n";
    $glob = glob($modules_dir . '/*', GLOB_NOSORT) ?: [];
    if (!empty($glob)) {
        // mcopy -s handles recursion; quote path with wildcard via shell
        run("mcopy -i " . escapeshellarg($out_img) . " -sp " . escapeshellarg($modules_dir) . "/* ::/modules/");
    } else {
        echo ">> Modules directory is empty; skipping.\n";
    }
} else {
    echo ">> No modules directory at '{$modules_dir}' — skipping.\n";
}

/* extra EFI payloads under limine/EFI (optional) */
if (is_dir($limine_dir . '/EFI')) {
    echo ">> Staging additional EFI files from '{$limine_dir}/EFI' (if any)\n";
    $cmd = "mcopy -i " . escapeshellarg($out_img) . " -sp " . escapeshellarg($limine_dir . '/EFI') . "/* ::/EFI/";
    // Allow empty matches without failing
    run($cmd . " 2>/dev/null || true");
}

/* --------------------------- summary ------------------------------ */

echo ">> Done.\n";
echo "Image ready: {$out_img}\n";
echo "This is a raw FAT32 partition image suitable for writing into the GPT partition's sector range.\n";
echo "Ensure your GPT entry's start LBA and size match this image when writing to disk.\n";
