/* kernel/ne2k_hw.c - Tranche 5 suite: NE2000 hardware core.
 *
 * Probe/reset, PROM MAC, ring setup, TX by remote DMA and RX ring polling,
 * all through the ne2k_io_t port callbacks. Moved out of kernel/ne2k.c
 * unchanged so the same code is linked in the kernel (boot probe, degraded
 * path, reclaim after a worker loss) and in the Ring 3 networker, which owns
 * the ports 0x300-0x31F through the TSS I/O bitmap at runtime. Pure: no
 * privileged instruction, no kernel symbol.
 */
#include "ne2k.h"

static void ne2k_remote_read_setup(const ne2k_io_t* io, uint16_t base,
                                   uint16_t address, uint16_t length) {
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR0), (uint8_t)length);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR1), (uint8_t)(length >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR0), (uint8_t)address);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR1), (uint8_t)(address >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_REMOTE_READ);
}

int ne2k_rx_poll(ne2k_device_t* device, const ne2k_io_t* io,
                 uint8_t* frame, uint16_t frame_capacity,
                 uint16_t* frame_length) {
    uint16_t base, page, packet_length, payload_length;
    uint8_t header[NE2K_RX_HEADER_SIZE];
    uint8_t next_page, current_page;
    uint32_t i;
    if (!device || !io || !io->inb || !io->outb || !frame || !frame_length ||
        !device->initialized || frame_capacity == 0U || device->base_port == 0U)
        return -1;
    *frame_length = 0U;
    base = device->base_port;
    /* L’IRQ peut déjà avoir acquitté PRX : CURR/BNRY reste l’état persistant
     * du ring et évite de perdre une trame DHCP ou DNS arrivée entre deux tours. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), 0x42U);
    current_page = io->inb(io->context, (uint16_t)(base + NE2K_REG_CURR));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), 0x22U);
    page = (uint16_t)io->inb(io->context, (uint16_t)(base + NE2K_REG_BNRY)) + 1U;
    if (page >= NE2K_RX_PAGE_STOP) page = NE2K_RX_PAGE_START;
    if (current_page < NE2K_RX_PAGE_START || current_page >= NE2K_RX_PAGE_STOP)
        return -2;
    if ((uint8_t)page == current_page) return 1;
    ne2k_remote_read_setup(io, base, (uint16_t)(page << 8), NE2K_RX_HEADER_SIZE);
    for (i = 0; i < NE2K_RX_HEADER_SIZE; ++i)
        header[i] = io->inb(io->context, (uint16_t)(base + NE2K_REG_DATA));
    if ((header[0] & NE2K_RX_STATUS_OK) == 0U) return -3;
    next_page = header[1];
    packet_length = (uint16_t)(header[2] | ((uint16_t)header[3] << 8));
    if (packet_length < NE2K_RX_HEADER_SIZE || packet_length > NE2K_ETHERNET_MAX_FRAME)
        return -4;
    payload_length = (uint16_t)(packet_length - NE2K_RX_HEADER_SIZE);
    if (payload_length > frame_capacity) return -5;
    ne2k_remote_read_setup(io, base, (uint16_t)((page << 8) + NE2K_RX_HEADER_SIZE),
                           payload_length);
    for (i = 0; i < payload_length; ++i)
        frame[i] = io->inb(io->context, (uint16_t)(base + NE2K_REG_DATA));
    if (next_page < NE2K_RX_PAGE_START || next_page >= NE2K_RX_PAGE_STOP)
        return -6;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_BNRY),
             (uint8_t)(next_page == NE2K_RX_PAGE_START ? NE2K_RX_PAGE_STOP - 1U : next_page - 1U));
    *frame_length = payload_length;
    return 0;
}

int ne2k_probe(ne2k_device_t* device, uint16_t base_port, const ne2k_io_t* io) {
    uint8_t reset_value;
    uint8_t isr_value;
    if (!device || !io || !io->inb || !io->outb || base_port == 0U) return -1;
    device->base_port = base_port;
    device->initialized = 0U;
    device->mac[0] = device->mac[1] = device->mac[2] = 0U;
    device->mac[3] = device->mac[4] = device->mac[5] = 0U;
    device->mac_valid = 0U;
    io->outb(io->context, (uint16_t)(base_port + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_PAGE0);
    reset_value = io->inb(io->context, (uint16_t)(base_port + NE2K_REG_RESET));
    io->outb(io->context, (uint16_t)(base_port + NE2K_REG_RESET), reset_value);
    isr_value = io->inb(io->context, (uint16_t)(base_port + NE2K_REG_ISR));
    if ((isr_value & NE2K_ISR_RESET) == 0U)
        return -2;
    io->outb(io->context, (uint16_t)(base_port + NE2K_REG_DCR), NE2K_DCR_WORD_MODE);
    /* QEMU ne relit pas le DCR sur ce modèle; les ports flottants renvoient 0xff. */
    if (reset_value == 0xffU || isr_value == 0xffU) return -3;
    return 0;
}

int ne2k_configure_rings(ne2k_device_t* device, const ne2k_io_t* io) {
    uint16_t base;
    uint8_t index;
    if (!device || !io || !io->outb || device->base_port == 0U) return -1;
    base = device->base_port;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TPSR), 0x40U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_PSTART), 0x46U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_PSTOP), 0x60U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_BNRY), 0x46U);
    /* Broadcast DHCP et unicast strictement filtré par PAR sont acceptés. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RCR), 0x04U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TCR), 0x00U);
    /* CURR vit en page 1 : la première page RX est occupée par la frontière. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE1);
    /* PAR[0..5] partage les offsets PSTART..TBCR0 de la page 0. */
    if (device->mac_valid)
        for (index = 0U; index < 6U; ++index)
            io->outb(io->context, (uint16_t)(base + 1U + index), device->mac[index]);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_CURR), NE2K_RX_PAGE_START + 1U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), 0x22U);
    return 0;
}

int ne2k_set_mac(ne2k_device_t* device, const uint8_t mac[6]) {
    uint32_t i;
    uint8_t nonzero = 0U;
    if (!device || !mac || (mac[0] & 1U) != 0U) return -1;
    for (i = 0; i < 6U; ++i) {
        device->mac[i] = mac[i];
        if (mac[i] != 0U) nonzero = 1U;
    }
    device->mac_valid = nonzero;
    return nonzero ? 0 : -2;
}

int ne2k_read_mac(ne2k_device_t* device, const ne2k_io_t* io) {
    uint8_t prom[12];
    uint16_t base;
    uint32_t i;
    if (!device || !io || !io->inb || !io->outb || device->base_port == 0U)
        return -1;
    base = device->base_port;
    /* La PROM NE2000 expose la MAC sur les octets pairs d’une lecture DMA
     * distante de 12 octets. Lire DATA sans initialiser RSAR/RBCR récupérait
     * une position résiduelle et programmait un PAR différent de la MAC QEMU. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_DCR), NE2K_DCR_BYTE_MODE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR0), 12U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR1), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR0), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR1), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), NE2K_COMMAND_REMOTE_READ);
    for (i = 0; i < 12U; ++i)
        prom[i] = io->inb(io->context, (uint16_t)(base + NE2K_REG_DATA));
    for (i = 0; i < 6U; ++i)
        device->mac[i] = prom[i * 2U];
    device->mac_valid = 0U;
    for (i = 0; i < 6U; ++i)
        if (device->mac[i] != 0U) device->mac_valid = 1U;
    if ((device->mac[0] & 1U) != 0U || device->mac_valid == 0U) {
        device->mac_valid = 0U;
        return -2;
    }
    return 0;
}

int ne2k_tx_submit(ne2k_device_t* device, const ne2k_io_t* io,
                   const uint8_t* frame, uint16_t length) {
    uint16_t base;
    uint16_t wire_length;
    uint32_t i;
    if (!device || !io || !io->inb || !io->outb || !frame ||
        !device->initialized || device->base_port == 0U || length == 0U ||
        length > NE2K_ETHERNET_MAX_FRAME)
        return -1;
    base = device->base_port;
    wire_length = length < NE2K_ETHERNET_MIN_FRAME ? NE2K_ETHERNET_MIN_FRAME : length;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_DCR), NE2K_DCR_BYTE_MODE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR0), (uint8_t)wire_length);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RBCR1), (uint8_t)(wire_length >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR0), 0U);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_RSAR1), NE2K_TX_PAGE);
    /* Acquitter RDC avant le DMA : l’acquittement après copie effaçait le
     * signal d’achèvement avant la boucle d’attente, notamment sous QEMU. */
    io->outb(io->context, (uint16_t)(base + NE2K_REG_ISR), NE2K_ISR_RDC);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), NE2K_COMMAND_REMOTE_WRITE);
    for (i = 0; i < (uint32_t)wire_length; ++i)
        io->outb(io->context, (uint16_t)(base + NE2K_REG_DATA),
                 i < length ? frame[i] : 0U);
    for (i = 0; i < 65535U; ++i)
        if ((io->inb(io->context, (uint16_t)(base + NE2K_REG_ISR)) & NE2K_ISR_RDC) != 0U)
            break;
    if (i == 65535U) return -2;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TPSR), NE2K_TX_PAGE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TBCR0), (uint8_t)wire_length);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_TBCR1), (uint8_t)(wire_length >> 8));
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND), NE2K_COMMAND_TRANSMIT);
    return 0;
}

int ne2k_prepare(ne2k_device_t* device, const ne2k_io_t* io) {
    uint16_t base;
    if (!device || !io || !io->outb || device->base_port == 0U) return -1;
    base = device->base_port;
    io->outb(io->context, (uint16_t)(base + NE2K_REG_COMMAND),
             NE2K_COMMAND_STOP | NE2K_COMMAND_NODMA | NE2K_COMMAND_PAGE0);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_DCR), NE2K_DCR_WORD_MODE);
    io->outb(io->context, (uint16_t)(base + NE2K_REG_ISR), 0xffU);
    device->initialized = 1U;
    return 0;
}

