#!/usr/bin/env php
<?php
declare(strict_types=1);

/*
 * Build a raw FAT32 EFI partition image (non-root) with:
 * - Limine UEFI loader (from ./limine or LIMINE_DIR)
 * - limine.cfg copied VERBATIM from the directory containing this script
 * - kernel.bin and kernel.sym at the ESP root (BOTH required)
 *
 * Usage:
 *   php make_efi_fat32.php [SIZE_MIB] [OUTPUT_IMG]
 *
 * Env:
 *   LIMINE_DIR, BUILD_DIR, KERNEL_PATH, LIMINE_BOOT_EFI, EFI_LABEL
 */

function fail(string $message, int $code = 1): void
{
    fwrite(STDERR, "Error: {$message}\n");
    exit($code);
}

function need(string $bin): void
{
    $path = trim(shell_exec("command -v " . escapeshellarg($bin) . " 2>/dev/null") ?? '');
    if ($path === '') {
        fail("required tool '{$bin}' not found in PATH");
    }
}

function runCommand(string $cmd): void
{
    passthru($cmd, $rc);
    if ($rc !== 0) {
        fail("command failed: {$cmd} (exit {$rc})");
    }
}

function hasBinary(string $bin): bool
{
    $path = trim(shell_exec("command -v " . escapeshellarg($bin) . " 2>/dev/null") ?? '');
    return $path !== '';
}

/* ----------------------------- config ----------------------------- */

$sizeMiB = $argv[1] ?? '68';
$outImage = $argv[2] ?? './iso/efi.fat';

if (!ctype_digit((string)$sizeMiB) || (int)$sizeMiB < 8) {
    fail("SIZE_MIB must be an integer >= 8");
}
$sizeMiB = (int)$sizeMiB;

$label      = getenv('EFI_LABEL') ?: 'RETROEFI';
$limineDir  = rtrim(getenv('LIMINE_DIR') ?: './../limine', '/');
$buildDir   = rtrim(getenv('BUILD_DIR') ?: './iso', '/');
$kernelPath = getenv('KERNEL_PATH') ?: ($buildDir . '/kernel.bin');
$symbolPath = dirname($kernelPath) . '/kernel.sym';
$limineBoot = getenv('LIMINE_BOOT_EFI') ?: '';
$scriptDir  = __DIR__;

/* -------------------------- preflight ----------------------------- */

need('mformat');
need('mcopy');
need('mmd');
need('gzip');

$haveTruncate = hasBinary('truncate');
if (!$haveTruncate) {
    need('dd');
}

if ($limineBoot === '') {
    $candidate1 = $limineDir . '/EFI/BOOT/BOOTX64.EFI';
    $candidate2 = $limineDir . '/BOOTX64.EFI';
    if (is_file($candidate1)) {
        $limineBoot = $candidate1;
    } elseif (is_file($candidate2)) {
        $limineBoot = $candidate2;
    } else {
        fail("couldn't find BOOTX64.EFI in '{$limineDir}/EFI/BOOT' or '{$limineDir}'. Set LIMINE_BOOT_EFI if non-standard.");
    }
}

if (!is_file($kernelPath)) {
    fail("kernel not found at '{$kernelPath}'");
}
if (!is_file($symbolPath)) {
    fail("kernel symbols not found at '{$symbolPath}'");
}

$limineCfgSrc = $scriptDir . '/limine-hdd.conf';
if (!is_file($limineCfgSrc)) {
    fail("limine-hdd.cfg not found next to this script ('{$limineCfgSrc}'). This is fatal.");
}

/* ----------------------- create and format ------------------------ */

$bytes = $sizeMiB * 1024 * 1024;

echo ">> Creating boot FS image: {$outImage}\n";
if ($haveTruncate) {
    runCommand("truncate -s " . escapeshellarg((string)$bytes) . " " . escapeshellarg($outImage));
} else {
    runCommand("dd if=/dev/zero of=" . escapeshellarg($outImage) . " bs=1 count=0 seek=" . escapeshellarg((string)$bytes) . " status=none");
}

echo ">> Formatting FAT32 with label '{$label}'\n";
runCommand("mformat -i " . escapeshellarg($outImage) . " -F -v " . escapeshellarg($label) . " ::");

echo ">> Creating EFI directory structure\n";
runCommand("mmd -i " . escapeshellarg($outImage) . " ::/EFI");
runCommand("mmd -i " . escapeshellarg($outImage) . " ::/EFI/BOOT");

echo ">> Copying files to boot image\n";
runCommand("mcopy -i " . escapeshellarg($outImage) . " -sp " . escapeshellarg($limineBoot) . " ::/EFI/BOOT/BOOTX64.EFI");
runCommand("mcopy -i " . escapeshellarg($outImage) . " -sp " . escapeshellarg($limineCfgSrc) . " ::/limine.conf");
runCommand("mcopy -i " . escapeshellarg($outImage) . " -sp " . escapeshellarg($kernelPath) . " ::/kernel.bin");
runCommand("mcopy -i " . escapeshellarg($outImage) . " -sp " . escapeshellarg($symbolPath) . " ::/kernel.sym");

/* ----------------------- gzip in-place ---------------------------- */

echo ">> Compressing image\n";
$tmpCompressed = $outImage . '.gz.tmp';
/* -n: no timestamp/original-name -> reproducible
 * -9: max compression
 * -f: overwrite tmp if it exists
 * -c: write to stdout so we can keep the same filename
 */
runCommand(
    "sh -c " .
    escapeshellarg(
        "gzip -n -9 -f -c " .
        escapeshellarg($outImage) .
        " > " .
        escapeshellarg($tmpCompressed) .
        " && mv -f " .
        escapeshellarg($tmpCompressed) . " " . escapeshellarg($outImage)
    )
);

echo ">> Done.\n";
echo "Image ready: {$outImage}\n";
