/****************************************************************************
**
** This file is part of the MiniAHCI project.
** Copyright (C) 2024 Alexander E. <aekhv@vk.com>
** License: GNU GPL v2, see file LICENSE.
**
****************************************************************************/

#ifndef AHCI_H
#define AHCI_H

#include <linux/kernel.h>
#include <asm/page_types.h>

#define AHCI_NUMBER_OF_PORTS_MAX	32
#define AHCI_DATA_BUFFER_SIZE_MAX	1048576

#pragma once

typedef struct _HBA_REG_CMD {
    uint32_t st : 1;		// Start
    uint32_t sud : 1;		// Spin-Up Device
    uint32_t pod : 1;		// Power On Device
    uint32_t clo : 1;		// Command List Override
    uint32_t fre : 1;		// FIS Receive Enable
    uint32_t rsvd : 3;		// Reserved
    uint32_t ccs : 5;		// Current Command Slot
    uint32_t mpss : 1;		// Mechanical Presense Switch State
    uint32_t fr : 1;		// FIS Receive Running
    uint32_t cr : 1;		// Command List Running
    uint32_t cps : 1;		// Cold Presence State
    uint32_t pma : 1;		// Port Multiplier Attached;
    uint32_t hpcp : 1;		// Hot Plug Capable Port;
    uint32_t mpsp : 1;		// Mechanical Presence Switch Attached to Port
    uint32_t cpd : 1;		// Cold Presence Detection
    uint32_t esp : 1;		// External SATA Port
    uint32_t fbscp : 1;     // FIS-based Switching Capable Port
    uint32_t apste : 1;     // Automatic Partial to Slumber Transitions Enabled
    uint32_t atapi : 1;     // Device is ATAPI
    uint32_t dlae : 1;		// Drive LED on ATAPI Enable;
    uint32_t alpe : 1;		// Aggressive Link Power Management Enable
    uint32_t asp : 1;		// Aggressive Slumber / Partial
    uint32_t icc : 4;		// Interface Communication Control
} HBA_REG_CMD;

typedef struct _HBA_REG_TFD  {
    uint32_t status : 8;	// Contains the latest copy of the task file status register
    uint32_t error : 8;     // Contains the latest copy of the task file error register
    uint32_t rsvd : 16;
} HBA_REG_TFD;

typedef struct _HBA_REG_SSTS  {
    uint32_t det : 4;		// Device detection and PHY state
    uint32_t spd : 4;		// Current interface speed
    uint32_t ipm : 4;		// Interface power management
    uint32_t rsvd : 20;
} HBA_REG_SSTS;

typedef struct _HBA_REG_SCTL {
    uint32_t det : 4;		// Device Detection Initialization
    uint32_t spd : 4;		// Speed Allowed
    uint32_t ipm : 4;		// Interface Power Management Transitions Allowed
    uint32_t spm : 4;		// Select Power Management
    uint32_t pmp : 4;		// Port Multiplier Port
    uint32_t rsvd : 12;     // Reserved
} HBA_REG_SCTL;

typedef struct _HBA_REG_SERR {
    uint32_t err : 16;		// Error
    uint32_t diag : 16;     // Diagnostics
} HBA_REG_SERR;

typedef volatile struct _HBA_PORT
{
    uint32_t clb;			// 0x00, Command List Base Address (1024-byte aligned, bits 9..0 are read only)
    uint32_t clbu;			// 0x04, Command List Base Address Upper 32-Bits
    uint32_t fb;			// 0x08, FIS Base Address (256-byte aligned, bits 7..0 are read only)
    uint32_t fbu;			// 0x0C, FIS Base Address Upper 32-Bits
    uint32_t is;			// 0x10, Interrupt Status
    uint32_t ie;			// 0x14, Interrupt Enable
    HBA_REG_CMD cmd;        // 0x18, Command and Status
    uint32_t rsvd0;         // 0x1C, Reserved
    HBA_REG_TFD tfd;        // 0x20, Task File Data
    uint32_t sig;			// 0x24, Signature
    HBA_REG_SSTS ssts;      // 0x28, Serial ATA Status (SCR0: SStatus)
    HBA_REG_SCTL sctl;      // 0x2C, Serial ATA Control (SCR2: SControl)
    HBA_REG_SERR serr;      // 0x30, Serial ATA Error (SCR1: SError)
    uint32_t sact;			// 0x34, Serial ATA Active (SCR3: SActive)
    uint32_t ci;			// 0x38, Command Issue
    uint32_t sntf;			// 0x3C, Serial ATA Notification (SCR4: SNotification)
    uint32_t fbs;			// 0x40, FIS-based Switching Control
    uint32_t devslp;		// 0x44, Device Sleep
    uint32_t rsvd1[10];     // 0x48, Reserved
    uint32_t vendor[4];     // 0x70, Vendor Specific
} HBA_PORT;

typedef struct _HBA_REG_CAP
{
    uint32_t np : 5;		// Number of Ports
    uint32_t sxs : 1;		// Supports External SATA
    uint32_t ems : 1;		// Enclosure Management Supported
    uint32_t cccs : 1;		// Command Completion Coalescing Supported
    uint32_t ncs : 5;		// Number of Command Slots
    uint32_t psc : 1;		// Partial State Capable
    uint32_t ssc : 1;		// Slumber State Capable
    uint32_t pmd : 1;		// PIO Multiple DRQ Block
    uint32_t fbss : 1;		// FIS-based Switching Supported
    uint32_t spm : 1;		// Supports Port Multiplier
    uint32_t sam : 1;		// Supports AHCI mode only
    uint32_t rsvd : 1;      // Reserved
    uint32_t iss : 4;		// Interface Speed Support
    uint32_t sclo : 1;		// Supports Command List Override
    uint32_t sal : 1;		// Supports Activity LED
    uint32_t salp : 1;		// Supports Aggressive Link Power Management
    uint32_t sss : 1;		// Supports Staggered Spin-up
    uint32_t smps : 1;		// Supports Mechanical Presence Switch
    uint32_t ssntf : 1;     // Supports SNotification Register
    uint32_t sncq : 1;		// Supports Native Command Queuing
    uint32_t s64a : 1;		// Supports 64-bit Addressing
} HBA_REG_CAP;

typedef struct _HBA_REG_GHC
{
    uint32_t hr : 1;		// HBA Reset
    uint32_t ie : 1;		// Interrupt Enable
    uint32_t mrsm : 1;		// MSI Revert to Single Message
    uint32_t rsvd : 28;     // Reserved
    uint32_t ae : 1;		// AHCI Enable
} HBA_REG_GHC;

typedef volatile struct _HBA_MEMORY
{
    HBA_REG_CAP cap;        // 0x00, Host Capabilities
    HBA_REG_GHC ghc;        // 0x04, Global Host Control
    uint32_t is;			// 0x08, Interrupt Status
    uint32_t pi;			// 0x0C, Ports Implemented
    uint32_t vs;			// 0x10, Version
    uint32_t ccc_ctl;		// 0x14, Command Completion Coalescing Control
    uint32_t ccc_ports;     // 0x18, Command Completion Coalsecing Ports
    uint32_t em_loc;		// 0x1C, Enclosure Management Location
    uint32_t em_ctl;		// 0x20, Enclosure Management Control
    uint32_t cap2;			// 0x24, Host Capabilities Extended
    uint32_t bohc;			// 0x28, BIOS/OS Handoff Control and Status
    uint32_t rsvd[13];		// 0x2C...0x5F, Reserved
    uint32_t rsvd_nvmhci[16];	// 0x60...0x9F, Reserved for NVMHCI
    uint32_t vendor[24];		// 0xA0...0xFF, Vendor Specific registers
    HBA_PORT port[AHCI_NUMBER_OF_PORTS_MAX];
} HBA_MEMORY;

typedef enum
{
    FIS_TYPE_REG_H2D = 0x27,	// Register FIS - host to device
    FIS_TYPE_REG_D2H = 0x34,	// Register FIS - device to host
    FIS_TYPE_DMA_SETUP = 0x41,	// DMA setup FIS - bidirectional
    FIS_TYPE_PIO_SETUP = 0x5F,	// PIO setup FIS - device to host
} HBA_FIS_TYPE;

typedef struct _FIS_DMA_SETUP
{
    // DWORD 0
    uint8_t  fis_type;      // FIS_TYPE_DMA_SETUP

    uint8_t  pmport : 4;	// Port multiplier
    uint8_t  rsvd0 : 1;     // Reserved
    uint8_t  d : 1;         // Data transfer direction, 1 - device to host
    uint8_t  i : 1;         // Interrupt bit
    uint8_t  a : 1;         // Auto-activate. Specifies if DMA Activate FIS is needed

    uint8_t  rsvd1[2];      // Reserved

    //DWORD 1&2

    uint32_t dma_buffer_id_lo;		// DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
    uint32_t dma_buffer_id_hi;

    //DWORD 3
    uint32_t rsvd2;                 // Reserved

    //DWORD 4
    uint32_t dma_buffer_offset;     // Byte offset into buffer. First 2 bits must be 0

    //DWORD 5
    uint32_t dma_transfer_count;	// Number of bytes to transfer. Bit 0 must be 0

    //DWORD 6
    uint32_t rsvd3;                 // Reserved

} FIS_DMA_SETUP;

typedef struct _FIS_PIO_SETUP
{
    // DWORD 0
    uint8_t  fis_type;	// FIS_TYPE_PIO_SETUP

    uint8_t  pmport : 4;// Port multiplier
    uint8_t  rsvd0 : 1;	// Reserved
    uint8_t  d : 1;		// Data transfer direction, 1 - device to host
    uint8_t  i : 1;		// Interrupt bit
    uint8_t  rsvd1 : 1;	// Reserved

    uint8_t  status;	// Status register
    uint8_t  error;		// Error register

    // DWORD 1
    uint8_t  lba0;		// LBA low register, 7:0
    uint8_t  lba1;		// LBA mid register, 15:8
    uint8_t  lba2;		// LBA high register, 23:16
    uint8_t  device;	// Device register

    // DWORD 2
    uint8_t  lba3;		// LBA register, 31:24
    uint8_t  lba4;		// LBA register, 39:32
    uint8_t  lba5;		// LBA register, 47:40
    uint8_t  rsvd2;		// Reserved

    // DWORD 3
    uint8_t  countl;	// Count register, 7:0
    uint8_t  counth;	// Count register, 15:8
    uint8_t  rsvd3;		// Reserved
    uint8_t  e_status;	// New value of status register

    // DWORD 4
    uint16_t tc;		// Transfer count
    uint16_t rsvd4;		// Reserved
} FIS_PIO_SETUP;

typedef struct _FIS_REG_D2H
{
    // DWORD 0
    uint8_t  fis_type;	// FIS_TYPE_REG_D2H

    uint8_t  pmport : 4;// Port multiplier
    uint8_t  rsvd0 : 2;	// Reserved
    uint8_t  i : 1;		// Interrupt bit
    uint8_t  rsvd1 : 1;	// Reserved

    uint8_t  status;	// Status register
    uint8_t  error;		// Error register

    // DWORD 1
    uint8_t  lba0;		// LBA low register, 7:0
    uint8_t  lba1;		// LBA mid register, 15:8
    uint8_t  lba2;		// LBA high register, 23:16
    uint8_t  device;	// Device register

    // DWORD 2
    uint8_t  lba3;		// LBA register, 31:24
    uint8_t  lba4;		// LBA register, 39:32
    uint8_t  lba5;		// LBA register, 47:40
    uint8_t  rsvd2;		// Reserved

    // DWORD 3
    uint8_t  countl;	// Count register, 7:0
    uint8_t  counth;	// Count register, 15:8
    uint8_t  rsvd3[2];	// Reserved

    // DWORD 4
    uint8_t  rsvd4[4];	// Reserved
} FIS_REG_D2H;

typedef struct _FIS_REG_H2D
{
    // DWORD 0
    uint8_t  fis_type;	// FIS_TYPE_REG_H2D

    uint8_t  pmport : 4;// Port multiplier
    uint8_t  rsvd0 : 3;	// Reserved
    uint8_t  c : 1;		// 1: Command, 0: Control

    uint8_t  command;	// Command register
    uint8_t  featuresl;	// Feature register, 7:0

    // DWORD 1
    uint8_t  lba0;		// LBA low register, 7:0
    uint8_t  lba1;		// LBA mid register, 15:8
    uint8_t  lba2;		// LBA high register, 23:16
    uint8_t  device;	// Device register

    // DWORD 2
    uint8_t  lba3;		// LBA register, 31:24
    uint8_t  lba4;		// LBA register, 39:32
    uint8_t  lba5;		// LBA register, 47:40
    uint8_t  featuresh;	// Feature register, 15:8

    // DWORD 3
    uint8_t  countl;	// Count register, 7:0
    uint8_t  counth;	// Count register, 15:8
    uint8_t  icc;		// Isochronous command completion
    uint8_t  control;	// Control register

    // DWORD 4
    uint8_t  rsvd1[4];	// Reserved
} FIS_REG_H2D;

typedef struct _HBA_RECEIVED_FIS	// sizeof() = 256 bytes
{
    FIS_DMA_SETUP dsfis;	// DMA setup FIS
    uint8_t rsvd0[4];		// Reserved

    FIS_PIO_SETUP psfis;	// PIO setup FIS
    uint8_t rsvd1[12];		// Reserved

    FIS_REG_D2H rfis;		// D2H Register FIS
    uint8_t rsvd2[4];		// Reserved

    uint8_t sdbfis[8];		// Set Device Bits FIS

    uint8_t ufis[64];		// Unknown FIS

    uint8_t rsvd3[96];		// Reserved
} HBA_RECEIVED_FIS;

typedef struct _HBA_COMMAND_HEADER
{
    uint16_t cfl : 5;	// Command FIS Length
    uint16_t a : 1;		// ATAPI
    uint16_t w : 1;		// Write
    uint16_t p : 1;		// Prefetchable
    uint16_t r : 1;		// Reset
    uint16_t b : 1;		// BIST
    uint16_t c : 1;		// Clear Busy upon R_OK
    uint16_t rsvd0 : 1;	// Reserved
    uint16_t pmp : 4;	// Port Multiplier Port

    uint16_t prdtl;		// Physical Region Descriptor Table Length
    uint32_t prdbc;		// Physical Region Descriptor Byte Count
    uint32_t ctba;		// Command Table Base Address (128-byte aligned, bits 6..0 are reserved)
    uint32_t ctbau;		// Command Table Base Address Upper 32-bits
    uint32_t rsvd1[4];	// Reserved
} HBA_COMMAND_HEADER;

typedef struct _HBA_PRDT_ENTRY
{
    uint32_t dba;		// Data Base Address
    uint32_t dbau;		// Data Base Address Upper 32-bits
    uint32_t rsvd;		// Reserved
    uint32_t dbc;		// Data Byte Count
} HBA_PRDT_ENTRY;

typedef struct _HBA_COMMAND_TABLE
{
    FIS_REG_H2D cfis;							// Command FIS
    uint8_t rsvd0[0x40 - sizeof(FIS_REG_H2D)];	// Reserved
    uint8_t acmd[0x10];							// ATAPI Command
    uint8_t rsvd1[0x30];						// Reserved
    HBA_PRDT_ENTRY prdt[(AHCI_DATA_BUFFER_SIZE_MAX / PAGE_SIZE) + 1];	// Physical Region Descriptor Table
} HBA_COMMAND_TABLE;

#endif // AHCI_H
