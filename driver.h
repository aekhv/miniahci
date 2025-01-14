/****************************************************************************
**
** This file is part of the MiniAHCI project.
** Copyright (C) 2024 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#ifndef DRIVER_H
#define DRIVER_H

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include "ahci.h"
#include "ioctl.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Alexander E. <aekhv@vk.com>");
MODULE_DESCRIPTION("MiniAHCI kernel module");
MODULE_VERSION("1.0");

// Use "insmod miniahci.ko debug=1" to turn debug on
static bool debug = 0;
module_param(debug, bool, 0);

// PCI base address register
#define AHCI_PCI_BAR                5

// Driver version
#define AHCI_DRIVER_VERSION_MAJOR   1
#define AHCI_DRIVER_VERSION_MINOR   0
#define AHCI_DRIVER_VERSION_PATCH   0

// Default timeout in milliseconds
#define AHCI_PORT_DEFAULT_TIMEOUT   10000

typedef struct {
    HBA_PORT *pPort;
    HBA_COMMAND_HEADER *pCmdHeader; // Virtual addresses
    HBA_COMMAND_TABLE *pCmdTable;
    HBA_RECEIVED_FIS *pRcvdFis;
    dma_addr_t pCmdHeaderDma; // Physical addresses
    dma_addr_t pCmdTableDma;
    dma_addr_t pRcvdFisDma;

    uint32_t userPagesCount;
    struct page *pUserPages[(AHCI_DATA_BUFFER_SIZE_MAX / PAGE_SIZE) + 1]; // User buffer mapped pages

    uint32_t timeout;
} ahci_channel_t;

typedef struct {
    struct pci_dev *pPciDev;
    struct cdev charDevice;
    struct device *pDevice;
    HBA_MEMORY __iomem *pAhciMem;
    ahci_channel_t channel[AHCI_NUMBER_OF_PORTS_MAX];
    bool debug;
} ahci_driver_data_t;

// Base part
void ahci_controller_enable(ahci_driver_data_t *pDrvData);
void ahci_controller_disable(ahci_driver_data_t *pDrvData);
void ahci_run_ata_command(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket);
void ahci_port_software_reset(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket);
void ahci_port_hardware_reset(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket);

// IOCTL part
int device_open(struct inode *pInode, struct file *pFile);
int device_release(struct inode *pInode, struct file *pFile);
long device_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg);

#endif // DRIVER_H
