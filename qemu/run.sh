#!/bin/bash
QEMU_PATH="/opt/ILLIXR/qemu/build/x86_64-softmmu"

# Download ubuntu-18.04.5-desktop-amd64.iso if it doesn't exist already and if there is no illixr image created
if [ ! -f ubuntu-18.04.5-desktop-amd64.iso ]; then
	if [ ! -f illixr.qcow2 ]; then
    	wget https://releases.ubuntu.com/18.04/ubuntu-18.04.5-desktop-amd64.iso
    fi
fi

# Create a qcow2 image, if one doesn't exist
if [ ! -f illixr.qcow2 ]; then
    qemu-img create -f qcow2 illixr.qcow2 30G
fi

if [ ! -f ubuntu-18.04.5-desktop-amd64.iso ]; then
	# Ubuntu image doesn't exist anymore, so launch without the CDROM option
    $QEMU_PATH/qemu-system-x86_64 -enable-kvm -M q35 -smp 2 -m 4G \
        -hda illixr.qcow2 \
        -net nic,model=virtio \
        -net user,hostfwd=tcp::2222-:22 \
        -vga virtio \
        -display sdl,gl=on
else
	# Ubuntu image exists, so launch with CDROM option (user may be installing Ubuntu or just never removed the image)
	echo "Running with CDROM"
	echo "Once you've finished installing Ubuntu, it's safe to delete ubuntu-18.04.5-desktop-amd64.iso"
	$QEMU_PATH/qemu-system-x86_64 -enable-kvm -M q35 -smp 2 -m 4G \
        -hda illixr.qcow2 \
        -net nic,model=virtio \
        -net user,hostfwd=tcp::2222-:22 \
        -vga virtio \
        -cdrom ubuntu-18.04.5-desktop-amd64.iso \
        -display sdl,gl=on
fi
