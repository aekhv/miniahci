/****************************************************************************
**
** This file is part of the MiniAHCI project.
** Copyright (C) 2024 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#ifndef IOCTL_H
#define IOCTL_H

#include <linux/kernel.h>
#include <linux/ioctl.h>
#include "minipci.h"

typedef struct {
    uint32_t pi;        // Ports implemented
} ahci_controller_info_t;

typedef struct {
    uint32_t sig;       // Attached device signature
    uint32_t det;       // Device detection and PHY state
    uint32_t spd;       // Current interface speed
} ahci_port_link_status_t;

typedef struct {
    uint8_t status;     // ATA status register
    uint8_t error;      // ATA error register
    uint8_t lba[6];
} ahci_port_ata_status_t;

typedef struct {
    uint8_t port;
    ahci_port_link_status_t link;
    ahci_port_ata_status_t ata;
} ahci_port_status_t;

typedef struct {
    uint8_t features[2];
    uint8_t count[2];
    uint8_t lba[6];
    uint8_t device;
    uint8_t command;
} ahci_ata_registers_t;

typedef struct {
    uint8_t *pointer;   // Buffer pointer
    uint32_t length;    // Buffer length
    bool write;         // Data direction: 0 - device to host (read), 1 - host to device (write)
} ahci_buffer_t;

typedef struct {
    uint8_t port;
    bool timeout;
    ahci_ata_registers_t ata;
    ahci_buffer_t buffer;
} ahci_command_packet_t;

typedef struct {
    uint8_t port;
    uint32_t value;     // Timeout in milliseconds
} ahci_port_timeout_t;

enum _NVME_IOCTL {
    _AHCI_IOCTL_GET_CONTROLLER_INFO = 0x40,
    _AHCI_IOCTL_GET_PORT_STATUS,
    _AHCI_IOCTL_RUN_ATA_COMMAND,
    _AHCI_IOCTL_SET_PORT_TIMOUT,
    _AHCI_IOCTL_GET_PORT_TIMOUT,
    _AHCI_IOCTL_PORT_SOFTWARE_RESET,
    _AHCI_IOCTL_PORT_HARDWARE_RESET
};

#define AHCI_IOCTL_GET_CONTROLLER_INFO      _IOR(MINIPCI_IOCTL_BASE, _AHCI_IOCTL_GET_CONTROLLER_INFO, ahci_controller_info_t)
#define AHCI_IOCTL_GET_PORT_STATUS          _IOWR(MINIPCI_IOCTL_BASE, _AHCI_IOCTL_GET_PORT_STATUS, ahci_port_status_t)
#define AHCI_IOCTL_RUN_ATA_COMMAND          _IOWR(MINIPCI_IOCTL_BASE, _AHCI_IOCTL_RUN_ATA_COMMAND, ahci_command_packet_t)
#define AHCI_IOCTL_SET_PORT_TIMOUT          _IOW(MINIPCI_IOCTL_BASE, _AHCI_IOCTL_SET_PORT_TIMOUT, ahci_port_timeout_t)
#define AHCI_IOCTL_GET_PORT_TIMOUT          _IOWR(MINIPCI_IOCTL_BASE, _AHCI_IOCTL_GET_PORT_TIMOUT, ahci_port_timeout_t)
#define AHCI_IOCTL_PORT_SOFTWARE_RESET      _IOWR(MINIPCI_IOCTL_BASE, _AHCI_IOCTL_PORT_SOFTWARE_RESET, ahci_command_packet_t)
#define AHCI_IOCTL_PORT_HARDWARE_RESET      _IOW(MINIPCI_IOCTL_BASE, _AHCI_IOCTL_PORT_HARDWARE_RESET, ahci_command_packet_t)

#endif // IOCTL_H
