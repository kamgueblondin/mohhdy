# Commande QEMU/KVM documentee (ASSIST-052). Pas le prototype AOS.
# Ne pas utiliser build/mohhdy.bin ni make iso (GRUB i386 Multiboot).
# Image disque : une cloud image Linux x86_64 (Ubuntu/Debian cloud).
# Seed NoCloud : agent/packaging/cloud-init/{user-data,meta-data}.
#
# Exemple, une fois CLOUD_IMAGE et SEED_ISO fournis par l'operateur :
#
# qemu-system-x86_64 \
#   -machine q35,accel=kvm:tcg \
#   -m 1024 -smp 2 \
#   -drive if=virtio,file=CLOUD_IMAGE,format=qcow2 \
#   -drive if=virtio,file=SEED_ISO,format=raw,readonly=on \
#   -netdev user,id=net0,hostfwd=tcp::8080-:8080 \
#   -device virtio-net-pci,netdev=net0 \
#   -nographic
#
# L'embed et l'admin sont alors les memes que Docker : /health /embed.js /admin.
# Voir docs/assist051_052_053_deploy.md.
