#!/bin/bash
cp "$1/kernel.bin" "$1/.iso/"
mkdir -p "$1/.iso/devices"
mkdir -p "$1/.iso/harddisk"
nm -a "$1/kernel.bin" | sort -d > "$1/.iso/kernel.sym"
xorriso -as mkisofs -b limine-cd.bin -joliet -no-emul-boot -boot-load-size 4 -boot-info-table -V "RETRO-ROCKET" --protective-msdos-label "$1/.iso" -o "$1/rr.iso" 2>/dev/null
cd "$1"
echo "qemu-system-x86_64 \
	-s \
	-cpu host \
	-trace *msi* \
	--enable-kvm \
	-monitor stdio \
	-smp 8 \
	-usb \
	-usbdevice mouse \
	-m 4096 \
	-drive id=disk,file=$2,format=raw,if=none \
	-device ahci,id=ahci \
	-device ide-hd,drive=disk,bus=ahci.0 \
	-drive file=rr.iso,media=cdrom,if=none,id=sata-cdrom \
	-device ide-cd,drive=sata-cdrom,bus=ahci.1 \
	-no-reboot \
	-no-shutdown \
	-boot d \
	-vnc 0.0.0.0:2 \
	-debugcon file:debug.log \
	-netdev user,id=netuser,hostfwd=tcp::2000-:2000 \
	-object filter-dump,id=dump,netdev=netuser,file=dump.dat \
	-device rtl8139,netdev=netuser" >run.sh
echo "gdb kernel.bin -ix $1/../.gdbargs" >debug.sh
chmod ugo+x *.sh
