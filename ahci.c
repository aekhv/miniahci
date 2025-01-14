/****************************************************************************
**
** This file is part of the MiniAHCI project.
** Copyright (C) 2024 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#include "driver.h"
#include <linux/delay.h>

void ahci_controller_enable(ahci_driver_data_t *pDrvData)
{
    HBA_MEMORY *pAhciMem = pDrvData->pAhciMem;
    bool debug = pDrvData->debug;
    uint32_t i, pi;

    pAhciMem->ghc.ae = 1;
    mdelay(500);

    if (debug)
        printk("%s: 64-bit address mode supported: %s\n", KBUILD_MODNAME, pAhciMem->cap.s64a ? "YES" : "NO");
    dma_set_mask(&(pDrvData->pPciDev->dev), DMA_BIT_MASK(pAhciMem->cap.s64a ? 64 : 32));
    dma_set_coherent_mask(&(pDrvData->pPciDev->dev), DMA_BIT_MASK(pAhciMem->cap.s64a ? 64 : 32));

    if (debug)
        printk("%s: Number of ports: %d\n", KBUILD_MODNAME, pAhciMem->cap.np + 1);

    pi = pAhciMem->pi;
    for (i = 0; i < AHCI_NUMBER_OF_PORTS_MAX; ++i) {
        if (pi & 1) {
            if (debug)
                printk(KERN_INFO "%s: Port %d memory allocation...\n", KBUILD_MODNAME, i);

            ahci_channel_t *pChannel = &(pDrvData->channel[i]);
            pChannel->pPort = &(pDrvData->pAhciMem->port[i]);

            pChannel->pCmdHeader = dma_alloc_coherent(&(pDrvData->pPciDev->dev), sizeof(HBA_COMMAND_HEADER), &(pChannel->pCmdHeaderDma), GFP_KERNEL);
            memset(pChannel->pCmdHeader, 0, sizeof(HBA_COMMAND_HEADER));

            pChannel->pCmdTable = dma_alloc_coherent(&(pDrvData->pPciDev->dev), sizeof(HBA_COMMAND_TABLE), &(pChannel->pCmdTableDma), GFP_KERNEL);
            memset(pChannel->pCmdTable, 0, sizeof(HBA_COMMAND_TABLE));

            pChannel->pRcvdFis = dma_alloc_coherent(&(pDrvData->pPciDev->dev), sizeof(HBA_RECEIVED_FIS), &(pChannel->pRcvdFisDma), GFP_KERNEL);
            memset(pChannel->pRcvdFis, 0, sizeof(HBA_RECEIVED_FIS));

            pChannel->pPort->clb = (uint64_t)pChannel->pCmdHeaderDma;
            pChannel->pPort->clbu = (uint64_t)pChannel->pCmdHeaderDma >> 32;

            pChannel->pCmdHeader->ctba = (uint64_t)pChannel->pCmdTableDma;
            pChannel->pCmdHeader->ctbau = (uint64_t)pChannel->pCmdTableDma >> 32;

            pChannel->pPort->fb = (uint64_t)pChannel->pRcvdFisDma;
            pChannel->pPort->fbu = (uint64_t)pChannel->pRcvdFisDma >> 32;

            // Ignition
            pChannel->pPort->cmd.fre = 1;
            pChannel->pPort->cmd.st = 1;

            pChannel->timeout = AHCI_PORT_DEFAULT_TIMEOUT;
        }
        pi >>= 1;
    }
}

void ahci_controller_disable(ahci_driver_data_t *pDrvData)
{
    HBA_MEMORY *pAhciMem = pDrvData->pAhciMem;
    bool debug = pDrvData->debug;
    uint32_t i, pi;

    pi = pAhciMem->pi;
    for (i = 0; i < AHCI_NUMBER_OF_PORTS_MAX; ++i) {
        if (pi & 1) {
            if (debug)
                printk(KERN_INFO "%s: Port %d memory free...\n", KBUILD_MODNAME, i);

            ahci_channel_t *pChannel = &(pDrvData->channel[i]);

            pChannel->pPort->cmd.fre = 0;
            pChannel->pPort->cmd.st = 0;

            while (true) {
                if (pChannel->pPort->cmd.fr)
                    continue;
                if (pChannel->pPort->cmd.cr)
                    continue;
                break;
            }

            pChannel->pPort->clb = 0;
            pChannel->pPort->clbu = 0;

            pChannel->pPort->fb = 0;
            pChannel->pPort->fbu = 0;

            if (pChannel->pRcvdFis)
                dma_free_coherent(&(pDrvData->pPciDev->dev), sizeof(HBA_RECEIVED_FIS), pChannel->pRcvdFis, pChannel->pRcvdFisDma);

            if (pChannel->pCmdTable)
                dma_free_coherent(&(pDrvData->pPciDev->dev), sizeof(HBA_COMMAND_TABLE), pChannel->pCmdTable, pChannel->pCmdTableDma);

            if (pChannel->pCmdHeader)
                dma_free_coherent(&(pDrvData->pPciDev->dev), sizeof(HBA_COMMAND_HEADER), pChannel->pCmdHeader, pChannel->pCmdHeaderDma);

        }
        pi >>= 1;
    }

    mdelay(500);
    pAhciMem->ghc.ae = 0;
}

static void ahci_map_user_pages(ahci_driver_data_t *pDrvData, uint8_t port, ahci_buffer_t *pBuffer)
{
    uint32_t i, n;
    uint32_t offs, len;
    ahci_channel_t *pChannel = &(pDrvData->channel[port]);

    const uint64_t first_page = (uint64_t)pBuffer->pointer >> PAGE_SHIFT;
    const uint64_t last_page = ((uint64_t)pBuffer->pointer + pBuffer->length - 1) >> PAGE_SHIFT;

    pChannel->userPagesCount = get_user_pages((uint64_t)pBuffer->pointer & PAGE_MASK,
                                              last_page - first_page + 1,
                                              FOLL_FORCE,
                                              pChannel->pUserPages);

    pChannel->pCmdHeader->prdtl = pChannel->userPagesCount;

    n = 0;
    for (i = 0; i < pChannel->userPagesCount; i++) {

        offs = 0;
        len = PAGE_SIZE;

        if (i == 0) {
            offs = (uint64_t)pBuffer->pointer & (PAGE_SIZE - 1);
            len -= offs;
            if (len > pBuffer->length)
                len = pBuffer->length;
        }
        else
            if (i == pChannel->userPagesCount - 1)
                len = pBuffer->length - n;

        dma_addr_t address = dma_map_page(&(pDrvData->pPciDev->dev),
                                          pChannel->pUserPages[i],
                                          offs,
                                          len,
                                          DMA_BIDIRECTIONAL);

        HBA_PRDT_ENTRY *pPRDT = pChannel->pCmdTable->prdt;
        pPRDT[i].dba = (uint64_t)address;
        pPRDT[i].dbau = (uint64_t)address >> 32;
        pPRDT[i].dbc = len - 1;

        n += len;

        if (pDrvData->debug)
            printk(KERN_INFO "%s: [+] page %d mapped (0x%016llx, %d)\n", KBUILD_MODNAME, i,
                   (uint64_t)address, len);
    }
}

static void ahci_unmap_user_pages(ahci_driver_data_t *pDrvData, uint8_t port, ahci_buffer_t *pBuffer)
{
    uint32_t i, n, len;
    ahci_channel_t *pChannel = &(pDrvData->channel[port]);
    HBA_PRDT_ENTRY *pPRDT = pChannel->pCmdTable->prdt;

    n = 0;
    for (i = 0; i < pChannel->userPagesCount; i++) {

        len = PAGE_SIZE;
        dma_addr_t address = ((uint64_t)(pPRDT[i].dbau) << 32) | pPRDT[i].dba;

        if (i == 0) {
            len -= address & (PAGE_SIZE - 1);
            if (len > pBuffer->length)
                len = pBuffer->length;
        }
        else
            if (i == pChannel->userPagesCount - 1)
                len = pBuffer->length - n;

        dma_unmap_page(&(pDrvData->pPciDev->dev),
                       address,
                       len,
                       DMA_BIDIRECTIONAL);
        n += len;

        if (pDrvData->debug)
            printk(KERN_INFO "%s: [-] page %d unmapped (0x%016llx, %d)\n", KBUILD_MODNAME, i,
                   (uint64_t)address, len);
    }

    pChannel->userPagesCount = 0;
}

void ahci_run_ata_command(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket)
{
    ahci_channel_t *pChannel = &(pDrvData->channel[pCmdPacket->port]);

    HBA_COMMAND_HEADER *pCmdHeader = pChannel->pCmdHeader;
    memset(pCmdHeader, 0, sizeof(HBA_COMMAND_HEADER) - sizeof(uint32_t) * 6); // Clear all expect command table base address
    pCmdHeader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    pCmdHeader->w = pCmdPacket->buffer.write; // Data direction!

    FIS_REG_H2D *pFis = &(pChannel->pCmdTable->cfis);
    memset(pFis, 0, sizeof(FIS_REG_H2D));
    pFis->fis_type = FIS_TYPE_REG_H2D;
    pFis->c = 1;
    pFis->featuresl = pCmdPacket->ata.features[0];
    pFis->featuresh = pCmdPacket->ata.features[1];
    pFis->countl = pCmdPacket->ata.count[0];
    pFis->counth = pCmdPacket->ata.count[1];
    pFis->lba0 = pCmdPacket->ata.lba[0];
    pFis->lba1 = pCmdPacket->ata.lba[1];
    pFis->lba2 = pCmdPacket->ata.lba[2];
    pFis->lba3 = pCmdPacket->ata.lba[3];
    pFis->lba4 = pCmdPacket->ata.lba[4];
    pFis->lba5 = pCmdPacket->ata.lba[5];
    pFis->device = pCmdPacket->ata.device;
    pFis->command = pCmdPacket->ata.command;

    if (pCmdPacket->buffer.length != 0)
        ahci_map_user_pages(pDrvData, pCmdPacket->port, &(pCmdPacket->buffer));

    // Reset all bits of the interrupt status register
    pChannel->pPort->is = 0xFFFFFFFF;

    // Remember initial value after reset
    volatile uint32_t is = pChannel->pPort->is;

    // Ignition
    pChannel->pPort->ci = 1;

    // Wait for complete...
    unsigned long future = jiffies + msecs_to_jiffies(pChannel->timeout);
    while (true) {
        // Command completed (normal deviation)
        if (pChannel->pPort->ci == 0)
            break;
        // Interrupt status changed (BAD sector occured)
        if (pChannel->pPort->is != is)
            break;
        // Timeout
        if (time_after(jiffies, future)) {
            pCmdPacket->timeout = true;
            printk(KERN_ERR "%s: Timeout detected!\n", KBUILD_MODNAME);
            break;
        }
        cpu_relax();
    }

    if (pCmdPacket->buffer.length != 0)
        ahci_unmap_user_pages(pDrvData, pCmdPacket->port, &(pCmdPacket->buffer));
}

void ahci_port_software_reset(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket)
{
    ahci_channel_t *pChannel = &(pDrvData->channel[pCmdPacket->port]);

    HBA_COMMAND_HEADER *pCmdHeader = pChannel->pCmdHeader;
    memset(pCmdHeader, 0, sizeof(HBA_COMMAND_HEADER) - sizeof(uint32_t) * 6); // Clear all expect command table base address
    pCmdHeader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);

    FIS_REG_H2D *pFis = &(pChannel->pCmdTable->cfis);
    memset(pFis, 0, sizeof(FIS_REG_H2D));
    pFis->fis_type = FIS_TYPE_REG_H2D;

    for (int i = 0; i < 2; i++) {
        if (i == 0) {
            pCmdHeader->r = 1;
            pCmdHeader->c = 1;
            pFis->c = 0;
            pFis->control = 0x04; // SRST bit is set
        } else {
            pCmdHeader->r = 0;
            pCmdHeader->c = 0;
            pFis->c = 0;
            pFis->control = 0x00; // SRST bit is clear
        }

        // Reset all bits of the interrupt status register
        pChannel->pPort->is = 0xFFFFFFFF;

        // Remember initial value after reset
        volatile uint32_t is = pChannel->pPort->is;

        // Ignition
        pChannel->pPort->ci = 1;

        // Wait for complete...
        unsigned long future = jiffies + msecs_to_jiffies(500);
        while (true) {
            // Command completed (normal deviation)
            if (pChannel->pPort->ci == 0)
                break;
            // Interrupt status changed (BAD sector occured)
            if (pChannel->pPort->is != is)
                break;
            // Timeout
            if (time_after(jiffies, future)) {
                pCmdPacket->timeout = true;
                printk(KERN_ERR "%s: Timeout detected!\n", KBUILD_MODNAME);
                break;
            }
            cpu_relax();
        }
    }
}

void ahci_port_hardware_reset(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket)
{
    ahci_channel_t *pChannel = &(pDrvData->channel[pCmdPacket->port]);

    // Disable Command List Running
    pChannel->pPort->cmd.st = 0;

    unsigned long future = jiffies + msecs_to_jiffies(pChannel->timeout);
    while (true) {
        // Command List Running disabled
        if (pChannel->pPort->cmd.cr == 0)
            break;
        // Timeout
        if (time_after(jiffies, future)) {
            printk(KERN_ERR "%s: Timeout detected!\n", KBUILD_MODNAME);
            break;
        }
    }

    // Device Detection Initialization
    pChannel->pPort->sctl.det = 1;
    mdelay(10);
    pChannel->pPort->sctl.det = 0;

    // Enable Command List Running
    pChannel->pPort->cmd.st = 1;
}
