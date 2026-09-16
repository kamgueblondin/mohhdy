# Image produit Mohhdy : boot du SE Multiboot sous QEMU.
# Pas de sidecar agent/. Le bureau est le framebuffer VBE QEMU (make run-gui).
# Docker par defaut = nographic (CI). Construire : docker build -t mohhdy-os .

FROM debian:bookworm-slim AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        gcc-multilib \
        libc6-dev-i386 \
        make \
        nasm \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN make all

FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        qemu-system-x86 \
        qemu-system-gui \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /os
COPY --from=build /src/build/mohhdy.bin /os/mohhdy.bin
COPY --from=build /src/my_initrd.tar /os/my_initrd.tar
COPY --from=build /src/build/overlay.img /os/overlay.img
COPY docker/qemu-nographic.sh /os/qemu-nographic.sh
COPY docker/qemu-gui.sh /os/qemu-gui.sh
COPY scripts/qemu_gui_fit.py /os/qemu_gui_fit.py
RUN chmod 0755 /os/qemu-nographic.sh /os/qemu-gui.sh /os/qemu_gui_fit.py \
    && test -s /os/mohhdy.bin \
    && test -s /os/my_initrd.tar \
    && test -s /os/overlay.img

ENV MOHHDY_RAM=256M
STOPSIGNAL SIGTERM

# Serial stdio = console de l'instance. Pas d'EXPOSE HTTP.
# Defaut nographic (CI). Bureau HTML : make run-gui sur l'hote.
# docker/qemu-gui.sh = fallback GTK. Dans le guest : gui
CMD ["/os/qemu-nographic.sh"]
