/****************************************************************************
**
** This file is part of the MiniAHCI project.
** Copyright (C) 2024 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#include "driver.h"

int device_open(struct inode *pInode, struct file *pFile)
{
    ahci_driver_data_t *pDrvData = container_of(pInode->i_cdev, ahci_driver_data_t, charDevice);
    pFile->private_data = pDrvData;

    if ((pFile->f_flags & O_ACCMODE) != O_RDWR)
        return -EACCES;

    return 0;
}

int device_release(struct inode *pInode, struct file *pFile)
{
    (void)(pInode);
    (void)(pFile);
    return 0;
}

static int ioctl_get_driver_version(minipci_driver_version_t *pVersion)
{
    minipci_driver_version_t version;

    if (!pVersion)
        return -EINVAL;

    version.major = AHCI_DRIVER_VERSION_MAJOR;
    version.minor = AHCI_DRIVER_VERSION_MINOR;
    version.patch = AHCI_DRIVER_VERSION_PATCH;

    if (copy_to_user(pVersion, &version, sizeof(version)))
        return -EFAULT;

    return 0;
}

static int ioctl_get_pci_device_info(ahci_driver_data_t *pDrvData, minipci_device_info_t *pDevInfo)
{
    struct pci_dev *pPciDev = pDrvData->pPciDev;
    struct pci_bus *pBus = pPciDev->bus;
    struct pci_host_bridge *host = pci_find_host_bridge(pBus);
    minipci_device_info_t info;
    uint16_t linkStatus = 0;

    if (!pDevInfo)
        return -EINVAL;

    info.location.domain = (host->domain_nr == -1) ? 0 : host->domain_nr; // -1 means domain is not defined
    info.location.bus = pPciDev->bus->number;
    info.location.slot = PCI_SLOT(pPciDev->devfn);
    info.location.func = PCI_FUNC(pPciDev->devfn);

    info.id.vendorId = pPciDev->vendor;
    info.id.deviceId = pPciDev->device;
    info.id.classId = pPciDev->class >> 8;
    info.id.revision = pPciDev->revision;

    pcie_capability_read_word(pPciDev, PCI_EXP_LNKSTA, &linkStatus);
    info.link.speed = linkStatus & PCI_EXP_LNKSTA_CLS;
    info.link.width = (linkStatus & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

    if (copy_to_user(pDevInfo, &info, sizeof(info)))
        return -EFAULT;

    return 0;
}

static int ioctl_get_controller_info(ahci_driver_data_t *pDrvData, ahci_controller_info_t *pInfo)
{
    ahci_controller_info_t info;

    if (!pInfo)
        return -EINVAL;

    info.pi = pDrvData->pAhciMem->pi;

    if (copy_to_user(pInfo, &info, sizeof(info)))
        return -EFAULT;

    return 0;
}

static bool port_number_is_valid(ahci_driver_data_t *pDrvData, uint8_t port)
{
    if (port >= AHCI_NUMBER_OF_PORTS_MAX)
        return false;

    uint32_t pi = pDrvData->pAhciMem->pi & (1 << port);
    if (pi == 0)
        return false;

    return true;
}

static int ioctl_get_port_status(ahci_driver_data_t *pDrvData, ahci_port_status_t *pStatus)
{
    ahci_port_status_t status;

    if (copy_from_user(&status, pStatus, sizeof (status)))
        return -EFAULT;

    if (!port_number_is_valid(pDrvData, status.port))
        return -EINVAL;

    ahci_channel_t *pChannel = &(pDrvData->channel[status.port]);

    HBA_PORT *pPort = pChannel->pPort;
    status.link.sig = pPort->sig;
    status.link.det = pPort->ssts.det;
    status.link.spd = pPort->ssts.spd;
    status.ata.status = pPort->tfd.status;
    status.ata.error = pPort->tfd.error;

    HBA_RECEIVED_FIS *pRcvdFis = pChannel->pRcvdFis;
//    status.ata.status = pRcvdFis->rfis.status;
//    status.ata.error = pRcvdFis->rfis.error;
    status.ata.lba[0] = pRcvdFis->rfis.lba0;
    status.ata.lba[1] = pRcvdFis->rfis.lba1;
    status.ata.lba[2] = pRcvdFis->rfis.lba2;
    status.ata.lba[3] = pRcvdFis->rfis.lba3;
    status.ata.lba[4] = pRcvdFis->rfis.lba4;
    status.ata.lba[5] = pRcvdFis->rfis.lba5;

    if (copy_to_user(pStatus, &status, sizeof (status)))
        return -EFAULT;

    return 0;
}

static int ioctl_run_ata_command(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket)
{
    ahci_command_packet_t packet;

    if (copy_from_user(&packet, pCmdPacket, sizeof (packet)))
        return -EFAULT;

    if (!port_number_is_valid(pDrvData, packet.port))
        return -EINVAL;

    if (packet.buffer.length > AHCI_DATA_BUFFER_SIZE_MAX)
        return -EINVAL;

    ahci_run_ata_command(pDrvData, &packet);

    if (copy_to_user(pCmdPacket, &packet, sizeof (packet)))
        return -EFAULT;

    return 0;
}

static int ioctl_set_port_timeout(ahci_driver_data_t *pDrvData, ahci_port_timeout_t *pTimeout)
{
    ahci_port_timeout_t timeout;

    if (copy_from_user(&timeout, pTimeout, sizeof (timeout)))
        return -EFAULT;

    if (!port_number_is_valid(pDrvData, timeout.port))
        return -EINVAL;

    ahci_channel_t *pChannel = &(pDrvData->channel[timeout.port]);
    pChannel->timeout = timeout.value;

    return 0;
}

static int ioctl_get_port_timeout(ahci_driver_data_t *pDrvData, ahci_port_timeout_t *pTimeout)
{
    ahci_port_timeout_t timeout;

    if (copy_from_user(&timeout, pTimeout, sizeof (timeout)))
        return -EFAULT;

    if (!port_number_is_valid(pDrvData, timeout.port))
        return -EINVAL;

    ahci_channel_t *pChannel = &(pDrvData->channel[timeout.port]);
    timeout.value = pChannel->timeout;

    if (copy_to_user(pTimeout, &timeout, sizeof (timeout)))
        return -EFAULT;

    return 0;
}

static int ioctl_port_software_reset(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket)
{
    ahci_command_packet_t packet;

    if (copy_from_user(&packet, pCmdPacket, sizeof (packet)))
        return -EFAULT;

    if (!port_number_is_valid(pDrvData, packet.port))
        return -EINVAL;

    ahci_port_software_reset(pDrvData, &packet);

    if (copy_to_user(pCmdPacket, &packet, sizeof (packet)))
        return -EFAULT;

    return 0;
}

static int ioctl_port_hardware_reset(ahci_driver_data_t *pDrvData, ahci_command_packet_t *pCmdPacket)
{
    ahci_command_packet_t packet;

    if (copy_from_user(&packet, pCmdPacket, sizeof (packet)))
        return -EFAULT;

    if (!port_number_is_valid(pDrvData, packet.port))
        return -EINVAL;

    ahci_port_hardware_reset(pDrvData, &packet);

    return 0;
}

long device_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    ahci_driver_data_t *pDrvData = pFile->private_data;

    if (_IOC_TYPE(cmd) != MINIPCI_IOCTL_BASE)
        return -EINVAL;

    switch (cmd) {
    case MINIPCI_IOCTL_GET_DRIVER_VERSION:
        return ioctl_get_driver_version((minipci_driver_version_t *)arg);

    case MINIPCI_IOCTL_GET_DEVICE_INFO:
        return ioctl_get_pci_device_info(pDrvData, (minipci_device_info_t *)arg);

    case AHCI_IOCTL_GET_CONTROLLER_INFO:
        return ioctl_get_controller_info(pDrvData, (ahci_controller_info_t *)arg);

    case AHCI_IOCTL_GET_PORT_STATUS:
        return ioctl_get_port_status(pDrvData, (ahci_port_status_t *)arg);

    case AHCI_IOCTL_RUN_ATA_COMMAND:
        return ioctl_run_ata_command(pDrvData, (ahci_command_packet_t *)arg);

    case AHCI_IOCTL_SET_PORT_TIMOUT:
        return ioctl_set_port_timeout(pDrvData, (ahci_port_timeout_t *)arg);

    case AHCI_IOCTL_GET_PORT_TIMOUT:
        return ioctl_get_port_timeout(pDrvData, (ahci_port_timeout_t *)arg);

    case AHCI_IOCTL_PORT_SOFTWARE_RESET:
        return ioctl_port_software_reset(pDrvData, (ahci_command_packet_t *)arg);

    case AHCI_IOCTL_PORT_HARDWARE_RESET:
        return ioctl_port_hardware_reset(pDrvData, (ahci_command_packet_t *)arg);

    default:
        return -EINVAL;
    }

    return 0;
}
