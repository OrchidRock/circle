//
// xhci.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2025  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _circle_usb_xhci_h
#define _circle_usb_xhci_h

#include <circle/usb/xhciconfig.h>
#include <circle/bcmpciehostbridge.h>
#include <circle/sysconfig.h>
#include <circle/macros.h>
#include <circle/types.h>
#include <assert.h>

//
// Private Completion Codes
//
#define XHCI_PRIV_COMPLETION_CODE_ERROR		-1
#define XHCI_PRIV_COMPLETION_CODE_TIMEOUT	-2

//
// Macros
//
#ifdef USE_XHCI_INTERNAL
	#define XHCI_TO_DMA(ptr)	((u64) (uintptr) (ptr))
#else
	#define XHCI_TO_DMA(ptr)	((u64) (uintptr) (ptr) \
					 | CBcmPCIeHostBridge::GetDMAAddress (PCIE_BUS_XHCI))
#endif
#define XHCI_TO_DMA_LO(ptr)		((u32) XHCI_TO_DMA (ptr))
#define XHCI_TO_DMA_HI(ptr)		((u32) (XHCI_TO_DMA (ptr) >> 32))

#define XHCI_FROM_DMA(addr)		((void *) ((uintptr) (addr) & 0xFFFFFFFFU))

#define XHCI_IS_SLOTID(num)		(1 <= (num) && (num) <= XHCI_CONFIG_MAX_SLOTS)
#define XHCI_IS_PORTID(num)		(1 <= (num) && (num) <= XHCI_CONFIG_MAX_PORTS)
#define XHCI_IS_ENDPOINTID(num)		(1 <= (num) && (num) <= XHCI_MAX_ENDPOINTS)

#define XHCI_TRB_SUCCESS(completion_code) \
	((completion_code) == XHCI_TRB_COMPLETION_CODE_SUCCESS)

#define XHCI_ALIGN_PTR(ptr, align)	((void *) (((uintptr) (ptr) + ((align)-1)) & ~((align)-1)))

//
// Capability Registers
//
#define XHCI_REG_CAP_CAPLENGTH		0x00
#define XHCI_REG_CAP_HCIVERSION		0x02
#define XHCI_REG_CAP_HCSPARAMS1		0x04
	#define XHCI_REG_CAP_HCSPARAMS1_MAX_SLOTS__MASK		0xFF
	#define XHCI_REG_CAP_HCSPARAMS1_MAX_INTRS__SHIFT	8
	#define XHCI_REG_CAP_HCSPARAMS1_MAX_INTRS__MASK		(0x7FF << 8)
	#define XHCI_REG_CAP_HCSPARAMS1_MAX_PORTS__SHIFT	24
	#define XHCI_REG_CAP_HCSPARAMS1_MAX_PORTS__MASK		(0xFF << 24)
#define XHCI_REG_CAP_HCSPARAMS2		0x08
	#define XHCI_REG_CAP_HCSPARAMS2_IST__MASK		0xF
	#define XHCI_REG_CAP_HCSPARAMS2_ERST_MAX__SHIFT		4
	#define XHCI_REG_CAP_HCSPARAMS2_ERST_MAX__MASK		(0xF << 4)
	#define XHCI_REG_CAP_HCSPARAMS2_SPR			(1 << 26)
	#define XHCI_REG_CAP_HCSPARAMS2_MAX_SCRATCHPAD_BUFS__SHIFT	27
	#define XHCI_REG_CAP_HCSPARAMS2_MAX_SCRATCHPAD_BUFS__MASK	(0x1F << 27)
#define XHCI_REG_CAP_HCSPARAMS3		0x0C
	#define XHCI_REG_CAP_HCSPARAMS3_U1_DEV_EXIT_LATENCY__MASK	0xFF
	#define XHCI_REG_CAP_HCSPARAMS3_U2_DEV_EXIT_LATENCY__SHIFT	16
	#define XHCI_REG_CAP_HCSPARAMS3_U2_DEV_EXIT_LATENCY__MASK	(0xFFFF << 16)
#define XHCI_REG_CAP_HCCPARAMS		0x10
	#define XHCI_REG_CAP_HCCPARAMS1_AC64			(1 << 0)
	#define XHCI_REG_CAP_HCCPARAMS1_BNC			(1 << 1)
	#define XHCI_REG_CAP_HCCPARAMS1_CSZ			(1 << 2)
	#define XHCI_REG_CAP_HCCPARAMS1_PPC			(1 << 3)
	#define XHCI_REG_CAP_HCCPARAMS1_PIND			(1 << 4)
	#define XHCI_REG_CAP_HCCPARAMS1_LHRC			(1 << 5)
	#define XHCI_REG_CAP_HCCPARAMS1_LTC			(1 << 6)
	#define XHCI_REG_CAP_HCCPARAMS1_NSS			(1 << 7)
	#define XHCI_REG_CAP_HCCPARAMS1_MAX_PSA_SIZE__SHIFT	12
	#define XHCI_REG_CAP_HCCPARAMS1_MAX_PSA_SIZE__MASK	(0xF << 12)
	#define XHCI_REG_CAP_HCCPARAMS1_XECP__SHIFT		16
	#define XHCI_REG_CAP_HCCPARAMS1_XECP__MASK		(0xFFFF << 16)
#define XHCI_REG_CAP_DBOFF		0x14
	#define XHCI_REG_CAP_DBOFF__MASK			0xFFFFFFFC
#define XHCI_REG_CAP_RTSOFF		0x18
	#define XHCI_REG_CAP_RTSOFF__MASK			0xFFFFFFE0

//
// Operational Registers
//
#define XHCI_REG_OP_USBCMD		0x00
	#define XHCI_REG_OP_USBCMD_RUN_STOP			(1 << 0)
	#define XHCI_REG_OP_USBCMD_HCRST			(1 << 1)
	#define XHCI_REG_OP_USBCMD_INTE				(1 << 2)
	#define XHCI_REG_OP_USBCMD_HSEE				(1 << 3)
	#define XHCI_REG_OP_USBCMD_LHCRST			(1 << 7)
	#define XHCI_REG_OP_USBCMD_CSS				(1 << 8)
	#define XHCI_REG_OP_USBCMD_CRS				(1 << 9)
	#define XHCI_REG_OP_USBCMD_EWE				(1 << 10)
	#define XHCI_REG_OP_USBCMD_EU3S				(1 << 11)
#define XHCI_REG_OP_USBSTS		0x04
	#define XHCI_REG_OP_USBSTS_HCH				(1 << 0)
	#define XHCI_REG_OP_USBSTS_HSE				(1 << 2)
	#define XHCI_REG_OP_USBSTS_EINT				(1 << 3)
	#define XHCI_REG_OP_USBSTS_PCD				(1 << 4)
	#define XHCI_REG_OP_USBSTS_SSS				(1 << 8)
	#define XHCI_REG_OP_USBSTS_RSS				(1 << 9)
	#define XHCI_REG_OP_USBSTS_SRE				(1 << 10)
	#define XHCI_REG_OP_USBSTS_CNR				(1 << 11)
	#define XHCI_REG_OP_USBSTS_HCE				(1 << 12)
#define XHCI_REG_OP_PAGESIZE		0x08
	#define XHCI_REG_OP_PAGESIZE_4K				(1 << 0)
#define XHCI_REG_OP_DNCTRL		0x14
	#define XHCI_REG_OP_DNCTRL_N0				(1 << 0)
	#define XHCI_REG_OP_DNCTRL_N1				(1 << 1)
	#define XHCI_REG_OP_DNCTRL_N2				(1 << 2)
	#define XHCI_REG_OP_DNCTRL_N3				(1 << 3)
	#define XHCI_REG_OP_DNCTRL_N4				(1 << 4)
	#define XHCI_REG_OP_DNCTRL_N5				(1 << 5)
	#define XHCI_REG_OP_DNCTRL_N6				(1 << 6)
	#define XHCI_REG_OP_DNCTRL_N7				(1 << 7)
	#define XHCI_REG_OP_DNCTRL_N8				(1 << 8)
	#define XHCI_REG_OP_DNCTRL_N9				(1 << 9)
	#define XHCI_REG_OP_DNCTRL_N10				(1 << 10)
	#define XHCI_REG_OP_DNCTRL_N11				(1 << 11)
	#define XHCI_REG_OP_DNCTRL_N12				(1 << 12)
	#define XHCI_REG_OP_DNCTRL_N13				(1 << 13)
	#define XHCI_REG_OP_DNCTRL_N14				(1 << 14)
	#define XHCI_REG_OP_DNCTRL_N15				(1 << 15)
	#define XHCI_REG_OP_DNCTRL__MASK			0xFFFF
#define XHCI_REG_OP_CRCR		0x18
	#define XHCI_REG_OP_CRCR_RCS				(1 << 0)
	#define XHCI_REG_OP_CRCR_CS				(1 << 1)
	#define XHCI_REG_OP_CRCR_CA				(1 << 2)
	#define XHCI_REG_OP_CRCR_CRR				(1 << 3)
	#define XHCI_REG_OP_CRCR_CR_PTR_LO__MASK		0xFFFFFFC0
#define XHCI_REG_OP_CRCR_HI		0x1C
#define XHCI_REG_OP_DCBAAP		0x30
	#define XHCI_REG_OP_DCBAAP__MASK			0xFFFFFFC0
#define XHCI_REG_OP_DCBAAP_HI		0x34
#define XHCI_REG_OP_CONFIG		0x38
	#define XHCI_REG_OP_CONFIG_MAX_SLOTS_EN__MASK		0xFF
#define XHCI_REG_OP_PORTS_BASE		0x400

//
// Operational Port Register Set
//
#define XHCI_REG_OP_PORTS_PORTSC	0x00
	#define XHCI_REG_OP_PORTS_PORTSC_CCS			(1 << 0)
	#define XHCI_REG_OP_PORTS_PORTSC_PED			(1 << 1)
	#define XHCI_REG_OP_PORTS_PORTSC_OCA			(1 << 3)
	#define XHCI_REG_OP_PORTS_PORTSC_PR			(1 << 4)
	#define XHCI_REG_OP_PORTS_PORTSC_PLS__SHIFT		5
	#define XHCI_REG_OP_PORTS_PORTSC_PLS__MASK		(0xF << 5)
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_U0			0
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_U1			1
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_U2			2
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_U3			3
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_DISABLED		4
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_RX_DETECT		5
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_INACTIVE		6
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_POLLING		7
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_RECOVERY		8
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_HOT_RESET		9
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_COMPLIANCE_MODE	10
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_TEST_MODE		11
		#define XHCI_REG_OP_PORTS_PORTSC_PLS_RESUME		15
	#define XHCI_REG_OP_PORTS_PORTSC_PP			(1 << 9)
	#define XHCI_REG_OP_PORTS_PORTSC_PORT_SPEED__SHIFT	10
	#define XHCI_REG_OP_PORTS_PORTSC_PORT_SPEED__MASK	(0xF << 10)
	#define XHCI_REG_OP_PORTS_PORTSC_PIC__SHIFT		14
	#define XHCI_REG_OP_PORTS_PORTSC_PIC__MASK		(3 << 14)
	#define XHCI_REG_OP_PORTS_PORTSC_LWS			(1 << 16)
	#define XHCI_REG_OP_PORTS_PORTSC_CSC			(1 << 17)
	#define XHCI_REG_OP_PORTS_PORTSC_PEC			(1 << 18)
	#define XHCI_REG_OP_PORTS_PORTSC_WRC			(1 << 19)
	#define XHCI_REG_OP_PORTS_PORTSC_OCC			(1 << 20)
	#define XHCI_REG_OP_PORTS_PORTSC_PRC			(1 << 21)
	#define XHCI_REG_OP_PORTS_PORTSC_PLC			(1 << 22)
	#define XHCI_REG_OP_PORTS_PORTSC_CEC			(1 << 23)
	#define XHCI_REG_OP_PORTS_PORTSC_CAS			(1 << 24)
	#define XHCI_REG_OP_PORTS_PORTSC_WCE			(1 << 25)
	#define XHCI_REG_OP_PORTS_PORTSC_WDE			(1 << 26)
	#define XHCI_REG_OP_PORTS_PORTSC_WOE			(1 << 27)
	#define XHCI_REG_OP_PORTS_PORTSC_DR			(1 << 30)
	#define XHCI_REG_OP_PORTS_PORTSC_WPR			(1 << 31)
#define XHCI_REG_OP_PORTS_PORTPMSC	0x04
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB3_U1_TIMEOUT__MASK	0xFF
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB3_U2_TIMEOUT__SHIFT	8
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB3_U2_TIMEOUT__MASK	(0xFF << 8)
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB3_FLA			(1 << 16)
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_L1S__MASK		7
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_RWE			(1 << 3)
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_HIRD__SHIFT		4
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_HIRD__MASK		(0xF << 4)
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_L1_DEVICE_SLOT__SHIFT	8
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_L1_DEVICE_SLOT__MASK	(0xFF << 8)
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_HLE			(1 << 16)
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_TEST_CONTROL__SHIFT	28
	#define XHCI_REG_OP_PORTS_PORTPMSC_USB2_TEST_CONTROL__MASK	(0xF << 28)
#define XHCI_REG_OP_PORTS_PORTLI	0x08
	#define XHCI_REG_OP_PORTS_PORTLI_USB3_LINK_ERROR_COUNT__MASK	0xFFFF
#define XHCI_REG_OP_PORTS_RESERVED	0x0C
#define XHCI_REG_OP_PORTS_PORT__SIZE	(4 * 4)

//
// Runtime Registers
//
#define XHCI_REG_RT_MFINDEX		0x00
	#define XHCI_REG_RT_MFINDEX_INDEX__MASK			0x3FFF
#define XHCI_REG_RT_IR0			0x20
#define XHCI_REG_RT_IR1023		0x8000

//
// Runtime Interrupter Register Set
//
#define XHCI_REG_RT_IR_IMAN		0x00
	#define XHCI_REG_RT_IR_IMAN_IP				(1 << 0)
	#define XHCI_REG_RT_IR_IMAN_IE				(1 << 1)
#define XHCI_REG_RT_IR_IMOD		0x04
	#define XHCI_REG_RT_IR_IMOD_IMODI__MASK			0xFFFF
	#define XHCI_REG_RT_IR_IMOD_IMODC__SHIFT		16
	#define XHCI_REG_RT_IR_IMOD_IMODC__MASK			(0xFFFF << 16)
#define XHCI_REG_RT_IR_ERSTSZ		0x08
	#define XHCI_REG_RT_IR_ERSTSZ__MASK			0xFFFF
#define XHCI_REG_RT_IR_ERSTBA_LO	0x10
	#define XHCI_REG_RT_IR_ERSTBA_LO__MASK			0xFFFFFFC0
#define XHCI_REG_RT_IR_ERSTBA_HI	0x14
#define XHCI_REG_RT_IR_ERDP_LO		0x18
	#define XHCI_REG_RT_IR_ERDP_LO_DESI__MASK		7
	#define XHCI_REG_RT_IR_ERDP_LO_EHB			(1 << 3)
	#define XHCI_REG_RT_IR_ERDP_LO__MASK			0xFFFFFFF0
	#define XHCI_REG_RT_IR_ERDP__MASK			(~(u64) 0xF)
#define XHCI_REG_RT_IR_ERDP_HI		0x1C
#define XHCI_REG_RT_IR__SIZE		0x20

//
// Doorbell Registers
//
#define XHCI_REG_DB_TARGET__MASK				0xFF
	#define XHCI_REG_DB_TARGET_HC_COMMAND				0
	#define XHCI_REG_DB_TARGET_EP0					1
	#define XHCI_REG_DB_TARGET_EP1_OUT				2
	#define XHCI_REG_DB_TARGET_EP1_IN				3
	#define XHCI_REG_DB_TARGET_EP15_OUT				30
	#define XHCI_REG_DB_TARGET_EP15_IN				31
#define XHCI_REG_DB_STREAM__SHIFT				16
#define XHCI_REG_DB_STREAM__MASK				(0xFFFF << 16)

#define XHCI_REG_DB_HOST_CONTROLLER				0
#define XHCI_REG_DB_DEVICE_CONTEXT1				1
#define XHCI_REG_DB_DEVICE_CONTEXT255				255
#define XHCI_REG_DB__SIZE					(4 * 256)
#define XHCI_REG_DB__SIZE_WORDS					256

//
// Transfer Request Blocks (TRB)
//
struct TXHCITRB
{
	union
	{
		struct
		{
			u32	Parameter1;
			u32	Parameter2;
		};

		u64	Parameter;
	};

	u32	Status;
	u32	Control;
	#define XHCI_TRB_CONTROL_C			(1 << 0)
	#define XHCI_TRB_CONTROL_TRB_TYPE__SHIFT	10
	#define XHCI_TRB_CONTROL_TRB_TYPE__MASK		(0x3F << 10)
}
PACKED;

// TRB Types
#define XHCI_TRB_TYPE_NORMAL			1
#define XHCI_TRB_TYPE_SETUP_STAGE		2
#define XHCI_TRB_TYPE_DATA_STAGE		3
#define XHCI_TRB_TYPE_STATUS_STAGE		4
#define XHCI_TRB_TYPE_ISOCH			5
#define XHCI_TRB_TYPE_LINK			6
#define XHCI_TRB_TYPE_EVENT_DATA		7
#define XHCI_TRB_TYPE_NO_OP			8

#define XHCI_TRB_TYPE_CMD_ENABLE_SLOT		9
#define XHCI_TRB_TYPE_CMD_DISABLE_SLOT		10
#define XHCI_TRB_TYPE_CMD_ADDRESS_DEVICE	11
#define XHCI_TRB_TYPE_CMD_CONFIGURE_ENDPOINT	12
#define XHCI_TRB_TYPE_CMD_EVALUATE_CONTEXT	13
#define XHCI_TRB_TYPE_CMD_RESET_ENDPOINT	14
#define XHCI_TRB_TYPE_CMD_STOP_ENDPOINT		15
#define XHCI_TRB_TYPE_CMD_SET_TR_DEQUEUE_PTR	16
#define XHCI_TRB_TYPE_CMD_RESET_DEVICE		17
//#define XHCI_TRB_TYPE_CMD_FORCE_EVENT		18
#define XHCI_TRB_TYPE_CMD_NEGOTIATE_BANDWIDTH	19
#define XHCI_TRB_TYPE_CMD_SET_LATENCY_TOLERANCE	20
#define XHCI_TRB_TYPE_CMD_GET_PORT_BANDWIDTH	21
#define XHCI_TRB_TYPE_CMD_FORCE_HEADER		22
#define XHCI_TRB_TYPE_CMD_NO_OP			23

#define XHCI_TRB_TYPE_EVENT_TRANSFER		32
#define XHCI_TRB_TYPE_EVENT_CMD_COMPLETION	33
#define XHCI_TRB_TYPE_EVENT_PORT_STATUS_CHANGE	34
#define XHCI_TRB_TYPE_EVENT_BANDWIDTH_REQUEST	35
//#define XHCI_TRB_TYPE_EVENT_DOORBELL		36
#define XHCI_TRB_TYPE_EVENT_HOST_CONTROLLER	37
#define XHCI_TRB_TYPE_EVENT_DEVICE_NOTIFICATION	38
#define XHCI_TRB_TYPE_EVENT_MFINDEX_WRAP	39

// TRB Completion Codes
#define XHCI_TRB_COMPLETION_CODE_SUCCESS			1
#define XHCI_TRB_COMPLETION_CODE_NO_SLOTS_AVAILABLE_ERROR	9
#define XHCI_TRB_COMPLETION_CODE_SHORT_PACKET			13
#define XHCI_TRB_COMPLETION_CODE_RING_UNDERRUN			14
#define XHCI_TRB_COMPLETION_CODE_RING_OVERRUN			15
#define XHCI_TRB_COMPLETION_CODE_EVENT_RING_FULL_ERROR		21
#define XHCI_TRB_COMPLETION_CODE_MISSED_SERVICE_ERROR		23

// Link TRB
#define XHCI_LINK_TRB_CONTROL_TC				(1 << 1)

// Event TRB
#define XHCI_EVENT_TRB_STATUS_COMPLETION_CODE__SHIFT		24
#define XHCI_EVENT_TRB_STATUS_COMPLETION_CODE__MASK		(0xFF << 24)

#define XHCI_TRANSFER_EVENT_TRB_STATUS_TRB_TRANSFER_LENGTH__MASK	0xFFFFFF
#define XHCI_TRANSFER_EVENT_TRB_CONTROL_ENDPOINTID__SHIFT		16
#define XHCI_TRANSFER_EVENT_TRB_CONTROL_ENDPOINTID__MASK		(0x1F << 16)
#define XHCI_TRANSFER_EVENT_TRB_CONTROL_SLOTID__SHIFT			24
#define XHCI_TRANSFER_EVENT_TRB_CONTROL_SLOTID__MASK			(0xFF << 24)

#define XHCI_CMD_COMPLETION_EVENT_TRB_CONTROL_SLOTID__SHIFT	24
#define XHCI_CMD_COMPLETION_EVENT_TRB_CONTROL_SLOTID__MASK	(0xFF << 24)

#define XHCI_PORT_STATUS_EVENT_TRB_PARAMETER1_PORTID__SHIFT	24
#define XHCI_PORT_STATUS_EVENT_TRB_PARAMETER1_PORTID__MASK	(0xFF << 24)

// Command TRB
#define XHCI_CMD_TRB_DISABLE_SLOT_CONTROL_SLOTID__SHIFT		24
#define XHCI_CMD_TRB_DISABLE_SLOT_CONTROL_SLOTID__MASK		(0xFF << 24)

#define XHCI_CMD_TRB_ADDRESS_DEVICE_CONTROL_BSR			(1 << 9)
#define XHCI_CMD_TRB_ADDRESS_DEVICE_CONTROL_SLOTID__SHIFT	24
#define XHCI_CMD_TRB_ADDRESS_DEVICE_CONTROL_SLOTID__MASK	(0xFF << 24)

#define XHCI_CMD_TRB_CONFIGURE_ENDPOINT_CONTROL_DC		(1 << 9)
#define XHCI_CMD_TRB_CONFIGURE_ENDPOINT_CONTROL_SLOTID__SHIFT	24
#define XHCI_CMD_TRB_CONFIGURE_ENDPOINT_CONTROL_SLOTID__MASK	(0xFF << 24)

#define XHCI_CMD_TRB_EVALUATE_CONTEXT_CONTROL_SLOTID__SHIFT	24
#define XHCI_CMD_TRB_EVALUATE_CONTEXT_CONTROL_SLOTID__MASK	(0xFF << 24)

#define XHCI_CMD_TRB_RESET_ENDPOINT_CONTROL_ENDPOINTID__SHIFT	16
#define XHCI_CMD_TRB_RESET_ENDPOINT_CONTROL_SLOTID__SHIFT	24

#define XHCI_CMD_TRB_SET_TR_DEQUEUE_PTR_CONTROL_DCS		(1 << 0)
#define XHCI_CMD_TRB_SET_TR_DEQUEUE_PTR_CONTROL_ENDPOINTID__SHIFT 16
#define XHCI_CMD_TRB_SET_TR_DEQUEUE_PTR_CONTROL_SLOTID__SHIFT	24

// Transfer TRB
#define XHCI_TRANSFER_TRB_STATUS_TD_SIZE__SHIFT			17
#define XHCI_TRANSFER_TRB_STATUS_TD_SIZE__MASK			(0x1F << 17)
#define XHCI_TRANSFER_TRB_STATUS_INTERRUPTER_TARGET__SHIFT	22
#define XHCI_TRANSFER_TRB_STATUS_INTERRUPTER_TARGET__MASK	(0x3FF << 22)
	#define XHCI_INTERRUPTER_TARGET_DEFAULT				0

#define XHCI_TRANSFER_TRB_CONTROL_TRT__SHIFT			16		// Setup stage
#define XHCI_TRANSFER_TRB_CONTROL_TRT__MASK			(3 << 16)
	#define XHCI_TRANSFER_TRB_CONTROL_TRT_NODATA			0
	#define XHCI_TRANSFER_TRB_CONTROL_TRT_OUT			2
	#define XHCI_TRANSFER_TRB_CONTROL_TRT_IN			3

#define XHCI_TRANSFER_TRB_CONTROL_ISP				(1 << 2)
#define XHCI_TRANSFER_TRB_CONTROL_IOC				(1 << 5)
#define XHCI_TRANSFER_TRB_CONTROL_IDT				(1 << 6)
#define XHCI_TRANSFER_TRB_CONTROL_DIR_IN			(1 << 16)

#define XHCI_TRANSFER_TRB_CONTROL_FRAME_ID__SHIFT		20		// Isoch TRB
#define XHCI_TRANSFER_TRB_CONTROL_FRAME_ID__MASK		(0x7FF << 20)
#define XHCI_TRANSFER_TRB_CONTROL_SIA				(1 << 31)

//
// Event Ring Segment Table Entry
//
struct TXHCIERSTEntry
{
	u64	RingSegmentBase;
	#define XHCI_ERST_ENTRY_RING_SEGMENT_BASE_LO__MASK	0xFFFFFFC0
	u32	RingSegmentSize;
	#define XHCI_ERST_ENTRY_RING_SEGMENT_SIZE__MASK		0xFFFF
	u32	Reserved;
}
PACKED;

//
// Contexts
//
struct TXHCISlotContext
{
	u32	RouteString		: 20,
		Speed			: 4,
		RsvdZ1			: 1,
		MTT			: 1,
		Hub			: 1,
		ContextEntries		: 5;

	u32	MaxExitLatency		: 16,
		RootHubPortNumber	: 8,
		NumberOfPorts		: 8;

	u32	TTHubSlotID		: 8,
		TTPortNumber		: 8,
		TTT			: 2,
		RsvdZ2			: 4,
		InterrupterTarget	: 10;

	u32	USBDeviceAddress	: 8,
		RsvdZ3			: 19,
		SlotState		: 5;

	u32	RsvdO[4];

#if XHCI_CONTEXT_SIZE == 64
	u32	RsvdO1[8];
#endif
}
PACKED;

#if XHCI_CONTEXT_SIZE == 64
ASSERT_STATIC (sizeof (TXHCISlotContext) == 0x40);
#else
ASSERT_STATIC (sizeof (TXHCISlotContext) == 0x20);
#endif

struct TXHCIEndpointContext
{
	u32	EPState			: 3,
		RsvdZ1			: 5,
		Mult			: 2,
		MaxPStreams		: 5,
		LSA			: 1,
		Interval		: 8,
		RsvdZ2			: 8;

	u32	RsvdZ3			: 1,
		CErr			: 2,
		EPType			: 3,
	#define XHCI_EP_CONTEXT_EP_TYPE_ISOCH_OUT	1
	#define XHCI_EP_CONTEXT_EP_TYPE_BULK_OUT	2
	#define XHCI_EP_CONTEXT_EP_TYPE_INTERRUPT_OUT	3
	#define XHCI_EP_CONTEXT_EP_TYPE_CONTROL		4
	#define XHCI_EP_CONTEXT_EP_TYPE_ISOCH_IN	5
	#define XHCI_EP_CONTEXT_EP_TYPE_BULK_IN		6
	#define XHCI_EP_CONTEXT_EP_TYPE_INTERRUPT_IN	7
		RsvdZ4			: 1,
		HID			: 1,
		MaxBurstSize		: 8,
		MaxPacketSize		: 16;

	u64	TRDequeuePointer;
	#define XHCI_EP_CONTEXT_TR_DEQUEUE_PTR_DCS	(1 << 0)

	u32	AverageTRBLength	: 16,
		MaxESITPayload		: 16;

	u32	RsvdO[3];

#if XHCI_CONTEXT_SIZE == 64
	u32	RsvdO1[8];
#endif
}
PACKED;

#if XHCI_CONTEXT_SIZE == 64
ASSERT_STATIC (sizeof (TXHCIEndpointContext) == 0x40);
#else
ASSERT_STATIC (sizeof (TXHCIEndpointContext) == 0x20);
#endif

struct TXHCIDeviceContext
{
	TXHCISlotContext		Slot;
#define XHCI_MAX_ENDPOINTS		31
	TXHCIEndpointContext		Endpoint[XHCI_MAX_ENDPOINTS];
}
PACKED;

#if XHCI_CONTEXT_SIZE == 64
ASSERT_STATIC (sizeof (TXHCIDeviceContext) == 0x800);
#else
ASSERT_STATIC (sizeof (TXHCIDeviceContext) == 0x400);
#endif

struct TXHCIInputControlContext
{
	u32	DropContextFlags;
	u32	AddContextFlags;
	u32	RsvdZ[6];

#if XHCI_CONTEXT_SIZE == 64
	u32	RsvdZ1[8];
#endif
}
PACKED;

#if XHCI_CONTEXT_SIZE == 64
ASSERT_STATIC (sizeof (TXHCIInputControlContext) == 0x40);
#else
ASSERT_STATIC (sizeof (TXHCIInputControlContext) == 0x20);
#endif

struct TXHCIInputContext
{
	TXHCIInputControlContext	Control;
	TXHCIDeviceContext		Device;
}
PACKED;

#if XHCI_CONTEXT_SIZE == 64
ASSERT_STATIC (sizeof (TXHCIInputContext) == 0x840);
#else
ASSERT_STATIC (sizeof (TXHCIInputContext) == 0x420);
#endif

#endif
