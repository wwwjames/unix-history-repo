/**************************************************************************

Copyright (c) 2001-2006, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

/*$FreeBSD$*/

#ifdef HAVE_KERNEL_OPTION_HEADERS
#include "opt_device_polling.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/rman.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/taskqueue.h>

#include <machine/bus.h>
#include <machine/resource.h>

#include <net/bpf.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net/if_types.h>
#include <net/if_vlan_var.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <machine/in_cksum.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/em/if_em_hw.h>
#include <dev/em/if_em.h>

/*********************************************************************
 *  Set this to one to display debug statistics
 *********************************************************************/
int	em_display_debug_stats = 0;

/*********************************************************************
 *  Driver version
 *********************************************************************/

char em_driver_version[] = "Version - 6.2.9";


/*********************************************************************
 *  PCI Device ID Table
 *
 *  Used by probe to select devices to load on
 *  Last field stores an index into em_strings
 *  Last entry must be all 0s
 *
 *  { Vendor ID, Device ID, SubVendor ID, SubDevice ID, String Index }
 *********************************************************************/

static em_vendor_info_t em_vendor_info_array[] =
{
	/* Intel(R) PRO/1000 Network Connection */
	{ 0x8086, E1000_DEV_ID_82540EM,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82540EM_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82540EP,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82540EP_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82540EP_LP,	PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82541EI,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82541ER,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82541ER_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82541EI_MOBILE,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82541GI,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82541GI_LF,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82541GI_MOBILE,	PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82542,		PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82543GC_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82543GC_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82544EI_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82544EI_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82544GC_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82544GC_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82545EM_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82545EM_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82545GM_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82545GM_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82545GM_SERDES,	PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82546EB_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546EB_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546EB_QUAD_COPPER, PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546GB_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546GB_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546GB_SERDES,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546GB_PCIE,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546GB_QUAD_COPPER, PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3,
						PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82547EI,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82547EI_MOBILE,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82547GI,		PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82571EB_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82571EB_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82571EB_SERDES,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82571EB_QUAD_COPPER,
						PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82571EB_QUAD_COPPER_LOWPROFILE,
						PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82572EI_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82572EI_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82572EI_SERDES,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82572EI,		PCI_ANY_ID, PCI_ANY_ID, 0},

	{ 0x8086, E1000_DEV_ID_82573E,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82573E_IAMT,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_82573L,		PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_80003ES2LAN_COPPER_SPT,
						PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_80003ES2LAN_SERDES_SPT,
						PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_80003ES2LAN_COPPER_DPT,
						PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_80003ES2LAN_SERDES_DPT,
						PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_ICH8_IGP_M_AMT,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_ICH8_IGP_AMT,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_ICH8_IGP_C,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_ICH8_IFE,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_ICH8_IFE_GT,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_ICH8_IFE_G,	PCI_ANY_ID, PCI_ANY_ID, 0},
	{ 0x8086, E1000_DEV_ID_ICH8_IGP_M,	PCI_ANY_ID, PCI_ANY_ID, 0},

	/* required last entry */
	{ 0, 0, 0, 0, 0}
};

/*********************************************************************
 *  Table of branding strings for all supported NICs.
 *********************************************************************/

static char *em_strings[] = {
	"Intel(R) PRO/1000 Network Connection"
};

/*********************************************************************
 *  Function prototypes
 *********************************************************************/
static int	em_probe(device_t);
static int	em_attach(device_t);
static int	em_detach(device_t);
static int	em_shutdown(device_t);
static int	em_suspend(device_t);
static int	em_resume(device_t);
static void	em_start(struct ifnet *);
static void	em_start_locked(struct ifnet *ifp);
static int	em_ioctl(struct ifnet *, u_long, caddr_t);
static void	em_watchdog(struct adapter *);
static void	em_init(void *);
static void	em_init_locked(struct adapter *);
static void	em_stop(void *);
static void	em_media_status(struct ifnet *, struct ifmediareq *);
static int	em_media_change(struct ifnet *);
static void	em_identify_hardware(struct adapter *);
static int	em_allocate_pci_resources(struct adapter *);
static int	em_allocate_intr(struct adapter *);
static void	em_free_intr(struct adapter *);
static void	em_free_pci_resources(struct adapter *);
static void	em_local_timer(void *);
static int	em_hardware_init(struct adapter *);
static void	em_setup_interface(device_t, struct adapter *);
static void	em_setup_transmit_structures(struct adapter *);
static void	em_initialize_transmit_unit(struct adapter *);
static int	em_setup_receive_structures(struct adapter *);
static void	em_initialize_receive_unit(struct adapter *);
static void	em_enable_intr(struct adapter *);
static void	em_disable_intr(struct adapter *);
static void	em_free_transmit_structures(struct adapter *);
static void	em_free_receive_structures(struct adapter *);
static void	em_update_stats_counters(struct adapter *);
static void	em_txeof(struct adapter *);
static int	em_allocate_receive_structures(struct adapter *);
static int	em_allocate_transmit_structures(struct adapter *);
static int	em_rxeof(struct adapter *, int);
#ifndef __NO_STRICT_ALIGNMENT
static int	em_fixup_rx(struct adapter *);
#endif
static void	em_receive_checksum(struct adapter *, struct em_rx_desc *,
		    struct mbuf *);
static void	em_transmit_checksum_setup(struct adapter *, struct mbuf *,
		    uint32_t *, uint32_t *);
static boolean_t em_tso_setup(struct adapter *, struct mbuf *,
		    uint32_t *, uint32_t *);
static void	em_set_promisc(struct adapter *);
static void	em_disable_promisc(struct adapter *);
static void	em_set_multi(struct adapter *);
static void	em_print_hw_stats(struct adapter *);
static void	em_update_link_status(struct adapter *);
static int	em_get_buf(struct adapter *, int);
static void	em_enable_vlans(struct adapter *);
static int	em_encap(struct adapter *, struct mbuf **);
static void	em_smartspeed(struct adapter *);
static int	em_82547_fifo_workaround(struct adapter *, int);
static void	em_82547_update_fifo_head(struct adapter *, int);
static int	em_82547_tx_fifo_reset(struct adapter *);
static void	em_82547_move_tail(void *arg);
static int	em_dma_malloc(struct adapter *, bus_size_t,
		struct em_dma_alloc *, int);
static void	em_dma_free(struct adapter *, struct em_dma_alloc *);
static void	em_print_debug_info(struct adapter *);
static int 	em_is_valid_ether_addr(uint8_t *);
static int	em_sysctl_stats(SYSCTL_HANDLER_ARGS);
static int	em_sysctl_debug_info(SYSCTL_HANDLER_ARGS);
static uint32_t	em_fill_descriptors (bus_addr_t address, uint32_t length,
		    PDESC_ARRAY desc_array);
static int	em_sysctl_int_delay(SYSCTL_HANDLER_ARGS);
static void	em_add_int_delay_sysctl(struct adapter *, const char *,
		const char *, struct em_int_delay_info *, int, int);

/*
 * Fast interrupt handler and legacy ithread/polling modes are
 * mutually exclusive.
 */
#ifdef DEVICE_POLLING
static poll_handler_t em_poll;
static void	em_intr(void *);
#else
static void	em_intr_fast(void *);
static void	em_add_int_process_limit(struct adapter *, const char *,
		const char *, int *, int);
static void	em_handle_rxtx(void *context, int pending);
static void	em_handle_link(void *context, int pending);
#endif

/*********************************************************************
 *  FreeBSD Device Interface Entry Points
 *********************************************************************/

static device_method_t em_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe, em_probe),
	DEVMETHOD(device_attach, em_attach),
	DEVMETHOD(device_detach, em_detach),
	DEVMETHOD(device_shutdown, em_shutdown),
	DEVMETHOD(device_suspend, em_suspend),
	DEVMETHOD(device_resume, em_resume),
	{0, 0}
};

static driver_t em_driver = {
	"em", em_methods, sizeof(struct adapter),
};

static devclass_t em_devclass;
DRIVER_MODULE(em, pci, em_driver, em_devclass, 0, 0);
MODULE_DEPEND(em, pci, 1, 1, 1);
MODULE_DEPEND(em, ether, 1, 1, 1);

/*********************************************************************
 *  Tunable default values.
 *********************************************************************/

#define E1000_TICKS_TO_USECS(ticks)	((1024 * (ticks) + 500) / 1000)
#define E1000_USECS_TO_TICKS(usecs)	((1000 * (usecs) + 512) / 1024)
#define M_TSO_LEN			66

static int em_tx_int_delay_dflt = E1000_TICKS_TO_USECS(EM_TIDV);
static int em_rx_int_delay_dflt = E1000_TICKS_TO_USECS(EM_RDTR);
static int em_tx_abs_int_delay_dflt = E1000_TICKS_TO_USECS(EM_TADV);
static int em_rx_abs_int_delay_dflt = E1000_TICKS_TO_USECS(EM_RADV);
static int em_rxd = EM_DEFAULT_RXD;
static int em_txd = EM_DEFAULT_TXD;
static int em_smart_pwr_down = FALSE;

TUNABLE_INT("hw.em.tx_int_delay", &em_tx_int_delay_dflt);
TUNABLE_INT("hw.em.rx_int_delay", &em_rx_int_delay_dflt);
TUNABLE_INT("hw.em.tx_abs_int_delay", &em_tx_abs_int_delay_dflt);
TUNABLE_INT("hw.em.rx_abs_int_delay", &em_rx_abs_int_delay_dflt);
TUNABLE_INT("hw.em.rxd", &em_rxd);
TUNABLE_INT("hw.em.txd", &em_txd);
TUNABLE_INT("hw.em.smart_pwr_down", &em_smart_pwr_down);
#ifndef DEVICE_POLLING
static int em_rx_process_limit = 100;
TUNABLE_INT("hw.em.rx_process_limit", &em_rx_process_limit);
#endif

/*********************************************************************
 *  Device identification routine
 *
 *  em_probe determines if the driver should be loaded on
 *  adapter based on PCI vendor/device id of the adapter.
 *
 *  return BUS_PROBE_DEFAULT on success, positive on failure
 *********************************************************************/

static int
em_probe(device_t dev)
{
	char		adapter_name[60];
	uint16_t	pci_vendor_id = 0;
	uint16_t	pci_device_id = 0;
	uint16_t	pci_subvendor_id = 0;
	uint16_t	pci_subdevice_id = 0;
	em_vendor_info_t *ent;

	INIT_DEBUGOUT("em_probe: begin");

	pci_vendor_id = pci_get_vendor(dev);
	if (pci_vendor_id != EM_VENDOR_ID)
		return (ENXIO);

	pci_device_id = pci_get_device(dev);
	pci_subvendor_id = pci_get_subvendor(dev);
	pci_subdevice_id = pci_get_subdevice(dev);

	ent = em_vendor_info_array;
	while (ent->vendor_id != 0) {
		if ((pci_vendor_id == ent->vendor_id) &&
		    (pci_device_id == ent->device_id) &&

		    ((pci_subvendor_id == ent->subvendor_id) ||
		    (ent->subvendor_id == PCI_ANY_ID)) &&

		    ((pci_subdevice_id == ent->subdevice_id) ||
		    (ent->subdevice_id == PCI_ANY_ID))) {
			sprintf(adapter_name, "%s %s",
				em_strings[ent->index],
				em_driver_version);
			device_set_desc_copy(dev, adapter_name);
			return (BUS_PROBE_DEFAULT);
		}
		ent++;
	}

	return (ENXIO);
}

/*********************************************************************
 *  Device initialization routine
 *
 *  The attach entry point is called when the driver is being loaded.
 *  This routine identifies the type of hardware, allocates all resources
 *  and initializes the hardware.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/

static int
em_attach(device_t dev)
{
	struct adapter	*adapter;
	int		tsize, rsize;
	int		error = 0;

	INIT_DEBUGOUT("em_attach: begin");

	adapter = device_get_softc(dev);
	adapter->dev = adapter->osdep.dev = dev;
	EM_LOCK_INIT(adapter, device_get_nameunit(dev));

	/* SYSCTL stuff */
	SYSCTL_ADD_PROC(device_get_sysctl_ctx(dev),
	    SYSCTL_CHILDREN(device_get_sysctl_tree(dev)),
	    OID_AUTO, "debug_info", CTLTYPE_INT|CTLFLAG_RW, adapter, 0,
	    em_sysctl_debug_info, "I", "Debug Information");

	SYSCTL_ADD_PROC(device_get_sysctl_ctx(dev),
	    SYSCTL_CHILDREN(device_get_sysctl_tree(dev)),
	    OID_AUTO, "stats", CTLTYPE_INT|CTLFLAG_RW, adapter, 0,
	    em_sysctl_stats, "I", "Statistics");

	callout_init_mtx(&adapter->timer, &adapter->mtx, 0);
	callout_init_mtx(&adapter->tx_fifo_timer, &adapter->mtx, 0);

	/* Determine hardware revision */
	em_identify_hardware(adapter);

	/* Set up some sysctls for the tunable interrupt delays */
	em_add_int_delay_sysctl(adapter, "rx_int_delay",
	    "receive interrupt delay in usecs", &adapter->rx_int_delay,
	    E1000_REG_OFFSET(&adapter->hw, RDTR), em_rx_int_delay_dflt);
	em_add_int_delay_sysctl(adapter, "tx_int_delay",
	    "transmit interrupt delay in usecs", &adapter->tx_int_delay,
	    E1000_REG_OFFSET(&adapter->hw, TIDV), em_tx_int_delay_dflt);
	if (adapter->hw.mac_type >= em_82540) {
		em_add_int_delay_sysctl(adapter, "rx_abs_int_delay",
		    "receive interrupt delay limit in usecs",
		    &adapter->rx_abs_int_delay,
		    E1000_REG_OFFSET(&adapter->hw, RADV),
		    em_rx_abs_int_delay_dflt);
		em_add_int_delay_sysctl(adapter, "tx_abs_int_delay",
		    "transmit interrupt delay limit in usecs",
		    &adapter->tx_abs_int_delay,
		    E1000_REG_OFFSET(&adapter->hw, TADV),
		    em_tx_abs_int_delay_dflt);
	}

#ifndef DEVICE_POLLING
	/* Sysctls for limiting the amount of work done in the taskqueue */
	em_add_int_process_limit(adapter, "rx_processing_limit",
	    "max number of rx packets to process", &adapter->rx_process_limit,
	    em_rx_process_limit);
#endif

	/*
	 * Validate number of transmit and receive descriptors. It
	 * must not exceed hardware maximum, and must be multiple
	 * of EM_DBA_ALIGN.
	 */
	if (((em_txd * sizeof(struct em_tx_desc)) % EM_DBA_ALIGN) != 0 ||
	    (adapter->hw.mac_type >= em_82544 && em_txd > EM_MAX_TXD) ||
	    (adapter->hw.mac_type < em_82544 && em_txd > EM_MAX_TXD_82543) ||
	    (em_txd < EM_MIN_TXD)) {
		device_printf(dev, "Using %d TX descriptors instead of %d!\n",
		    EM_DEFAULT_TXD, em_txd);
		adapter->num_tx_desc = EM_DEFAULT_TXD;
	} else
		adapter->num_tx_desc = em_txd;
	if (((em_rxd * sizeof(struct em_rx_desc)) % EM_DBA_ALIGN) != 0 ||
	    (adapter->hw.mac_type >= em_82544 && em_rxd > EM_MAX_RXD) ||
	    (adapter->hw.mac_type < em_82544 && em_rxd > EM_MAX_RXD_82543) ||
	    (em_rxd < EM_MIN_RXD)) {
		device_printf(dev, "Using %d RX descriptors instead of %d!\n",
		    EM_DEFAULT_RXD, em_rxd);
		adapter->num_rx_desc = EM_DEFAULT_RXD;
	} else
		adapter->num_rx_desc = em_rxd;

	adapter->hw.autoneg = DO_AUTO_NEG;
	adapter->hw.wait_autoneg_complete = WAIT_FOR_AUTO_NEG_DEFAULT;
	adapter->hw.autoneg_advertised = AUTONEG_ADV_DEFAULT;
	adapter->hw.tbi_compatibility_en = TRUE;
	adapter->rx_buffer_len = EM_RXBUFFER_2048;

	adapter->hw.phy_init_script = 1;
	adapter->hw.phy_reset_disable = FALSE;

#ifndef EM_MASTER_SLAVE
	adapter->hw.master_slave = em_ms_hw_default;
#else
	adapter->hw.master_slave = EM_MASTER_SLAVE;
#endif
	/*
	 * Set the max frame size assuming standard ethernet
	 * sized frames.
	 */
	adapter->hw.max_frame_size = ETHERMTU + ETHER_HDR_LEN + ETHER_CRC_LEN;

	adapter->hw.min_frame_size = MINIMUM_ETHERNET_PACKET_SIZE + ETHER_CRC_LEN;

	/*
	 * This controls when hardware reports transmit completion
	 * status.
	 */
	adapter->hw.report_tx_early = 1;
	if (em_allocate_pci_resources(adapter)) {
		device_printf(dev, "Allocation of PCI resources failed\n");
		error = ENXIO;
		goto err_pci;
	}
	
	/* Initialize eeprom parameters */
	em_init_eeprom_params(&adapter->hw);

	tsize = roundup2(adapter->num_tx_desc * sizeof(struct em_tx_desc),
	    EM_DBA_ALIGN);

	/* Allocate Transmit Descriptor ring */
	if (em_dma_malloc(adapter, tsize, &adapter->txdma, BUS_DMA_NOWAIT)) {
		device_printf(dev, "Unable to allocate tx_desc memory\n");
		error = ENOMEM;
		goto err_tx_desc;
	}
	adapter->tx_desc_base = (struct em_tx_desc *)adapter->txdma.dma_vaddr;

	rsize = roundup2(adapter->num_rx_desc * sizeof(struct em_rx_desc),
	    EM_DBA_ALIGN);

	/* Allocate Receive Descriptor ring */
	if (em_dma_malloc(adapter, rsize, &adapter->rxdma, BUS_DMA_NOWAIT)) {
		device_printf(dev, "Unable to allocate rx_desc memory\n");
		error = ENOMEM;
		goto err_rx_desc;
	}
	adapter->rx_desc_base = (struct em_rx_desc *)adapter->rxdma.dma_vaddr;

	/* Initialize the hardware */
	if (em_hardware_init(adapter)) {
		device_printf(dev, "Unable to initialize the hardware\n");
		error = EIO;
		goto err_hw_init;
	}

	/* Copy the permanent MAC address out of the EEPROM */
	if (em_read_mac_addr(&adapter->hw) < 0) {
		device_printf(dev, "EEPROM read error while reading MAC"
		    " address\n");
		error = EIO;
		goto err_hw_init;
	}

	if (!em_is_valid_ether_addr(adapter->hw.mac_addr)) {
		device_printf(dev, "Invalid MAC address\n");
		error = EIO;
		goto err_hw_init;
	}

	/* Allocate transmit descriptors and buffers */
	if (em_allocate_transmit_structures(adapter)) {
		device_printf(dev, "Could not setup transmit structures\n");
		error = ENOMEM;
		goto err_tx_struct;
	}

	/* Allocate receive descriptors and buffers */
	if (em_allocate_receive_structures(adapter)) {
		device_printf(dev, "Could not setup receive structures\n");
		error = ENOMEM;
		goto err_rx_struct;
	}

	/* Setup OS specific network interface */
	em_setup_interface(dev, adapter);

	em_allocate_intr(adapter);

	/* Initialize statistics */
	em_clear_hw_cntrs(&adapter->hw);
	em_update_stats_counters(adapter);
	adapter->hw.get_link_status = 1;
	em_update_link_status(adapter);

	/* Indicate SOL/IDER usage */
	if (em_check_phy_reset_block(&adapter->hw))
		device_printf(dev,
		    "PHY reset is blocked due to SOL/IDER session.\n");

	/* Identify 82544 on PCIX */
	em_get_bus_info(&adapter->hw);
	if(adapter->hw.bus_type == em_bus_type_pcix && adapter->hw.mac_type == em_82544)
		adapter->pcix_82544 = TRUE;
	else
		adapter->pcix_82544 = FALSE;

	INIT_DEBUGOUT("em_attach: end");

	return (0);

err_rx_struct:
	em_free_transmit_structures(adapter);
err_tx_struct:
err_hw_init:
	em_dma_free(adapter, &adapter->rxdma);
err_rx_desc:
	em_dma_free(adapter, &adapter->txdma);
err_tx_desc:
err_pci:
	em_free_intr(adapter);
	em_free_pci_resources(adapter);
	EM_LOCK_DESTROY(adapter);

	return (error);
}

/*********************************************************************
 *  Device removal routine
 *
 *  The detach entry point is called when the driver is being removed.
 *  This routine stops the adapter and deallocates all the resources
 *  that were allocated for driver operation.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/

static int
em_detach(device_t dev)
{
	struct adapter	*adapter = device_get_softc(dev);
	struct ifnet	*ifp = adapter->ifp;

	INIT_DEBUGOUT("em_detach: begin");

#ifdef DEVICE_POLLING
	if (ifp->if_capenable & IFCAP_POLLING)
		ether_poll_deregister(ifp);
#endif

	em_free_intr(adapter);
	EM_LOCK(adapter);
	adapter->in_detach = 1;
	em_stop(adapter);
	em_phy_hw_reset(&adapter->hw);
	EM_UNLOCK(adapter);
	ether_ifdetach(adapter->ifp);

	callout_drain(&adapter->timer);
	callout_drain(&adapter->tx_fifo_timer);

	em_free_pci_resources(adapter);
	bus_generic_detach(dev);
	if_free(ifp);

	em_free_transmit_structures(adapter);
	em_free_receive_structures(adapter);

	/* Free Transmit Descriptor ring */
	if (adapter->tx_desc_base) {
		em_dma_free(adapter, &adapter->txdma);
		adapter->tx_desc_base = NULL;
	}

	/* Free Receive Descriptor ring */
	if (adapter->rx_desc_base) {
		em_dma_free(adapter, &adapter->rxdma);
		adapter->rx_desc_base = NULL;
	}

	EM_LOCK_DESTROY(adapter);

	return (0);
}

/*********************************************************************
 *
 *  Shutdown entry point
 *
 **********************************************************************/

static int
em_shutdown(device_t dev)
{
	struct adapter *adapter = device_get_softc(dev);
	EM_LOCK(adapter);
	em_stop(adapter);
	EM_UNLOCK(adapter);
	return (0);
}

/*
 * Suspend/resume device methods.
 */
static int
em_suspend(device_t dev)
{
	struct adapter *adapter = device_get_softc(dev);

	EM_LOCK(adapter);
	em_stop(adapter);
	EM_UNLOCK(adapter);

	return bus_generic_suspend(dev);
}

static int
em_resume(device_t dev)
{
	struct adapter *adapter = device_get_softc(dev);
	struct ifnet *ifp = adapter->ifp;

	EM_LOCK(adapter);
	em_init_locked(adapter);
	if ((ifp->if_flags & IFF_UP) &&
	    (ifp->if_drv_flags & IFF_DRV_RUNNING))
		em_start_locked(ifp);
	EM_UNLOCK(adapter);

	return bus_generic_resume(dev);
}


/*********************************************************************
 *  Transmit entry point
 *
 *  em_start is called by the stack to initiate a transmit.
 *  The driver will remain in this routine as long as there are
 *  packets to transmit and transmit resources are available.
 *  In case resources are not available stack is notified and
 *  the packet is requeued.
 **********************************************************************/

static void
em_start_locked(struct ifnet *ifp)
{
	struct adapter	*adapter = ifp->if_softc;
	struct mbuf	*m_head;

	EM_LOCK_ASSERT(adapter);

	if ((ifp->if_drv_flags & (IFF_DRV_RUNNING|IFF_DRV_OACTIVE)) !=
	    IFF_DRV_RUNNING)
		return;
	if (!adapter->link_active)
		return;

	while (!IFQ_DRV_IS_EMPTY(&ifp->if_snd)) {

		IFQ_DRV_DEQUEUE(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;
		/*
		 * em_encap() can modify our pointer, and or make it NULL on
		 * failure.  In that event, we can't requeue.
		 */
		if (em_encap(adapter, &m_head)) {
			if (m_head == NULL)
				break;
			ifp->if_drv_flags |= IFF_DRV_OACTIVE;
			IFQ_DRV_PREPEND(&ifp->if_snd, m_head);
			break;
		}

		/* Send a copy of the frame to the BPF listener */
		ETHER_BPF_MTAP(ifp, m_head);

		/* Set timeout in case hardware has problems transmitting. */
		adapter->watchdog_timer = EM_TX_TIMEOUT;
	}
}

static void
em_start(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;

	EM_LOCK(adapter);
	if (ifp->if_drv_flags & IFF_DRV_RUNNING)
		em_start_locked(ifp);
	EM_UNLOCK(adapter);
}

/*********************************************************************
 *  Ioctl entry point
 *
 *  em_ioctl is called when the user wants to configure the
 *  interface.
 *
 *  return 0 on success, positive on failure
 **********************************************************************/

static int
em_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
{
	struct adapter	*adapter = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int error = 0;

	if (adapter->in_detach)
		return (error);

	switch (command) {
	case SIOCSIFADDR:
	case SIOCGIFADDR:
		if (ifa->ifa_addr->sa_family == AF_INET) {
			/*
			 * XXX
			 * Since resetting hardware takes a very long time
			 * and results in link renegotiation we only
			 * initialize the hardware only when it is absolutely
			 * required.
			 */
			ifp->if_flags |= IFF_UP;
			if (!(ifp->if_drv_flags & IFF_DRV_RUNNING)) {
				EM_LOCK(adapter);
				em_init_locked(adapter);
				EM_UNLOCK(adapter);
			}
			arp_ifinit(ifp, ifa);
		} else
			error = ether_ioctl(ifp, command, data);
		break;
	case SIOCSIFMTU:
	    {
		int max_frame_size;
		uint16_t eeprom_data = 0;

		IOCTL_DEBUGOUT("ioctl rcv'd: SIOCSIFMTU (Set Interface MTU)");

		EM_LOCK(adapter);
		switch (adapter->hw.mac_type) {
		case em_82573:
			/*
			 * 82573 only supports jumbo frames
			 * if ASPM is disabled.
			 */
			em_read_eeprom(&adapter->hw, EEPROM_INIT_3GIO_3, 1,
			    &eeprom_data);
			if (eeprom_data & EEPROM_WORD1A_ASPM_MASK) {
				max_frame_size = ETHER_MAX_LEN;
				break;
			}
			/* Allow Jumbo frames - fall thru */
		case em_82571:
		case em_82572:
		case em_80003es2lan:	/* Limit Jumbo Frame size */
			max_frame_size = 9234;
			break;
		case em_ich8lan:
			/* ICH8 does not support jumbo frames */
			max_frame_size = ETHER_MAX_LEN;
			break;
		default:
			max_frame_size = MAX_JUMBO_FRAME_SIZE;
		}
		if (ifr->ifr_mtu > max_frame_size - ETHER_HDR_LEN -
		    ETHER_CRC_LEN) {
			EM_UNLOCK(adapter);
			error = EINVAL;
			break;
		}

		ifp->if_mtu = ifr->ifr_mtu;
		adapter->hw.max_frame_size =
		    ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN;
		em_init_locked(adapter);
		EM_UNLOCK(adapter);
		break;
	    }
	case SIOCSIFFLAGS:
		IOCTL_DEBUGOUT("ioctl rcv'd: SIOCSIFFLAGS (Set Interface Flags)");
		EM_LOCK(adapter);
		if (ifp->if_flags & IFF_UP) {
			if ((ifp->if_drv_flags & IFF_DRV_RUNNING)) {
				if ((ifp->if_flags ^ adapter->if_flags) &
				    IFF_PROMISC) {
					em_disable_promisc(adapter);
					em_set_promisc(adapter);
				}
			} else
				em_init_locked(adapter);
		} else {
			if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
				em_stop(adapter);
			}
		}
		adapter->if_flags = ifp->if_flags;
		EM_UNLOCK(adapter);
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		IOCTL_DEBUGOUT("ioctl rcv'd: SIOC(ADD|DEL)MULTI");
		if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
			EM_LOCK(adapter);
			em_disable_intr(adapter);
			em_set_multi(adapter);
			if (adapter->hw.mac_type == em_82542_rev2_0) {
				em_initialize_receive_unit(adapter);
			}
#ifdef DEVICE_POLLING
			if (!(ifp->if_capenable & IFCAP_POLLING))
#endif
				em_enable_intr(adapter);
			EM_UNLOCK(adapter);
		}
		break;
	case SIOCSIFMEDIA:
		/* Check SOL/IDER usage */
		EM_LOCK(adapter);
		if (em_check_phy_reset_block(&adapter->hw)) {
			EM_UNLOCK(adapter);
			device_printf(adapter->dev, "Media change is"
			    "blocked due to SOL/IDER session.\n");
			break;
		}
		EM_UNLOCK(adapter);
	case SIOCGIFMEDIA:
		IOCTL_DEBUGOUT("ioctl rcv'd: SIOCxIFMEDIA (Get/Set Interface Media)");
		error = ifmedia_ioctl(ifp, ifr, &adapter->media, command);
		break;
	case SIOCSIFCAP:
	    {
		int mask, reinit;

		IOCTL_DEBUGOUT("ioctl rcv'd: SIOCSIFCAP (Set Capabilities)");
		reinit = 0;
		mask = ifr->ifr_reqcap ^ ifp->if_capenable;
#ifdef DEVICE_POLLING
		if (mask & IFCAP_POLLING) {
			if (ifr->ifr_reqcap & IFCAP_POLLING) {
				error = ether_poll_register(em_poll, ifp);
				if (error)
					return (error);
				EM_LOCK(adapter);
				em_disable_intr(adapter);
				ifp->if_capenable |= IFCAP_POLLING;
				EM_UNLOCK(adapter);
			} else {
				error = ether_poll_deregister(ifp);
				/* Enable interrupt even in error case */
				EM_LOCK(adapter);
				em_enable_intr(adapter);
				ifp->if_capenable &= ~IFCAP_POLLING;
				EM_UNLOCK(adapter);
			}
		}
#endif
		if (mask & IFCAP_HWCSUM) {
			ifp->if_capenable ^= IFCAP_HWCSUM;
			reinit = 1;
		}
		if (mask & IFCAP_TSO4) {
			ifp->if_capenable ^= IFCAP_TSO4;
			reinit = 1;
		}
		if (mask & IFCAP_VLAN_HWTAGGING) {
			ifp->if_capenable ^= IFCAP_VLAN_HWTAGGING;
			reinit = 1;
		}
		if (reinit && (ifp->if_drv_flags & IFF_DRV_RUNNING))
			em_init(adapter);
		VLAN_CAPABILITIES(ifp);
		break;
	    }
	default:
		error = ether_ioctl(ifp, command, data);
		break;
	}

	return (error);
}

/*********************************************************************
 *  Watchdog timer:
 *
 *  This routine is called from the local timer every second.
 *  As long as transmit descriptors are being cleaned the value
 *  is non-zero and we do nothing. Reaching 0 indicates a tx hang
 *  and we then reset the device.
 *
 **********************************************************************/

static void
em_watchdog(struct adapter *adapter)
{

	EM_LOCK_ASSERT(adapter);

	/*
	 * The timer is set to 5 every time em_start() queues a packet.
	 * Then em_txeof() keeps resetting to 5 as long as it cleans at
	 * least one descriptor.
	 * Finally, anytime all descriptors are clean the timer is
	 * set to 0.
	 */
	if (adapter->watchdog_timer == 0 || --adapter->watchdog_timer)
		return;

	/* If we are in this routine because of pause frames, then
	 * don't reset the hardware.
	 */
	if (E1000_READ_REG(&adapter->hw, STATUS) & E1000_STATUS_TXOFF) {
		adapter->watchdog_timer = EM_TX_TIMEOUT;
		return;
	}

	if (em_check_for_link(&adapter->hw) == 0)
		device_printf(adapter->dev, "watchdog timeout -- resetting\n");

	adapter->ifp->if_drv_flags &= ~IFF_DRV_RUNNING;
	adapter->watchdog_events++;

	em_init_locked(adapter);
}

/*********************************************************************
 *  Init entry point
 *
 *  This routine is used in two ways. It is used by the stack as
 *  init entry point in network interface structure. It is also used
 *  by the driver as a hw/sw initialization routine to get to a
 *  consistent state.
 *
 *  return 0 on success, positive on failure
 **********************************************************************/

static void
em_init_locked(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	device_t	dev = adapter->dev;
	uint32_t	pba;

	INIT_DEBUGOUT("em_init: begin");

	EM_LOCK_ASSERT(adapter);

	em_stop(adapter);

	/*
	 * Packet Buffer Allocation (PBA)
	 * Writing PBA sets the receive portion of the buffer
	 * the remainder is used for the transmit buffer.
	 *
	 * Devices before the 82547 had a Packet Buffer of 64K.
	 *   Default allocation: PBA=48K for Rx, leaving 16K for Tx.
	 * After the 82547 the buffer was reduced to 40K.
	 *   Default allocation: PBA=30K for Rx, leaving 10K for Tx.
	 *   Note: default does not leave enough room for Jumbo Frame >10k.
	 */
	switch (adapter->hw.mac_type) {
	case em_82547:
	case em_82547_rev_2: /* 82547: Total Packet Buffer is 40K */
		if (adapter->hw.max_frame_size > EM_RXBUFFER_8192)
			pba = E1000_PBA_22K; /* 22K for Rx, 18K for Tx */
		else
			pba = E1000_PBA_30K; /* 30K for Rx, 10K for Tx */
		adapter->tx_fifo_head = 0;
		adapter->tx_head_addr = pba << EM_TX_HEAD_ADDR_SHIFT;
		adapter->tx_fifo_size = (E1000_PBA_40K - pba) << EM_PBA_BYTES_SHIFT;
		break;
	case em_80003es2lan: /* 80003es2lan: Total Packet Buffer is 48K */
	case em_82571: /* 82571: Total Packet Buffer is 48K */
	case em_82572: /* 82572: Total Packet Buffer is 48K */
			pba = E1000_PBA_32K; /* 32K for Rx, 16K for Tx */
		break;
	case em_82573: /* 82573: Total Packet Buffer is 32K */
		/* Jumbo frames not supported */
			pba = E1000_PBA_12K; /* 12K for Rx, 20K for Tx */
		break;
	case em_ich8lan:
		pba = E1000_PBA_8K;
		break;
	default:
		/* Devices before 82547 had a Packet Buffer of 64K.   */
		if(adapter->hw.max_frame_size > EM_RXBUFFER_8192)
			pba = E1000_PBA_40K; /* 40K for Rx, 24K for Tx */
		else
			pba = E1000_PBA_48K; /* 48K for Rx, 16K for Tx */
	}

	INIT_DEBUGOUT1("em_init: pba=%dK",pba);
	E1000_WRITE_REG(&adapter->hw, PBA, pba);
	
	/* Get the latest mac address, User can use a LAA */
	bcopy(IF_LLADDR(adapter->ifp), adapter->hw.mac_addr, ETHER_ADDR_LEN);

	/* Initialize the hardware */
	if (em_hardware_init(adapter)) {
		device_printf(dev, "Unable to initialize the hardware\n");
		return;
	}
	em_update_link_status(adapter);

	if (ifp->if_capenable & IFCAP_VLAN_HWTAGGING)
		em_enable_vlans(adapter);

	ifp->if_hwassist = 0;
	if (adapter->hw.mac_type >= em_82543) {
		if (ifp->if_capenable & IFCAP_TXCSUM)
			ifp->if_hwassist = EM_CHECKSUM_FEATURES;
		if (ifp->if_capenable & IFCAP_TSO)
			ifp->if_hwassist |= EM_TCPSEG_FEATURES;
	}

	/* Prepare transmit descriptors and buffers */
	em_setup_transmit_structures(adapter);
	em_initialize_transmit_unit(adapter);

	/* Setup Multicast table */
	em_set_multi(adapter);

	/* Prepare receive descriptors and buffers */
	if (em_setup_receive_structures(adapter)) {
		device_printf(dev, "Could not setup receive structures\n");
		em_stop(adapter);
		return;
	}
	em_initialize_receive_unit(adapter);

	/* Don't lose promiscuous settings */
	em_set_promisc(adapter);

	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;

	callout_reset(&adapter->timer, hz, em_local_timer, adapter);
	em_clear_hw_cntrs(&adapter->hw);
#ifdef DEVICE_POLLING
	/*
	 * Only enable interrupts if we are not polling, make sure
	 * they are off otherwise.
	 */
	if (ifp->if_capenable & IFCAP_POLLING)
		em_disable_intr(adapter);
	else
#endif /* DEVICE_POLLING */
		em_enable_intr(adapter);

	/* Don't reset the phy next time init gets called */
	adapter->hw.phy_reset_disable = TRUE;
}

static void
em_init(void *arg)
{
	struct adapter *adapter = arg;

	EM_LOCK(adapter);
	em_init_locked(adapter);
	EM_UNLOCK(adapter);
}


#ifdef DEVICE_POLLING
/*********************************************************************
 *
 *  Legacy polling routine
 *
 *********************************************************************/
static void
em_poll(struct ifnet *ifp, enum poll_cmd cmd, int count)
{
	struct adapter *adapter = ifp->if_softc;
	uint32_t reg_icr;

	EM_LOCK(adapter);
	if ((ifp->if_drv_flags & IFF_DRV_RUNNING) == 0) {
		EM_UNLOCK(adapter);
		return;
	}

	if (cmd == POLL_AND_CHECK_STATUS) {
		reg_icr = E1000_READ_REG(&adapter->hw, ICR);
		if (reg_icr & (E1000_ICR_RXSEQ | E1000_ICR_LSC)) {
			callout_stop(&adapter->timer);
			adapter->hw.get_link_status = 1;
			em_check_for_link(&adapter->hw);
			em_update_link_status(adapter);
			callout_reset(&adapter->timer, hz, em_local_timer, adapter);
		}
	}
	em_rxeof(adapter, count);
	em_txeof(adapter);

	if (!IFQ_DRV_IS_EMPTY(&ifp->if_snd))
		em_start_locked(ifp);
	EM_UNLOCK(adapter);
}

/*********************************************************************
 *
 *  Legacy Interrupt Service routine
 *
 *********************************************************************/
static void
em_intr(void *arg)
{
	struct adapter	*adapter = arg;
	struct ifnet	*ifp;
	uint32_t	reg_icr;

	EM_LOCK(adapter);

	ifp = adapter->ifp;

	if (ifp->if_capenable & IFCAP_POLLING) {
		EM_UNLOCK(adapter);
		return;
	}

	for (;;) {
		reg_icr = E1000_READ_REG(&adapter->hw, ICR);
		if (adapter->hw.mac_type >= em_82571 &&
		    (reg_icr & E1000_ICR_INT_ASSERTED) == 0)
			break;
		else if (reg_icr == 0)
			break;

		/*
		 * XXX: some laptops trigger several spurious interrupts
		 * on em(4) when in the resume cycle. The ICR register
		 * reports all-ones value in this case. Processing such
		 * interrupts would lead to a freeze. I don't know why.
		 */
		if (reg_icr == 0xffffffff)
			break;

		if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
			em_rxeof(adapter, -1);
			em_txeof(adapter);
		}

		/* Link status change */
		if (reg_icr & (E1000_ICR_RXSEQ | E1000_ICR_LSC)) {
			callout_stop(&adapter->timer);
			adapter->hw.get_link_status = 1;
			em_check_for_link(&adapter->hw);
			em_update_link_status(adapter);
			callout_reset(&adapter->timer, hz, em_local_timer, adapter);
		}

		if (reg_icr & E1000_ICR_RXO)
			adapter->rx_overruns++;
	}

	if (ifp->if_drv_flags & IFF_DRV_RUNNING &&
	    !IFQ_DRV_IS_EMPTY(&ifp->if_snd))
		em_start_locked(ifp);

	EM_UNLOCK(adapter);
}

#else  /* if not DEVICE_POLLING, then fast interrupt routines only */

static void
em_handle_link(void *context, int pending)
{
	struct adapter	*adapter = context;
	struct ifnet *ifp;

	ifp = adapter->ifp;

	EM_LOCK(adapter);
	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING)) {
		EM_UNLOCK(adapter);
		return;
	}

	callout_stop(&adapter->timer);
	adapter->hw.get_link_status = 1;
	em_check_for_link(&adapter->hw);
	em_update_link_status(adapter);
	callout_reset(&adapter->timer, hz, em_local_timer, adapter);
	EM_UNLOCK(adapter);
}

static void
em_handle_rxtx(void *context, int pending)
{
	struct adapter	*adapter = context;
	struct ifnet	*ifp;

	NET_LOCK_GIANT();
	ifp = adapter->ifp;

	/*
	 * TODO:
	 * It should be possible to run the tx clean loop without the lock.
	 */
	if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
		if (em_rxeof(adapter, adapter->rx_process_limit) != 0)
			taskqueue_enqueue(adapter->tq, &adapter->rxtx_task);
		EM_LOCK(adapter);
		em_txeof(adapter);

		if (!IFQ_DRV_IS_EMPTY(&ifp->if_snd))
			em_start_locked(ifp);
		EM_UNLOCK(adapter);
	}

	em_enable_intr(adapter);
	NET_UNLOCK_GIANT();
}

/*********************************************************************
 *
 *  Fast Interrupt Service routine
 *
 *********************************************************************/
static void
em_intr_fast(void *arg)
{
	struct adapter	*adapter = arg;
	struct ifnet	*ifp;
	uint32_t	reg_icr;

	ifp = adapter->ifp;

	reg_icr = E1000_READ_REG(&adapter->hw, ICR);

	/* Hot eject?  */
	if (reg_icr == 0xffffffff)
		return;

	/* Definitely not our interrupt.  */
	if (reg_icr == 0x0)
		return;

	/*
	 * Starting with the 82571 chip, bit 31 should be used to
	 * determine whether the interrupt belongs to us.
	 */
	if (adapter->hw.mac_type >= em_82571 &&
	    (reg_icr & E1000_ICR_INT_ASSERTED) == 0)
		return;

	/*
	 * Mask interrupts until the taskqueue is finished running.  This is
	 * cheap, just assume that it is needed.  This also works around the
	 * MSI message reordering errata on certain systems.
	 */
	em_disable_intr(adapter);
	taskqueue_enqueue(adapter->tq, &adapter->rxtx_task);

	/* Link status change */
	if (reg_icr & (E1000_ICR_RXSEQ | E1000_ICR_LSC))
		taskqueue_enqueue(taskqueue_fast, &adapter->link_task);

	if (reg_icr & E1000_ICR_RXO)
		adapter->rx_overruns++;
}
#endif /* ! DEVICE_POLLING */

/*********************************************************************
 *
 *  Media Ioctl callback
 *
 *  This routine is called whenever the user queries the status of
 *  the interface using ifconfig.
 *
 **********************************************************************/
static void
em_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct adapter *adapter = ifp->if_softc;

	INIT_DEBUGOUT("em_media_status: begin");

	EM_LOCK(adapter);
	em_check_for_link(&adapter->hw);
	em_update_link_status(adapter);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (!adapter->link_active) {
		EM_UNLOCK(adapter);
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;

	if ((adapter->hw.media_type == em_media_type_fiber) ||
	    (adapter->hw.media_type == em_media_type_internal_serdes)) {
		if (adapter->hw.mac_type == em_82545)
			ifmr->ifm_active |= IFM_1000_LX | IFM_FDX;
		else
			ifmr->ifm_active |= IFM_1000_SX | IFM_FDX;
	} else {
		switch (adapter->link_speed) {
		case 10:
			ifmr->ifm_active |= IFM_10_T;
			break;
		case 100:
			ifmr->ifm_active |= IFM_100_TX;
			break;
		case 1000:
			ifmr->ifm_active |= IFM_1000_T;
			break;
		}
		if (adapter->link_duplex == FULL_DUPLEX)
			ifmr->ifm_active |= IFM_FDX;
		else
			ifmr->ifm_active |= IFM_HDX;
	}
	EM_UNLOCK(adapter);
}

/*********************************************************************
 *
 *  Media Ioctl callback
 *
 *  This routine is called when the user changes speed/duplex using
 *  media/mediopt option with ifconfig.
 *
 **********************************************************************/
static int
em_media_change(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;
	struct ifmedia  *ifm = &adapter->media;

	INIT_DEBUGOUT("em_media_change: begin");

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	EM_LOCK(adapter);
	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		adapter->hw.autoneg = DO_AUTO_NEG;
		adapter->hw.autoneg_advertised = AUTONEG_ADV_DEFAULT;
		break;
	case IFM_1000_LX:
	case IFM_1000_SX:
	case IFM_1000_T:
		adapter->hw.autoneg = DO_AUTO_NEG;
		adapter->hw.autoneg_advertised = ADVERTISE_1000_FULL;
		break;
	case IFM_100_TX:
		adapter->hw.autoneg = FALSE;
		adapter->hw.autoneg_advertised = 0;
		if ((ifm->ifm_media & IFM_GMASK) == IFM_FDX)
			adapter->hw.forced_speed_duplex = em_100_full;
		else
			adapter->hw.forced_speed_duplex = em_100_half;
		break;
	case IFM_10_T:
		adapter->hw.autoneg = FALSE;
		adapter->hw.autoneg_advertised = 0;
		if ((ifm->ifm_media & IFM_GMASK) == IFM_FDX)
			adapter->hw.forced_speed_duplex = em_10_full;
		else
			adapter->hw.forced_speed_duplex = em_10_half;
		break;
	default:
		device_printf(adapter->dev, "Unsupported media type\n");
	}

	/* As the speed/duplex settings my have changed we need to
	 * reset the PHY.
	 */
	adapter->hw.phy_reset_disable = FALSE;

	em_init_locked(adapter);
	EM_UNLOCK(adapter);

	return (0);
}

/*********************************************************************
 *
 *  This routine maps the mbufs to tx descriptors.
 *
 *  return 0 on success, positive on failure
 **********************************************************************/
static int
em_encap(struct adapter *adapter, struct mbuf **m_headp)
{
	struct ifnet		*ifp = adapter->ifp;
	bus_dma_segment_t	segs[EM_MAX_SCATTER];
	bus_dmamap_t		map;
	struct em_buffer	*tx_buffer, *tx_buffer_mapped;
	struct em_tx_desc	*current_tx_desc;
	struct mbuf		*m_head;
	uint32_t		txd_upper, txd_lower, txd_used, txd_saved;
	int			nsegs, i, j, first, last = 0;
	int			error, do_tso, tso_desc = 0;

	m_head = *m_headp;
	current_tx_desc = NULL;
	txd_upper = txd_lower = txd_used = txd_saved = 0;

	do_tso = ((m_head->m_pkthdr.csum_flags & CSUM_TSO) != 0);

	/*
	 * Force a cleanup if number of TX descriptors
	 * available hits the threshold.
	 */
	if (adapter->num_tx_desc_avail <= EM_TX_CLEANUP_THRESHOLD) {
		em_txeof(adapter);
		if (adapter->num_tx_desc_avail <= EM_TX_CLEANUP_THRESHOLD) {
			adapter->no_tx_desc_avail1++;
			return (ENOBUFS);
		}
	}

	/*
	 * TSO workaround:
	 *  If an mbuf contains only the IP and TCP header we have
	 *  to pull 4 bytes of data into it.
	 */
	if (do_tso && (m_head->m_len <= M_TSO_LEN)) {
		m_head = m_pullup(m_head, M_TSO_LEN + 4);
		*m_headp = m_head;
		if (m_head == NULL) {
			return (ENOBUFS);
		}
	}

	/*
	 * Map the packet for DMA.
	 *
	 * Capture the first descriptor index,
	 * this descriptor will have the index
	 * of the EOP which is the only one that
	 * now gets a DONE bit writeback.
	 */
	first = adapter->next_avail_tx_desc;
	tx_buffer = &adapter->tx_buffer_area[first];
	tx_buffer_mapped = tx_buffer;
	map = tx_buffer->map;

	error = bus_dmamap_load_mbuf_sg(adapter->txtag, map, *m_headp, segs,
	    &nsegs, BUS_DMA_NOWAIT);

	/*
	 * There are two types of errors we can (try) to handle:
	 * - EFBIG means the mbuf chain was too long and bus_dma ran
	 *   out of segments.  Defragment the mbuf chain and try again.
	 * - ENOMEM means bus_dma could not obtain enough bounce buffers
	 *   at this point in time.  Defer sending and try again later.
	 * All other errors, in particular EINVAL, are fatal and prevent the
	 * mbuf chain from ever going through.  Drop it and report error.
	 */
	if (error == EFBIG) {
		struct mbuf *m;

		m = m_defrag(*m_headp, M_DONTWAIT);
		if (m == NULL) {
			/* Assume m_defrag(9) used only m_get(9). */
			adapter->mbuf_alloc_failed++;
			m_freem(*m_headp);
			*m_headp = NULL;
			return (ENOBUFS);
		}
		*m_headp = m;

		error = bus_dmamap_load_mbuf_sg(adapter->txtag, map, *m_headp,
		    segs, &nsegs, BUS_DMA_NOWAIT);

		if (error == ENOMEM) {
			adapter->no_tx_dma_setup++;
			return (error);
		} else if (error != 0) {
			adapter->no_tx_dma_setup++;
			m_freem(*m_headp);
			*m_headp = NULL;
			return (error);
		}
	} else if (error == ENOMEM) {
		adapter->no_tx_dma_setup++;
		return (error);
	} else if (error != 0) {
		adapter->no_tx_dma_setup++;
		m_freem(*m_headp);
		*m_headp = NULL;
		return (error);
	}

	/*
	 * TSO Hardware workaround, if this packet is not
	 * TSO, and is only a single descriptor long, and
	 * it follows a TSO burst, then we need to add a
	 * sentinel descriptor to prevent premature writeback.
	 */
	if ((do_tso == 0) && (adapter->tx_tso == TRUE)) {
		if (nsegs == 1)
			tso_desc = TRUE;
		adapter->tx_tso = FALSE;
	}

	if (nsegs > adapter->num_tx_desc_avail - 2) {
		adapter->no_tx_desc_avail2++;
		bus_dmamap_unload(adapter->txtag, map);
		return (ENOBUFS);
	}
	m_head = *m_headp;

	/* Do hardware assists */
	if (ifp->if_hwassist) {
		if (do_tso &&
		    em_tso_setup(adapter, m_head, &txd_upper, &txd_lower)) {
			/* we need to make a final sentinel transmit desc */
			tso_desc = TRUE;
		} else
			em_transmit_checksum_setup(adapter,  m_head,
			    &txd_upper, &txd_lower);
	}

	i = adapter->next_avail_tx_desc;
	if (adapter->pcix_82544)
		txd_saved = i;

	for (j = 0; j < nsegs; j++) {
		bus_size_t seg_len;
		bus_addr_t seg_addr;
		/* If adapter is 82544 and on PCIX bus. */
		if(adapter->pcix_82544) {
			DESC_ARRAY	desc_array;
			uint32_t	array_elements, counter;

			/*
			 * Check the Address and Length combination and
			 * split the data accordingly
			 */
			array_elements = em_fill_descriptors(segs[j].ds_addr,
			    segs[j].ds_len, &desc_array);
			for (counter = 0; counter < array_elements; counter++) {
				if (txd_used == adapter->num_tx_desc_avail) {
					adapter->next_avail_tx_desc = txd_saved;
					adapter->no_tx_desc_avail2++;
					bus_dmamap_unload(adapter->txtag, map);
					return (ENOBUFS);
				}
				tx_buffer = &adapter->tx_buffer_area[i];
				current_tx_desc = &adapter->tx_desc_base[i];
				current_tx_desc->buffer_addr = htole64(
					desc_array.descriptor[counter].address);
				current_tx_desc->lower.data = htole32(
					(adapter->txd_cmd | txd_lower |
					(uint16_t)desc_array.descriptor[counter].length));
				current_tx_desc->upper.data = htole32((txd_upper));
				last = i;
				if (++i == adapter->num_tx_desc)
					i = 0;

				tx_buffer->m_head = NULL;
				tx_buffer->next_eop = -1;
				txd_used++;
			}
		} else {
			tx_buffer = &adapter->tx_buffer_area[i];
			current_tx_desc = &adapter->tx_desc_base[i];
			seg_addr = segs[j].ds_addr;
			seg_len  = segs[j].ds_len;
			/*
			** TSO Workaround:
			** If this is the last descriptor, we want to
			** split it so we have a small final sentinel
			*/
			if (tso_desc && (j == (nsegs -1)) && (seg_len > 8)) {
				seg_len -= 4;
				current_tx_desc->buffer_addr = htole64(seg_addr);
				current_tx_desc->lower.data = htole32(
				adapter->txd_cmd | txd_lower | seg_len);
				current_tx_desc->upper.data =
				    htole32(txd_upper);
				if (++i == adapter->num_tx_desc)
					i = 0;
				/* Now make the sentinel */
				++txd_used; /* using an extra txd */
				current_tx_desc = &adapter->tx_desc_base[i];
				tx_buffer = &adapter->tx_buffer_area[i];
				current_tx_desc->buffer_addr =
					htole64(seg_addr + seg_len);
				current_tx_desc->lower.data = htole32(
				adapter->txd_cmd | txd_lower | 4);
				current_tx_desc->upper.data =
				    htole32(txd_upper);
				last = i;
				if (++i == adapter->num_tx_desc)
					i = 0;
			} else {
				current_tx_desc->buffer_addr = htole64(seg_addr);
				current_tx_desc->lower.data = htole32(
				adapter->txd_cmd | txd_lower | seg_len);
				current_tx_desc->upper.data =
				    htole32(txd_upper);
				last = i;
				if (++i == adapter->num_tx_desc)
					i = 0;
			}
			tx_buffer->m_head = NULL;
			tx_buffer->next_eop = -1;
		}
	}

	adapter->next_avail_tx_desc = i;
	if (adapter->pcix_82544)
		adapter->num_tx_desc_avail -= txd_used;
	else {
		adapter->num_tx_desc_avail -= nsegs;
		if (tso_desc) /* TSO used an extra for sentinel */
			adapter->num_tx_desc_avail -= txd_used;
	}

	if (m_head->m_flags & M_VLANTAG) {
		/* Set the vlan id. */
		current_tx_desc->upper.fields.special =
		    htole16(m_head->m_pkthdr.ether_vtag);

		/* Tell hardware to add tag. */
		current_tx_desc->lower.data |= htole32(E1000_TXD_CMD_VLE);
	}

	tx_buffer->m_head = m_head;
	tx_buffer_mapped->map = tx_buffer->map;
	tx_buffer->map = map;
	bus_dmamap_sync(adapter->txtag, map, BUS_DMASYNC_PREWRITE);

	/*
	 * Last Descriptor of Packet
	 * needs End Of Packet (EOP)
	 * and Report Status (RS)
	 */
	current_tx_desc->lower.data |=
	    htole32(E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS);
	/*
	 * Keep track in the first buffer which
	 * descriptor will be written back
	 */
	tx_buffer = &adapter->tx_buffer_area[first];
	tx_buffer->next_eop = last;

	/*
	 * Advance the Transmit Descriptor Tail (Tdt), this tells the E1000
	 * that this frame is available to transmit.
	 */
	bus_dmamap_sync(adapter->txdma.dma_tag, adapter->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if (adapter->hw.mac_type == em_82547 &&
	    adapter->link_duplex == HALF_DUPLEX)
		em_82547_move_tail(adapter);
	else {
		E1000_WRITE_REG(&adapter->hw, TDT, i);
		if (adapter->hw.mac_type == em_82547)
			em_82547_update_fifo_head(adapter, m_head->m_pkthdr.len);
	}

	return (0);
}

/*********************************************************************
 *
 * 82547 workaround to avoid controller hang in half-duplex environment.
 * The workaround is to avoid queuing a large packet that would span
 * the internal Tx FIFO ring boundary. We need to reset the FIFO pointers
 * in this case. We do that only when FIFO is quiescent.
 *
 **********************************************************************/
static void
em_82547_move_tail(void *arg)
{
	struct adapter *adapter = arg;
	uint16_t hw_tdt;
	uint16_t sw_tdt;
	struct em_tx_desc *tx_desc;
	uint16_t length = 0;
	boolean_t eop = 0;

	EM_LOCK_ASSERT(adapter);

	hw_tdt = E1000_READ_REG(&adapter->hw, TDT);
	sw_tdt = adapter->next_avail_tx_desc;
	
	while (hw_tdt != sw_tdt) {
		tx_desc = &adapter->tx_desc_base[hw_tdt];
		length += tx_desc->lower.flags.length;
		eop = tx_desc->lower.data & E1000_TXD_CMD_EOP;
		if(++hw_tdt == adapter->num_tx_desc)
			hw_tdt = 0;

		if (eop) {
			if (em_82547_fifo_workaround(adapter, length)) {
				adapter->tx_fifo_wrk_cnt++;
				callout_reset(&adapter->tx_fifo_timer, 1,
					em_82547_move_tail, adapter);
				break;
			}
			E1000_WRITE_REG(&adapter->hw, TDT, hw_tdt);
			em_82547_update_fifo_head(adapter, length);
			length = 0;
		}
	}	
}

static int
em_82547_fifo_workaround(struct adapter *adapter, int len)
{	
	int fifo_space, fifo_pkt_len;

	fifo_pkt_len = roundup2(len + EM_FIFO_HDR, EM_FIFO_HDR);

	if (adapter->link_duplex == HALF_DUPLEX) {
		fifo_space = adapter->tx_fifo_size - adapter->tx_fifo_head;

		if (fifo_pkt_len >= (EM_82547_PKT_THRESH + fifo_space)) {
			if (em_82547_tx_fifo_reset(adapter))
				return (0);
			else
				return (1);
		}
	}

	return (0);
}

static void
em_82547_update_fifo_head(struct adapter *adapter, int len)
{
	int fifo_pkt_len = roundup2(len + EM_FIFO_HDR, EM_FIFO_HDR);
	
	/* tx_fifo_head is always 16 byte aligned */
	adapter->tx_fifo_head += fifo_pkt_len;
	if (adapter->tx_fifo_head >= adapter->tx_fifo_size) {
		adapter->tx_fifo_head -= adapter->tx_fifo_size;
	}
}


static int
em_82547_tx_fifo_reset(struct adapter *adapter)
{	
	uint32_t tctl;

	if ((E1000_READ_REG(&adapter->hw, TDT) == E1000_READ_REG(&adapter->hw, TDH)) &&
	    (E1000_READ_REG(&adapter->hw, TDFT) == E1000_READ_REG(&adapter->hw, TDFH)) &&
	    (E1000_READ_REG(&adapter->hw, TDFTS) == E1000_READ_REG(&adapter->hw, TDFHS))&&
	    (E1000_READ_REG(&adapter->hw, TDFPC) == 0)) {

		/* Disable TX unit */
		tctl = E1000_READ_REG(&adapter->hw, TCTL);
		E1000_WRITE_REG(&adapter->hw, TCTL, tctl & ~E1000_TCTL_EN);

		/* Reset FIFO pointers */
		E1000_WRITE_REG(&adapter->hw, TDFT,  adapter->tx_head_addr);
		E1000_WRITE_REG(&adapter->hw, TDFH,  adapter->tx_head_addr);
		E1000_WRITE_REG(&adapter->hw, TDFTS, adapter->tx_head_addr);
		E1000_WRITE_REG(&adapter->hw, TDFHS, adapter->tx_head_addr);

		/* Re-enable TX unit */
		E1000_WRITE_REG(&adapter->hw, TCTL, tctl);
		E1000_WRITE_FLUSH(&adapter->hw);

		adapter->tx_fifo_head = 0;
		adapter->tx_fifo_reset_cnt++;

		return (TRUE);
	}
	else {
		return (FALSE);
	}
}

static void
em_set_promisc(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	uint32_t	reg_rctl;

	reg_rctl = E1000_READ_REG(&adapter->hw, RCTL);

	if (ifp->if_flags & IFF_PROMISC) {
		reg_rctl |= (E1000_RCTL_UPE | E1000_RCTL_MPE);
		E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
	} else if (ifp->if_flags & IFF_ALLMULTI) {
		reg_rctl |= E1000_RCTL_MPE;
		reg_rctl &= ~E1000_RCTL_UPE;
		E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
	}
}

static void
em_disable_promisc(struct adapter *adapter)
{
	uint32_t	reg_rctl;

	reg_rctl = E1000_READ_REG(&adapter->hw, RCTL);

	reg_rctl &=  (~E1000_RCTL_UPE);
	reg_rctl &=  (~E1000_RCTL_MPE);
	E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
}


/*********************************************************************
 *  Multicast Update
 *
 *  This routine is called whenever multicast address list is updated.
 *
 **********************************************************************/

static void
em_set_multi(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	struct ifmultiaddr *ifma;
	uint32_t reg_rctl = 0;
	uint8_t  mta[MAX_NUM_MULTICAST_ADDRESSES * ETH_LENGTH_OF_ADDRESS];
	int mcnt = 0;

	IOCTL_DEBUGOUT("em_set_multi: begin");

	if (adapter->hw.mac_type == em_82542_rev2_0) {
		reg_rctl = E1000_READ_REG(&adapter->hw, RCTL);
		if (adapter->hw.pci_cmd_word & CMD_MEM_WRT_INVALIDATE)
			em_pci_clear_mwi(&adapter->hw);
		reg_rctl |= E1000_RCTL_RST;
		E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
		msec_delay(5);
	}

	IF_ADDR_LOCK(ifp);
	TAILQ_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
		if (ifma->ifma_addr->sa_family != AF_LINK)
			continue;

		if (mcnt == MAX_NUM_MULTICAST_ADDRESSES)
			break;

		bcopy(LLADDR((struct sockaddr_dl *)ifma->ifma_addr),
		    &mta[mcnt*ETH_LENGTH_OF_ADDRESS], ETH_LENGTH_OF_ADDRESS);
		mcnt++;
	}
	IF_ADDR_UNLOCK(ifp);

	if (mcnt >= MAX_NUM_MULTICAST_ADDRESSES) {
		reg_rctl = E1000_READ_REG(&adapter->hw, RCTL);
		reg_rctl |= E1000_RCTL_MPE;
		E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
	} else
		em_mc_addr_list_update(&adapter->hw, mta, mcnt, 0, 1);

	if (adapter->hw.mac_type == em_82542_rev2_0) {
		reg_rctl = E1000_READ_REG(&adapter->hw, RCTL);
		reg_rctl &= ~E1000_RCTL_RST;
		E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);
		msec_delay(5);
		if (adapter->hw.pci_cmd_word & CMD_MEM_WRT_INVALIDATE)
			em_pci_set_mwi(&adapter->hw);
	}
}


/*********************************************************************
 *  Timer routine
 *
 *  This routine checks for link status and updates statistics.
 *
 **********************************************************************/

static void
em_local_timer(void *arg)
{
	struct adapter	*adapter = arg;
	struct ifnet	*ifp = adapter->ifp;

	EM_LOCK_ASSERT(adapter);

	em_check_for_link(&adapter->hw);
	em_update_link_status(adapter);
	em_update_stats_counters(adapter);
	if (em_display_debug_stats && ifp->if_drv_flags & IFF_DRV_RUNNING)
		em_print_hw_stats(adapter);
	em_smartspeed(adapter);
	/*
	 * Each second we check the watchdog to 
	 * protect against hardware hangs.
	 */
	em_watchdog(adapter);

	callout_reset(&adapter->timer, hz, em_local_timer, adapter);
}

static void
em_update_link_status(struct adapter *adapter)
{
	struct ifnet *ifp = adapter->ifp;
	device_t dev = adapter->dev;

	if (E1000_READ_REG(&adapter->hw, STATUS) & E1000_STATUS_LU) {
		if (adapter->link_active == 0) {
			em_get_speed_and_duplex(&adapter->hw, &adapter->link_speed,
			    &adapter->link_duplex);
			/* Check if we may set SPEED_MODE bit on PCI-E */
			if ((adapter->link_speed == SPEED_1000) &&
			    ((adapter->hw.mac_type == em_82571) ||
			    (adapter->hw.mac_type == em_82572))) {
				int tarc0;

				tarc0 = E1000_READ_REG(&adapter->hw, TARC0);
				tarc0 |= SPEED_MODE_BIT;
				E1000_WRITE_REG(&adapter->hw, TARC0, tarc0);
			}
			if (bootverbose)
				device_printf(dev, "Link is up %d Mbps %s\n",
				    adapter->link_speed,
				    ((adapter->link_duplex == FULL_DUPLEX) ?
				    "Full Duplex" : "Half Duplex"));
			adapter->link_active = 1;
			adapter->smartspeed = 0;
			ifp->if_baudrate = adapter->link_speed * 1000000;
			if_link_state_change(ifp, LINK_STATE_UP);
		}
	} else {
		if (adapter->link_active == 1) {
			ifp->if_baudrate = adapter->link_speed = 0;
			adapter->link_duplex = 0;
			if (bootverbose)
				device_printf(dev, "Link is Down\n");
			adapter->link_active = 0;
			if_link_state_change(ifp, LINK_STATE_DOWN);
		}
	}
}

/*********************************************************************
 *
 *  This routine disables all traffic on the adapter by issuing a
 *  global reset on the MAC and deallocates TX/RX buffers.
 *
 **********************************************************************/

static void
em_stop(void *arg)
{
	struct adapter	*adapter = arg;
	struct ifnet	*ifp = adapter->ifp;

	EM_LOCK_ASSERT(adapter);

	INIT_DEBUGOUT("em_stop: begin");

	em_disable_intr(adapter);
	em_reset_hw(&adapter->hw);
	callout_stop(&adapter->timer);
	callout_stop(&adapter->tx_fifo_timer);

	/* Tell the stack that the interface is no longer active */
	ifp->if_drv_flags &= ~(IFF_DRV_RUNNING | IFF_DRV_OACTIVE);
}


/********************************************************************
 *
 *  Determine hardware revision.
 *
 **********************************************************************/
static void
em_identify_hardware(struct adapter *adapter)
{
	device_t dev = adapter->dev;

	/* Make sure our PCI config space has the necessary stuff set */
	pci_enable_busmaster(dev);
	pci_enable_io(dev, SYS_RES_MEMORY);
	adapter->hw.pci_cmd_word = pci_read_config(dev, PCIR_COMMAND, 2);

	/* Save off the information about this board */
	adapter->hw.vendor_id = pci_get_vendor(dev);
	adapter->hw.device_id = pci_get_device(dev);
	adapter->hw.revision_id = pci_get_revid(dev);
	adapter->hw.subsystem_vendor_id = pci_get_subvendor(dev);
	adapter->hw.subsystem_id = pci_get_subdevice(dev);

	/* Identify the MAC */
	if (em_set_mac_type(&adapter->hw))
		device_printf(dev, "Unknown MAC Type\n");
	
	if(adapter->hw.mac_type == em_82541 || adapter->hw.mac_type == em_82541_rev_2 ||
	   adapter->hw.mac_type == em_82547 || adapter->hw.mac_type == em_82547_rev_2)
		adapter->hw.phy_init_script = TRUE;
}

static int
em_allocate_pci_resources(struct adapter *adapter)
{
	device_t	dev = adapter->dev;
	int		val, rid;

	rid = PCIR_BAR(0);
	adapter->res_memory = bus_alloc_resource_any(dev, SYS_RES_MEMORY,
	    &rid, RF_ACTIVE);
	if (adapter->res_memory == NULL) {
		device_printf(dev, "Unable to allocate bus resource: memory\n");
		return (ENXIO);
	}
	adapter->osdep.mem_bus_space_tag =
	rman_get_bustag(adapter->res_memory);
	adapter->osdep.mem_bus_space_handle = rman_get_bushandle(adapter->res_memory);
	adapter->hw.hw_addr = (uint8_t *)&adapter->osdep.mem_bus_space_handle;

	if (adapter->hw.mac_type > em_82543) {
		/* Figure our where our IO BAR is ? */
		for (rid = PCIR_BAR(0); rid < PCIR_CIS;) {
			val = pci_read_config(dev, rid, 4);
			if (EM_BAR_TYPE(val) == EM_BAR_TYPE_IO) {
				adapter->io_rid = rid;
				break;
			}
			rid += 4;
			/* check for 64bit BAR */
			if (EM_BAR_MEM_TYPE(val) == EM_BAR_MEM_TYPE_64BIT)
				rid += 4;
		}
		if (rid >= PCIR_CIS) {
			device_printf(dev, "Unable to locate IO BAR\n");
			return (ENXIO);
		}
		adapter->res_ioport = bus_alloc_resource_any(dev, SYS_RES_IOPORT,
		    &adapter->io_rid, RF_ACTIVE);
		if (adapter->res_ioport == NULL) {
			device_printf(dev, "Unable to allocate bus resource: "
			    "ioport\n");
			return (ENXIO);
		}
		adapter->hw.io_base = 0;
		adapter->osdep.io_bus_space_tag = rman_get_bustag(adapter->res_ioport);
		adapter->osdep.io_bus_space_handle =
		    rman_get_bushandle(adapter->res_ioport);
	}

	/* For ICH8 we need to find the flash memory. */
	if (adapter->hw.mac_type == em_ich8lan) {
		rid = EM_FLASH;

		adapter->flash_mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY,
		    &rid, RF_ACTIVE);
		adapter->osdep.flash_bus_space_tag = rman_get_bustag(adapter->flash_mem);
		adapter->osdep.flash_bus_space_handle =
		    rman_get_bushandle(adapter->flash_mem);
	}

	val = pci_msi_count(dev);
	if (val == 1 && pci_alloc_msi(dev, &val) == 0) {
		rid = 1;
		adapter->msi = 1;
	} else
		rid = 0;
	adapter->res_interrupt = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
	    RF_SHAREABLE | RF_ACTIVE);
	if (adapter->res_interrupt == NULL) {
		device_printf(dev, "Unable to allocate bus resource: "
		    "interrupt\n");
		return (ENXIO);
	}

	adapter->hw.back = &adapter->osdep;

	return (0);
}

int
em_allocate_intr(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	int error;

	/* Manually turn off all interrupts */
	E1000_WRITE_REG(&adapter->hw, IMC, 0xffffffff);

#ifdef DEVICE_POLLING
	if (adapter->int_handler_tag == NULL && (error = bus_setup_intr(dev,
	    adapter->res_interrupt, INTR_TYPE_NET | INTR_MPSAFE, em_intr, adapter,
	    &adapter->int_handler_tag)) != 0) {
		device_printf(dev, "Failed to register interrupt handler");
		return (error);
	}
#else
	/*
	 * Try allocating a fast interrupt and the associated deferred
	 * processing contexts.
	 */
	TASK_INIT(&adapter->rxtx_task, 0, em_handle_rxtx, adapter);
	TASK_INIT(&adapter->link_task, 0, em_handle_link, adapter);
	adapter->tq = taskqueue_create_fast("em_taskq", M_NOWAIT,
	    taskqueue_thread_enqueue, &adapter->tq);
	taskqueue_start_threads(&adapter->tq, 1, PI_NET, "%s taskq",
	    device_get_nameunit(adapter->dev));
	if ((error = bus_setup_intr(dev, adapter->res_interrupt,
	    INTR_TYPE_NET | INTR_FAST, em_intr_fast, adapter,
	    &adapter->int_handler_tag)) != 0) {
		device_printf(dev, "Failed to register fast interrupt "
			    "handler: %d\n", error);
		taskqueue_free(adapter->tq);
		adapter->tq = NULL;
		return (error);
	}
#endif

	em_enable_intr(adapter);
	return (0);
}

static void
em_free_intr(struct adapter *adapter)
{
	device_t dev = adapter->dev;

	if (adapter->int_handler_tag != NULL) {
		bus_teardown_intr(dev, adapter->res_interrupt, adapter->int_handler_tag);
		adapter->int_handler_tag = NULL;
	}
	if (adapter->tq != NULL) {
		taskqueue_drain(adapter->tq, &adapter->rxtx_task);
		taskqueue_drain(taskqueue_fast, &adapter->link_task);
		taskqueue_free(adapter->tq);
		adapter->tq = NULL;
	}
}

static void
em_free_pci_resources(struct adapter *adapter)
{
	device_t dev = adapter->dev;

	if (adapter->res_interrupt != NULL)
		bus_release_resource(dev, SYS_RES_IRQ, adapter->msi ? 1 : 0,
		    adapter->res_interrupt);

	if (adapter->msi)
		pci_release_msi(dev);

	if (adapter->res_memory != NULL)
		bus_release_resource(dev, SYS_RES_MEMORY, PCIR_BAR(0),
		    adapter->res_memory);

	if (adapter->flash_mem != NULL)
		bus_release_resource(dev, SYS_RES_MEMORY, EM_FLASH,
		    adapter->flash_mem);

	if (adapter->res_ioport != NULL)
		bus_release_resource(dev, SYS_RES_IOPORT, adapter->io_rid,
		    adapter->res_ioport);
}

/*********************************************************************
 *
 *  Initialize the hardware to a configuration as specified by the
 *  adapter structure. The controller is reset, the EEPROM is
 *  verified, the MAC address is set, then the shared initialization
 *  routines are called.
 *
 **********************************************************************/
static int
em_hardware_init(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	uint16_t rx_buffer_size;

	INIT_DEBUGOUT("em_hardware_init: begin");
	/* Issue a global reset */
	em_reset_hw(&adapter->hw);

	/* When hardware is reset, fifo_head is also reset */
	adapter->tx_fifo_head = 0;

	/* Make sure we have a good EEPROM before we read from it */
	if (em_validate_eeprom_checksum(&adapter->hw) < 0) {
		device_printf(dev, "The EEPROM Checksum Is Not Valid\n");
		return (EIO);
	}

	if (em_read_part_num(&adapter->hw, &(adapter->part_num)) < 0) {
		device_printf(dev, "EEPROM read error while reading part "
		    "number\n");
		return (EIO);
	}

	/* Set up smart power down as default off on newer adapters. */
	if (!em_smart_pwr_down &&
	    (adapter->hw.mac_type == em_82571 || adapter->hw.mac_type == em_82572)) {
		uint16_t phy_tmp = 0;

		/* Speed up time to link by disabling smart power down. */
		em_read_phy_reg(&adapter->hw, IGP02E1000_PHY_POWER_MGMT, &phy_tmp);
		phy_tmp &= ~IGP02E1000_PM_SPD;
		em_write_phy_reg(&adapter->hw, IGP02E1000_PHY_POWER_MGMT, phy_tmp);
	}

	/*
	 * These parameters control the automatic generation (Tx) and
	 * response (Rx) to Ethernet PAUSE frames.
	 * - High water mark should allow for at least two frames to be
	 *   received after sending an XOFF.
	 * - Low water mark works best when it is very near the high water mark.
	 *   This allows the receiver to restart by sending XON when it has
	 *   drained a bit. Here we use an arbitary value of 1500 which will
	 *   restart after one full frame is pulled from the buffer. There
	 *   could be several smaller frames in the buffer and if so they will
	 *   not trigger the XON until their total number reduces the buffer
	 *   by 1500.
	 * - The pause time is fairly large at 1000 x 512ns = 512 usec.
	 */
	rx_buffer_size = ((E1000_READ_REG(&adapter->hw, PBA) & 0xffff) << 10 );

	adapter->hw.fc_high_water = rx_buffer_size -
	    roundup2(adapter->hw.max_frame_size, 1024);
	adapter->hw.fc_low_water = adapter->hw.fc_high_water - 1500;
	if (adapter->hw.mac_type == em_80003es2lan)
		adapter->hw.fc_pause_time = 0xFFFF;
	else
		adapter->hw.fc_pause_time = 0x1000;
	adapter->hw.fc_send_xon = TRUE;
	adapter->hw.fc = E1000_FC_FULL;

	if (em_init_hw(&adapter->hw) < 0) {
		device_printf(dev, "Hardware Initialization Failed");
		return (EIO);
	}

	em_check_for_link(&adapter->hw);

	return (0);
}

/*********************************************************************
 *
 *  Setup networking device structure and register an interface.
 *
 **********************************************************************/
static void
em_setup_interface(device_t dev, struct adapter *adapter)
{
	struct ifnet   *ifp;
	INIT_DEBUGOUT("em_setup_interface: begin");

	ifp = adapter->ifp = if_alloc(IFT_ETHER);
	if (ifp == NULL)
		panic("%s: can not if_alloc()", device_get_nameunit(dev));
	if_initname(ifp, device_get_name(dev), device_get_unit(dev));
	ifp->if_mtu = ETHERMTU;
	ifp->if_init =  em_init;
	ifp->if_softc = adapter;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = em_ioctl;
	ifp->if_start = em_start;
	IFQ_SET_MAXLEN(&ifp->if_snd, adapter->num_tx_desc - 1);
	ifp->if_snd.ifq_drv_maxlen = adapter->num_tx_desc - 1;
	IFQ_SET_READY(&ifp->if_snd);

	ether_ifattach(ifp, adapter->hw.mac_addr);

	ifp->if_capabilities = ifp->if_capenable = 0;

	if (adapter->hw.mac_type >= em_82543) {
		ifp->if_capabilities |= IFCAP_HWCSUM | IFCAP_VLAN_HWCSUM;
		ifp->if_capenable |= IFCAP_HWCSUM | IFCAP_VLAN_HWCSUM;
	}

	/* Enable TSO if available */
	if ((adapter->hw.mac_type > em_82544) &&
	    (adapter->hw.mac_type != em_82547)) {
		ifp->if_capabilities |= IFCAP_TSO4;
		ifp->if_capenable |= IFCAP_TSO4;
	}

	/*
	 * Tell the upper layer(s) we support long frames.
	 */
	ifp->if_data.ifi_hdrlen = sizeof(struct ether_vlan_header);
	ifp->if_capabilities |= IFCAP_VLAN_HWTAGGING | IFCAP_VLAN_MTU;
	ifp->if_capenable |= IFCAP_VLAN_HWTAGGING | IFCAP_VLAN_MTU;

#ifdef DEVICE_POLLING
	ifp->if_capabilities |= IFCAP_POLLING;
#endif

	/*
	 * Specify the media types supported by this adapter and register
	 * callbacks to update media and link information
	 */
	ifmedia_init(&adapter->media, IFM_IMASK, em_media_change,
	    em_media_status);
	if ((adapter->hw.media_type == em_media_type_fiber) ||
	    (adapter->hw.media_type == em_media_type_internal_serdes)) {
		u_char fiber_type = IFM_1000_SX;	/* default type; */

		if (adapter->hw.mac_type == em_82545)
			fiber_type = IFM_1000_LX;
		ifmedia_add(&adapter->media, IFM_ETHER | fiber_type | IFM_FDX,
		    0, NULL);
		ifmedia_add(&adapter->media, IFM_ETHER | fiber_type, 0, NULL);
	} else {
		ifmedia_add(&adapter->media, IFM_ETHER | IFM_10_T, 0, NULL);
		ifmedia_add(&adapter->media, IFM_ETHER | IFM_10_T | IFM_FDX,
			    0, NULL);
		ifmedia_add(&adapter->media, IFM_ETHER | IFM_100_TX,
			    0, NULL);
		ifmedia_add(&adapter->media, IFM_ETHER | IFM_100_TX | IFM_FDX,
			    0, NULL);
		if (adapter->hw.phy_type != em_phy_ife) {
			ifmedia_add(&adapter->media,
				IFM_ETHER | IFM_1000_T | IFM_FDX, 0, NULL);
			ifmedia_add(&adapter->media,
				IFM_ETHER | IFM_1000_T, 0, NULL);
		}
	}
	ifmedia_add(&adapter->media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&adapter->media, IFM_ETHER | IFM_AUTO);
}


/*********************************************************************
 *
 *  Workaround for SmartSpeed on 82541 and 82547 controllers
 *
 **********************************************************************/
static void
em_smartspeed(struct adapter *adapter)
{
	uint16_t phy_tmp;

	if (adapter->link_active || (adapter->hw.phy_type != em_phy_igp) ||
	    adapter->hw.autoneg == 0 ||
	    (adapter->hw.autoneg_advertised & ADVERTISE_1000_FULL) == 0)
		return;

	if (adapter->smartspeed == 0) {
		/* If Master/Slave config fault is asserted twice,
		 * we assume back-to-back */
		em_read_phy_reg(&adapter->hw, PHY_1000T_STATUS, &phy_tmp);
		if (!(phy_tmp & SR_1000T_MS_CONFIG_FAULT))
			return;
		em_read_phy_reg(&adapter->hw, PHY_1000T_STATUS, &phy_tmp);
		if (phy_tmp & SR_1000T_MS_CONFIG_FAULT) {
			em_read_phy_reg(&adapter->hw, PHY_1000T_CTRL, &phy_tmp);
			if(phy_tmp & CR_1000T_MS_ENABLE) {
				phy_tmp &= ~CR_1000T_MS_ENABLE;
				em_write_phy_reg(&adapter->hw, PHY_1000T_CTRL,
				    phy_tmp);
				adapter->smartspeed++;
				if(adapter->hw.autoneg &&
				   !em_phy_setup_autoneg(&adapter->hw) &&
				   !em_read_phy_reg(&adapter->hw, PHY_CTRL,
				    &phy_tmp)) {
					phy_tmp |= (MII_CR_AUTO_NEG_EN |
						    MII_CR_RESTART_AUTO_NEG);
					em_write_phy_reg(&adapter->hw, PHY_CTRL,
					    phy_tmp);
				}
			}
		}
		return;
	} else if(adapter->smartspeed == EM_SMARTSPEED_DOWNSHIFT) {
		/* If still no link, perhaps using 2/3 pair cable */
		em_read_phy_reg(&adapter->hw, PHY_1000T_CTRL, &phy_tmp);
		phy_tmp |= CR_1000T_MS_ENABLE;
		em_write_phy_reg(&adapter->hw, PHY_1000T_CTRL, phy_tmp);
		if(adapter->hw.autoneg &&
		   !em_phy_setup_autoneg(&adapter->hw) &&
		   !em_read_phy_reg(&adapter->hw, PHY_CTRL, &phy_tmp)) {
			phy_tmp |= (MII_CR_AUTO_NEG_EN |
				    MII_CR_RESTART_AUTO_NEG);
			em_write_phy_reg(&adapter->hw, PHY_CTRL, phy_tmp);
		}
	}
	/* Restart process after EM_SMARTSPEED_MAX iterations */
	if(adapter->smartspeed++ == EM_SMARTSPEED_MAX)
		adapter->smartspeed = 0;
}


/*
 * Manage DMA'able memory.
 */
static void
em_dmamap_cb(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{
	if (error)
		return;
	*(bus_addr_t *) arg = segs[0].ds_addr;
}

static int
em_dma_malloc(struct adapter *adapter, bus_size_t size, struct em_dma_alloc *dma,
	int mapflags)
{
	int error;

	error = bus_dma_tag_create(bus_get_dma_tag(adapter->dev), /* parent */
				EM_DBA_ALIGN, 0,	/* alignment, bounds */
				BUS_SPACE_MAXADDR,	/* lowaddr */
				BUS_SPACE_MAXADDR,	/* highaddr */
				NULL, NULL,		/* filter, filterarg */
				size,			/* maxsize */
				1,			/* nsegments */
				size,			/* maxsegsize */
				0,			/* flags */
				NULL,			/* lockfunc */
				NULL,			/* lockarg */
				&dma->dma_tag);
	if (error) {
		device_printf(adapter->dev, "%s: bus_dma_tag_create failed: %d\n",
		    __func__, error);
		goto fail_0;
	}

	error = bus_dmamem_alloc(dma->dma_tag, (void**) &dma->dma_vaddr,
	    BUS_DMA_NOWAIT, &dma->dma_map);
	if (error) {
		device_printf(adapter->dev, "%s: bus_dmamem_alloc(%ju) failed: %d\n",
		    __func__, (uintmax_t)size, error);
		goto fail_2;
	}

	dma->dma_paddr = 0;
	error = bus_dmamap_load(dma->dma_tag, dma->dma_map, dma->dma_vaddr,
	    size, em_dmamap_cb, &dma->dma_paddr, mapflags | BUS_DMA_NOWAIT);
	if (error || dma->dma_paddr == 0) {
		device_printf(adapter->dev, "%s: bus_dmamap_load failed: %d\n",
		    __func__, error);
		goto fail_3;
	}

	return (0);

fail_3:
	bus_dmamap_unload(dma->dma_tag, dma->dma_map);
fail_2:
	bus_dmamem_free(dma->dma_tag, dma->dma_vaddr, dma->dma_map);
	bus_dma_tag_destroy(dma->dma_tag);
fail_0:
	dma->dma_map = NULL;
	dma->dma_tag = NULL;

	return (error);
}

static void
em_dma_free(struct adapter *adapter, struct em_dma_alloc *dma)
{
	if (dma->dma_tag == NULL)
		return;
	if (dma->dma_map != NULL) {
		bus_dmamap_sync(dma->dma_tag, dma->dma_map,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dma->dma_tag, dma->dma_map);
		bus_dmamem_free(dma->dma_tag, dma->dma_vaddr, dma->dma_map);
		dma->dma_map = NULL;
	}
	bus_dma_tag_destroy(dma->dma_tag);
	dma->dma_tag = NULL;
}


/*********************************************************************
 *
 *  Allocate memory for tx_buffer structures. The tx_buffer stores all
 *  the information needed to transmit a packet on the wire.
 *
 **********************************************************************/
static int
em_allocate_transmit_structures(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	struct em_buffer *tx_buffer;
	bus_size_t size, segsize;
	int error, i;

	/*
	 * Setup DMA descriptor areas.
	 */
	segsize = size = roundup2(adapter->hw.max_frame_size, MCLBYTES);

	/* Overrides for TSO - want large sizes */
	if ((adapter->hw.mac_type > em_82544) &&
	    (adapter->hw.mac_type != em_82547)) {
		size = EM_TSO_SIZE;
		segsize = EM_TSO_PCIE_SEGMENT_SIZE;
	}

	if ((error = bus_dma_tag_create(bus_get_dma_tag(dev),	/* parent */
				1, 0,			/* alignment, bounds */
				BUS_SPACE_MAXADDR,	/* lowaddr */
				BUS_SPACE_MAXADDR,	/* highaddr */
				NULL, NULL,		/* filter, filterarg */
				size,			/* maxsize */
				EM_MAX_SCATTER,		/* nsegments */
				segsize,		/* maxsegsize */
				0,			/* flags */
				NULL,		/* lockfunc */
				NULL,		/* lockarg */
				&adapter->txtag)) != 0) {
		device_printf(dev, "Unable to allocate TX DMA tag\n");
		goto fail;
	}

	adapter->tx_buffer_area = malloc(sizeof(struct em_buffer) *
	    adapter->num_tx_desc, M_DEVBUF, M_NOWAIT | M_ZERO);
	if (adapter->tx_buffer_area == NULL) {
		device_printf(dev, "Unable to allocate tx_buffer memory\n");
		error = ENOMEM;
		goto fail;
	}

	tx_buffer = adapter->tx_buffer_area;
	for (i = 0; i < adapter->num_tx_desc; i++) {
		error = bus_dmamap_create(adapter->txtag, 0, &tx_buffer->map);
		if (error != 0) {
			device_printf(dev, "Unable to create TX DMA map\n");
			goto fail;
		}
		tx_buffer++;
	}

	return (0);

fail:
	em_free_transmit_structures(adapter);
	return (error);
}

/*********************************************************************
 *
 *  Initialize transmit structures.
 *
 **********************************************************************/
static void
em_setup_transmit_structures(struct adapter *adapter)
{
	struct em_buffer *tx_buffer;
	int i;

	bzero(adapter->tx_desc_base, (sizeof(struct em_tx_desc)) * adapter->num_tx_desc);

	adapter->next_avail_tx_desc = 0;
	adapter->next_tx_to_clean = 0;

	/* Free any existing tx buffers. */
	tx_buffer = adapter->tx_buffer_area;
	for (i = 0; i < adapter->num_tx_desc; i++, tx_buffer++) {
		if (tx_buffer->m_head != NULL) {
			bus_dmamap_sync(adapter->txtag, tx_buffer->map,
			    BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(adapter->txtag, tx_buffer->map);
			m_freem(tx_buffer->m_head);
			tx_buffer->m_head = NULL;
		}
	}

	/* Set number of descriptors available */
	adapter->num_tx_desc_avail = adapter->num_tx_desc;

	/* Set checksum context */
	adapter->active_checksum_context = OFFLOAD_NONE;
	bus_dmamap_sync(adapter->txdma.dma_tag, adapter->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

/*********************************************************************
 *
 *  Enable transmit unit.
 *
 **********************************************************************/
static void
em_initialize_transmit_unit(struct adapter *adapter)
{
	uint32_t	reg_tctl;
	uint32_t	reg_tipg = 0;
	uint64_t	bus_addr;

	 INIT_DEBUGOUT("em_initialize_transmit_unit: begin");
	/* Setup the Base and Length of the Tx Descriptor Ring */
	bus_addr = adapter->txdma.dma_paddr;
	E1000_WRITE_REG(&adapter->hw, TDLEN,
	    adapter->num_tx_desc * sizeof(struct em_tx_desc));
	E1000_WRITE_REG(&adapter->hw, TDBAH, (uint32_t)(bus_addr >> 32));
	E1000_WRITE_REG(&adapter->hw, TDBAL, (uint32_t)bus_addr);

	/* Setup the HW Tx Head and Tail descriptor pointers */
	E1000_WRITE_REG(&adapter->hw, TDT, 0);
	E1000_WRITE_REG(&adapter->hw, TDH, 0);


	HW_DEBUGOUT2("Base = %x, Length = %x\n", E1000_READ_REG(&adapter->hw, TDBAL),
	    E1000_READ_REG(&adapter->hw, TDLEN));

	/* Set the default values for the Tx Inter Packet Gap timer */
	switch (adapter->hw.mac_type) {
	case em_82542_rev2_0:
	case em_82542_rev2_1:
		reg_tipg = DEFAULT_82542_TIPG_IPGT;
		reg_tipg |= DEFAULT_82542_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
		reg_tipg |= DEFAULT_82542_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
		break;
	case em_80003es2lan:
		reg_tipg = DEFAULT_82543_TIPG_IPGR1;
		reg_tipg |= DEFAULT_80003ES2LAN_TIPG_IPGR2 <<
		    E1000_TIPG_IPGR2_SHIFT;
		break;
	default:
		if ((adapter->hw.media_type == em_media_type_fiber) ||
		    (adapter->hw.media_type == em_media_type_internal_serdes))
			reg_tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
		else
			reg_tipg = DEFAULT_82543_TIPG_IPGT_COPPER;
		reg_tipg |= DEFAULT_82543_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT;
		reg_tipg |= DEFAULT_82543_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT;
	}

	E1000_WRITE_REG(&adapter->hw, TIPG, reg_tipg);
	E1000_WRITE_REG(&adapter->hw, TIDV, adapter->tx_int_delay.value);
	if(adapter->hw.mac_type >= em_82540)
		E1000_WRITE_REG(&adapter->hw, TADV, adapter->tx_abs_int_delay.value);

	/* Program the Transmit Control Register */
	reg_tctl = E1000_TCTL_PSP | E1000_TCTL_EN |
		   (E1000_COLLISION_THRESHOLD << E1000_CT_SHIFT);
	if (adapter->hw.mac_type >= em_82571)
		reg_tctl |= E1000_TCTL_MULR;
	if (adapter->link_duplex == FULL_DUPLEX) {
		reg_tctl |= E1000_FDX_COLLISION_DISTANCE << E1000_COLD_SHIFT;
	} else {
		reg_tctl |= E1000_HDX_COLLISION_DISTANCE << E1000_COLD_SHIFT;
	}
	/* This write will effectively turn on the transmit unit. */
	E1000_WRITE_REG(&adapter->hw, TCTL, reg_tctl);

	/* Setup Transmit Descriptor Base Settings */
	adapter->txd_cmd = E1000_TXD_CMD_IFCS;

	if (adapter->tx_int_delay.value > 0)
		adapter->txd_cmd |= E1000_TXD_CMD_IDE;
}

/*********************************************************************
 *
 *  Free all transmit related data structures.
 *
 **********************************************************************/
static void
em_free_transmit_structures(struct adapter *adapter)
{
	struct em_buffer *tx_buffer;
	int i;

	INIT_DEBUGOUT("free_transmit_structures: begin");

	if (adapter->tx_buffer_area != NULL) {
		tx_buffer = adapter->tx_buffer_area;
		for (i = 0; i < adapter->num_tx_desc; i++, tx_buffer++) {
			if (tx_buffer->m_head != NULL) {
				bus_dmamap_sync(adapter->txtag, tx_buffer->map,
				    BUS_DMASYNC_POSTWRITE);
				bus_dmamap_unload(adapter->txtag,
				    tx_buffer->map);
				m_freem(tx_buffer->m_head);
				tx_buffer->m_head = NULL;
			} else if (tx_buffer->map != NULL)
				bus_dmamap_unload(adapter->txtag,
				    tx_buffer->map);
			if (tx_buffer->map != NULL) {
				bus_dmamap_destroy(adapter->txtag,
				    tx_buffer->map);
				tx_buffer->map = NULL;
			}
		}
	}
	if (adapter->tx_buffer_area != NULL) {
		free(adapter->tx_buffer_area, M_DEVBUF);
		adapter->tx_buffer_area = NULL;
	}
	if (adapter->txtag != NULL) {
		bus_dma_tag_destroy(adapter->txtag);
		adapter->txtag = NULL;
	}
}

/*********************************************************************
 *
 *  The offload context needs to be set when we transfer the first
 *  packet of a particular protocol (TCP/UDP). We change the
 *  context only if the protocol type changes.
 *
 **********************************************************************/
static void
em_transmit_checksum_setup(struct adapter *adapter, struct mbuf *mp,
    uint32_t *txd_upper, uint32_t *txd_lower)
{
	struct em_context_desc *TXD;
	struct em_buffer *tx_buffer;
	struct ether_vlan_header *eh;
	struct ip *ip;
	struct ip6_hdr *ip6;
	struct tcp_hdr *th;
	int curr_txd, ehdrlen, hdr_len, ip_hlen;
	uint32_t cmd = 0;
	uint16_t etype;
	uint8_t ipproto;

	/* Setup checksum offload context. */
	curr_txd = adapter->next_avail_tx_desc;
	tx_buffer = &adapter->tx_buffer_area[curr_txd];
	TXD = (struct em_context_desc *) &adapter->tx_desc_base[curr_txd];

	*txd_lower = E1000_TXD_CMD_DEXT |	/* Extended descr type */
		     E1000_TXD_DTYP_D;		/* Data descr */

	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present,
	 * helpful for QinQ too.
	 */
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		etype = ntohs(eh->evl_proto);
		ehdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	} else {
		etype = ntohs(eh->evl_encap_proto);
		ehdrlen = ETHER_HDR_LEN;
	}

	/*
	 * We only support TCP/UDP for IPv4 and IPv6 for the moment.
	 * TODO: Support SCTP too when it hits the tree.
	 */
	switch (etype) {
	case ETHERTYPE_IP:
		ip = (struct ip *)(mp->m_data + ehdrlen);
		ip_hlen = ip->ip_hl << 2;

		/* Setup of IP header checksum. */
		if (mp->m_pkthdr.csum_flags & CSUM_IP) {
			/*
			 * Start offset for header checksum calculation.
			 * End offset for header checksum calculation.
			 * Offset of place to put the checksum.
			 */
			TXD->lower_setup.ip_fields.ipcss = ehdrlen;
			TXD->lower_setup.ip_fields.ipcse =
			    htole16(ehdrlen + ip_hlen);
			TXD->lower_setup.ip_fields.ipcso =
			    ehdrlen + offsetof(struct ip, ip_sum);
			cmd |= E1000_TXD_CMD_IP;
			*txd_upper |= E1000_TXD_POPTS_IXSM << 8;
		}

		if (mp->m_len < ehdrlen + ip_hlen)
			return;	/* failure */

		hdr_len = ehdrlen + ip_hlen;
		ipproto = ip->ip_p;

		break;
	case ETHERTYPE_IPV6:
		ip6 = (struct ip6_hdr *)(mp->m_data + ehdrlen);
		ip_hlen = sizeof(struct ip6_hdr); /* XXX: No header stacking. */

		if (mp->m_len < ehdrlen + ip_hlen)
			return;	/* failure */

		/* IPv6 doesn't have a header checksum. */

		hdr_len = ehdrlen + ip_hlen;
		ipproto = ip6->ip6_nxt;

		break;
	default:
		*txd_upper = 0;
		*txd_lower = 0;
		return;
	}

	switch (ipproto) {
	case IPPROTO_TCP:
		if (mp->m_pkthdr.csum_flags & CSUM_TCP) {
			/*
			 * Start offset for payload checksum calculation.
			 * End offset for payload checksum calculation.
			 * Offset of place to put the checksum.
			 */
			th = (struct tcp_hdr *)(mp->m_data + hdr_len);
			TXD->upper_setup.tcp_fields.tucss = hdr_len;
			TXD->upper_setup.tcp_fields.tucse = htole16(0);
			TXD->upper_setup.tcp_fields.tucso =
			    hdr_len + offsetof(struct tcphdr, th_sum);
			cmd |= E1000_TXD_CMD_TCP;
			*txd_upper |= E1000_TXD_POPTS_TXSM << 8;
		}
		break;
	case IPPROTO_UDP:
		if (mp->m_pkthdr.csum_flags & CSUM_UDP) {
			/*
			 * Start offset for header checksum calculation.
			 * End offset for header checksum calculation.
			 * Offset of place to put the checksum.
			 */
			TXD->upper_setup.tcp_fields.tucss = hdr_len;
			TXD->upper_setup.tcp_fields.tucse = htole16(0);
			TXD->upper_setup.tcp_fields.tucso =
			    hdr_len + offsetof(struct udphdr, uh_sum);
			*txd_upper |= E1000_TXD_POPTS_TXSM << 8;
		}
		break;
	default:
		break;
	}

	TXD->tcp_seg_setup.data = htole32(0);
	TXD->cmd_and_length =
	    htole32(adapter->txd_cmd | E1000_TXD_CMD_DEXT | cmd);
	tx_buffer->m_head = NULL;
	tx_buffer->next_eop = -1;

	if (++curr_txd == adapter->num_tx_desc)
		curr_txd = 0;

	adapter->num_tx_desc_avail--;
	adapter->next_avail_tx_desc = curr_txd;
}

/**********************************************************************
 *
 *  Setup work for hardware segmentation offload (TSO)
 *
 **********************************************************************/
static boolean_t
em_tso_setup(struct adapter *adapter, struct mbuf *mp, uint32_t *txd_upper,
   uint32_t *txd_lower)
{
	struct em_context_desc *TXD;
	struct em_buffer *tx_buffer;
	struct ether_vlan_header *eh;
	struct ip *ip;
	struct ip6_hdr *ip6;
	struct tcphdr *th;
	int curr_txd, ehdrlen, hdr_len, ip_hlen, isip6;
	uint16_t etype;

	/*
	 * XXX: This is not really correct as the stack would not have
	 * set up all checksums.
	 * XXX: Return FALSE is not sufficient as we may have to return
	 * in true failure cases as well.  Should do -1 (failure), 0 (no)
	 * and 1 (success).
	 */
	if (mp->m_pkthdr.len <= E1000_TX_BUFFER_SIZE)
		return FALSE;	/* 0 */

	/*
	 * This function could/should be extended to support IP/IPv6
	 * fragmentation as well.  But as they say, one step at a time.
	 */

	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present,
	 * helpful for QinQ too.
	 */
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		etype = ntohs(eh->evl_proto);
		ehdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	} else {
		etype = ntohs(eh->evl_encap_proto);
		ehdrlen = ETHER_HDR_LEN;
	}

	/* Ensure we have at least the IP+TCP header in the first mbuf. */
	if (mp->m_len < ehdrlen + sizeof(struct ip) + sizeof(struct tcphdr))
		return FALSE;	/* -1 */

	/*
	 * We only support TCP for IPv4 and IPv6 (notyet) for the moment.
	 * TODO: Support SCTP too when it hits the tree.
	 */
	switch (etype) {
	case ETHERTYPE_IP:
		isip6 = 0;
		ip = (struct ip *)(mp->m_data + ehdrlen);
		if (ip->ip_p != IPPROTO_TCP)
			return FALSE;	/* 0 */
		ip->ip_len = 0;
		ip->ip_sum = 0;
		ip_hlen = ip->ip_hl << 2;
		if (mp->m_len < ehdrlen + ip_hlen + sizeof(struct tcphdr))
			return FALSE;	/* -1 */
		th = (struct tcphdr *)((caddr_t)ip + ip_hlen);
#if 1
		th->th_sum = in_pseudo(ip->ip_src.s_addr,
		    ip->ip_dst.s_addr, htons(IPPROTO_TCP));
#else
		th->th_sum = mp->m_pkthdr.csum_data;
#endif
		break;
	case ETHERTYPE_IPV6:
		isip6 = 1;
		return FALSE;			/* Not supported yet. */
		ip6 = (struct ip6_hdr *)(mp->m_data + ehdrlen);
		if (ip6->ip6_nxt != IPPROTO_TCP)
			return FALSE;	/* 0 */
		ip6->ip6_plen = 0;
		ip_hlen = sizeof(struct ip6_hdr); /* XXX: no header stacking. */
		if (mp->m_len < ehdrlen + ip_hlen + sizeof(struct tcphdr))
			return FALSE;	/* -1 */
		th = (struct tcphdr *)((caddr_t)ip6 + ip_hlen);
#if 0
		th->th_sum = in6_pseudo(ip6->ip6_src, ip->ip6_dst,
		    htons(IPPROTO_TCP));	/* XXX: function notyet. */
#else
		th->th_sum = mp->m_pkthdr.csum_data;
#endif
		break;
	default:
		return FALSE;
	}
	hdr_len = ehdrlen + ip_hlen + (th->th_off << 2);

	*txd_lower = (E1000_TXD_CMD_DEXT |	/* Extended descr type */
		      E1000_TXD_DTYP_D |	/* Data descr type */
		      E1000_TXD_CMD_TSE);	/* Do TSE on this packet */

	/* IP and/or TCP header checksum calculation and insertion. */
	*txd_upper = ((isip6 ? 0 : E1000_TXD_POPTS_IXSM) |
		      E1000_TXD_POPTS_TXSM) << 8;

	curr_txd = adapter->next_avail_tx_desc;
	tx_buffer = &adapter->tx_buffer_area[curr_txd];
	TXD = (struct em_context_desc *) &adapter->tx_desc_base[curr_txd];

	/* IPv6 doesn't have a header checksum. */
	if (!isip6) {
		/*
		 * Start offset for header checksum calculation.
		 * End offset for header checksum calculation.
		 * Offset of place put the checksum.
		 */
		TXD->lower_setup.ip_fields.ipcss = ehdrlen;
		TXD->lower_setup.ip_fields.ipcse =
		    htole16(ehdrlen + ip_hlen - 1);
		TXD->lower_setup.ip_fields.ipcso =
		    ehdrlen + offsetof(struct ip, ip_sum);
	}
	/*
	 * Start offset for payload checksum calculation.
	 * End offset for payload checksum calculation.
	 * Offset of place to put the checksum.
	 */
	TXD->upper_setup.tcp_fields.tucss =
	    ehdrlen + ip_hlen;
	TXD->upper_setup.tcp_fields.tucse = 0;
	TXD->upper_setup.tcp_fields.tucso =
	    ehdrlen + ip_hlen + offsetof(struct tcphdr, th_sum);
	/*
	 * Payload size per packet w/o any headers.
	 * Length of all headers up to payload.
	 */
	TXD->tcp_seg_setup.fields.mss = htole16(mp->m_pkthdr.tso_segsz);
	TXD->tcp_seg_setup.fields.hdr_len = hdr_len;

	TXD->cmd_and_length = htole32(adapter->txd_cmd |
				E1000_TXD_CMD_DEXT |	/* Extended descr */
				E1000_TXD_CMD_TSE |	/* TSE context */
				(isip6 ? 0 : E1000_TXD_CMD_IP) | /* Do IP csum */
				E1000_TXD_CMD_TCP |	/* Do TCP checksum */
				(mp->m_pkthdr.len - (hdr_len))); /* Total len */

	tx_buffer->m_head = NULL;

	if (++curr_txd == adapter->num_tx_desc)
		curr_txd = 0;

	adapter->num_tx_desc_avail--;
	adapter->next_avail_tx_desc = curr_txd;
	adapter->tx_tso = TRUE;

	return TRUE;
}

/**********************************************************************
 *
 *  Examine each tx_buffer in the used queue. If the hardware is done
 *  processing the packet then free associated resources. The
 *  tx_buffer is put back on the free queue.
 *
 **********************************************************************/
static void
em_txeof(struct adapter *adapter)
{
	int first, last, done, num_avail;
	struct em_buffer *tx_buffer;
	struct em_tx_desc   *tx_desc, *eop_desc;
	struct ifnet   *ifp = adapter->ifp;

	EM_LOCK_ASSERT(adapter);

	if (adapter->num_tx_desc_avail == adapter->num_tx_desc)
		return;

	num_avail = adapter->num_tx_desc_avail;
	first = adapter->next_tx_to_clean;
	tx_desc = &adapter->tx_desc_base[first];
	tx_buffer = &adapter->tx_buffer_area[first];
	last = tx_buffer->next_eop;
	eop_desc = &adapter->tx_desc_base[last];

	/*
	 * Now calculate the terminating index
	 * for the cleanup loop below.
	 */
	if (++last == adapter->num_tx_desc)
		last = 0;
	done = last;

	bus_dmamap_sync(adapter->txdma.dma_tag, adapter->txdma.dma_map,
	    BUS_DMASYNC_POSTREAD);
	while (eop_desc->upper.fields.status & E1000_TXD_STAT_DD) {
		/* We clean the range of the packet */
		while (first != done) {
			tx_desc->upper.data = 0;
			tx_desc->lower.data = 0;
			num_avail++;

			if (tx_buffer->m_head) {
				ifp->if_opackets++;
				bus_dmamap_sync(adapter->txtag, tx_buffer->map,
				    BUS_DMASYNC_POSTWRITE);
				bus_dmamap_unload(adapter->txtag,
				    tx_buffer->map);

				m_freem(tx_buffer->m_head);
				tx_buffer->m_head = NULL;
			}
			tx_buffer->next_eop = -1;

			if (++first == adapter->num_tx_desc)
				first = 0;

			tx_buffer = &adapter->tx_buffer_area[first];
			tx_desc = &adapter->tx_desc_base[first];
		}
		/* See if we can continue to the next packet */
		last = tx_buffer->next_eop;
		if (last != -1) {
			eop_desc = &adapter->tx_desc_base[last];
			/* Get new done point */
			if (++last == adapter->num_tx_desc)
				last = 0;
			done = last;
		} else
			break;
	}
	bus_dmamap_sync(adapter->txdma.dma_tag, adapter->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	adapter->next_tx_to_clean = first;

	/*
	 * If we have enough room, clear IFF_DRV_OACTIVE to tell the stack
	 * that it is OK to send packets.
	 * If there are no pending descriptors, clear the timeout. Otherwise,
	 * if some descriptors have been freed, restart the timeout.
	 */
	if (num_avail > EM_TX_CLEANUP_THRESHOLD) {
		ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
		if (num_avail == adapter->num_tx_desc)
			adapter->watchdog_timer = 0;
		else if (num_avail != adapter->num_tx_desc_avail)
			adapter->watchdog_timer = EM_TX_TIMEOUT;
	}
	adapter->num_tx_desc_avail = num_avail;
}

/*********************************************************************
 *
 *  Get a buffer from system mbuf buffer pool.
 *
 **********************************************************************/
static int
em_get_buf(struct adapter *adapter, int i)
{
	struct mbuf		*m;
	bus_dma_segment_t	segs[1];
	bus_dmamap_t		map;
	struct em_buffer	*rx_buffer;
	int			error, nsegs;

	m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
	if (m == NULL) {
		adapter->mbuf_cluster_failed++;
		return (ENOBUFS);
	}
	m->m_len = m->m_pkthdr.len = MCLBYTES;
	if (adapter->hw.max_frame_size <= (MCLBYTES - ETHER_ALIGN))
		m_adj(m, ETHER_ALIGN);

	/*
	 * Using memory from the mbuf cluster pool, invoke the
	 * bus_dma machinery to arrange the memory mapping.
	 */
	error = bus_dmamap_load_mbuf_sg(adapter->rxtag, adapter->rx_sparemap,
	    m, segs, &nsegs, BUS_DMA_NOWAIT);
	if (error != 0) {
		m_free(m);
		return (error);
	}
	/* If nsegs is wrong then the stack is corrupt. */
	KASSERT(nsegs == 1, ("Too many segments returned!"));

	rx_buffer = &adapter->rx_buffer_area[i];
	if (rx_buffer->m_head != NULL)
		bus_dmamap_unload(adapter->rxtag, rx_buffer->map);

	map = rx_buffer->map;
	rx_buffer->map = adapter->rx_sparemap;
	adapter->rx_sparemap = map;
	bus_dmamap_sync(adapter->rxtag, rx_buffer->map, BUS_DMASYNC_PREREAD);
	rx_buffer->m_head = m;

	adapter->rx_desc_base[i].buffer_addr = htole64(segs[0].ds_addr);

	return (0);
}

/*********************************************************************
 *
 *  Allocate memory for rx_buffer structures. Since we use one
 *  rx_buffer per received packet, the maximum number of rx_buffer's
 *  that we'll need is equal to the number of receive descriptors
 *  that we've allocated.
 *
 **********************************************************************/
static int
em_allocate_receive_structures(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	struct em_buffer *rx_buffer;
	int i, error;

	adapter->rx_buffer_area = malloc(sizeof(struct em_buffer) * adapter->num_rx_desc,
	    M_DEVBUF, M_NOWAIT);
	if (adapter->rx_buffer_area == NULL) {
		device_printf(dev, "Unable to allocate rx_buffer memory\n");
		return (ENOMEM);
	}

	bzero(adapter->rx_buffer_area, sizeof(struct em_buffer) * adapter->num_rx_desc);

	error = bus_dma_tag_create(bus_get_dma_tag(dev),	/* parent */
				1, 0,			/* alignment, bounds */
				BUS_SPACE_MAXADDR,	/* lowaddr */
				BUS_SPACE_MAXADDR,	/* highaddr */
				NULL, NULL,		/* filter, filterarg */
				MCLBYTES,	        /* maxsize */
				1,			/* nsegments */
				MCLBYTES,	        /* maxsegsize */
				0,			/* flags */
				NULL,			/* lockfunc */
				NULL,			/* lockarg */
				&adapter->rxtag);
	if (error) {
		device_printf(dev, "%s: bus_dma_tag_create failed %d\n",
		    __func__, error);
		goto fail;
	}

	error = bus_dmamap_create(adapter->rxtag, BUS_DMA_NOWAIT,
	    &adapter->rx_sparemap);
	if (error) {
		device_printf(dev, "%s: bus_dmamap_create failed: %d\n",
		    __func__, error);
		goto fail;
	}
	rx_buffer = adapter->rx_buffer_area;
	for (i = 0; i < adapter->num_rx_desc; i++, rx_buffer++) {
		error = bus_dmamap_create(adapter->rxtag, BUS_DMA_NOWAIT,
		    &rx_buffer->map);
		if (error) {
			device_printf(dev, "%s: bus_dmamap_create failed: %d\n",
			    __func__, error);
			goto fail;
		}
	}

	return (0);

fail:
	em_free_receive_structures(adapter);
	return (error);
}

/*********************************************************************
 *
 *  Allocate and initialize receive structures.
 *
 **********************************************************************/
static int
em_setup_receive_structures(struct adapter *adapter)
{
	struct em_buffer *rx_buffer;
	int i, error;

	bzero(adapter->rx_desc_base, (sizeof(struct em_rx_desc)) * adapter->num_rx_desc);

	/* Free current RX buffers. */
	rx_buffer = adapter->rx_buffer_area;
	for (i = 0; i < adapter->num_rx_desc; i++, rx_buffer++) {
		if (rx_buffer->m_head != NULL) {
			bus_dmamap_sync(adapter->rxtag, rx_buffer->map,
			    BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(adapter->rxtag, rx_buffer->map);
			m_freem(rx_buffer->m_head);
			rx_buffer->m_head = NULL;
		}
	}

	/* Allocate new ones. */
	for (i = 0; i < adapter->num_rx_desc; i++) {
		error = em_get_buf(adapter, i);
		if (error)
			return (error);
	}

	/* Setup our descriptor pointers */
	adapter->next_rx_desc_to_check = 0;
	bus_dmamap_sync(adapter->rxdma.dma_tag, adapter->rxdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return (0);
}

/*********************************************************************
 *
 *  Enable receive unit.
 *
 **********************************************************************/
static void
em_initialize_receive_unit(struct adapter *adapter)
{
	struct ifnet	*ifp = adapter->ifp;
	uint64_t	bus_addr;
	uint32_t	reg_rctl;
	uint32_t	reg_rxcsum;

	INIT_DEBUGOUT("em_initialize_receive_unit: begin");

	/*
	 * Make sure receives are disabled while setting
	 * up the descriptor ring
	 */
	E1000_WRITE_REG(&adapter->hw, RCTL, 0);

	/* Set the Receive Delay Timer Register */
	E1000_WRITE_REG(&adapter->hw, RDTR, adapter->rx_int_delay.value | E1000_RDT_FPDB);

	if(adapter->hw.mac_type >= em_82540) {
		E1000_WRITE_REG(&adapter->hw, RADV, adapter->rx_abs_int_delay.value);

		/*
		 * Set the interrupt throttling rate. Value is calculated
		 * as DEFAULT_ITR = 1/(MAX_INTS_PER_SEC * 256ns)
		 */
#define MAX_INTS_PER_SEC	8000
#define DEFAULT_ITR	     1000000000/(MAX_INTS_PER_SEC * 256)
		E1000_WRITE_REG(&adapter->hw, ITR, DEFAULT_ITR);
	}

	/* Setup the Base and Length of the Rx Descriptor Ring */
	bus_addr = adapter->rxdma.dma_paddr;
	E1000_WRITE_REG(&adapter->hw, RDLEN, adapter->num_rx_desc *
			sizeof(struct em_rx_desc));
	E1000_WRITE_REG(&adapter->hw, RDBAH, (uint32_t)(bus_addr >> 32));
	E1000_WRITE_REG(&adapter->hw, RDBAL, (uint32_t)bus_addr);

	/* Setup the Receive Control Register */
	reg_rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_LBM_NO |
		   E1000_RCTL_RDMTS_HALF |
		   (adapter->hw.mc_filter_type << E1000_RCTL_MO_SHIFT);

	if (adapter->hw.tbi_compatibility_on == TRUE)
		reg_rctl |= E1000_RCTL_SBP;


	switch (adapter->rx_buffer_len) {
	default:
	case EM_RXBUFFER_2048:
		reg_rctl |= E1000_RCTL_SZ_2048;
		break;
	case EM_RXBUFFER_4096:
		reg_rctl |= E1000_RCTL_SZ_4096 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
		break;
	case EM_RXBUFFER_8192:
		reg_rctl |= E1000_RCTL_SZ_8192 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
		break;
	case EM_RXBUFFER_16384:
		reg_rctl |= E1000_RCTL_SZ_16384 | E1000_RCTL_BSEX | E1000_RCTL_LPE;
		break;
	}

	if (ifp->if_mtu > ETHERMTU)
		reg_rctl |= E1000_RCTL_LPE;

	/* Enable 82543 Receive Checksum Offload for TCP and UDP */
	if ((adapter->hw.mac_type >= em_82543) &&
	    (ifp->if_capenable & IFCAP_RXCSUM)) {
		reg_rxcsum = E1000_READ_REG(&adapter->hw, RXCSUM);
		reg_rxcsum |= (E1000_RXCSUM_IPOFL | E1000_RXCSUM_TUOFL);
		E1000_WRITE_REG(&adapter->hw, RXCSUM, reg_rxcsum);
	}

	/* Enable Receives */
	E1000_WRITE_REG(&adapter->hw, RCTL, reg_rctl);

	/* Setup the HW Rx Head and Tail Descriptor Pointers */
	E1000_WRITE_REG(&adapter->hw, RDH, 0);
	E1000_WRITE_REG(&adapter->hw, RDT, adapter->num_rx_desc - 1);
}

/*********************************************************************
 *
 *  Free receive related data structures.
 *
 **********************************************************************/
static void
em_free_receive_structures(struct adapter *adapter)
{
	struct em_buffer *rx_buffer;
	int i;

	INIT_DEBUGOUT("free_receive_structures: begin");

	if (adapter->rx_sparemap) {
		bus_dmamap_destroy(adapter->rxtag, adapter->rx_sparemap);
		adapter->rx_sparemap = NULL;
	}
	if (adapter->rx_buffer_area != NULL) {
		rx_buffer = adapter->rx_buffer_area;
		for (i = 0; i < adapter->num_rx_desc; i++, rx_buffer++) {
			if (rx_buffer->m_head != NULL) {
				bus_dmamap_sync(adapter->rxtag, rx_buffer->map,
				    BUS_DMASYNC_POSTREAD);
				bus_dmamap_unload(adapter->rxtag,
				    rx_buffer->map);
				m_freem(rx_buffer->m_head);
				rx_buffer->m_head = NULL;
			} else if (rx_buffer->map != NULL)
				bus_dmamap_unload(adapter->rxtag,
				    rx_buffer->map);
			if (rx_buffer->map != NULL) {
				bus_dmamap_destroy(adapter->rxtag,
				    rx_buffer->map);
				rx_buffer->map = NULL;
			}
		}
	}
	if (adapter->rx_buffer_area != NULL) {
		free(adapter->rx_buffer_area, M_DEVBUF);
		adapter->rx_buffer_area = NULL;
	}
	if (adapter->rxtag != NULL) {
		bus_dma_tag_destroy(adapter->rxtag);
		adapter->rxtag = NULL;
	}
}

/*********************************************************************
 *
 *  This routine executes in interrupt context. It replenishes
 *  the mbufs in the descriptor and sends data which has been
 *  dma'ed into host memory to upper layer.
 *
 *  We loop at most count times if count is > 0, or until done if
 *  count < 0.
 *
 *********************************************************************/
static int
em_rxeof(struct adapter *adapter, int count)
{
	struct ifnet	*ifp;
	struct mbuf	*mp;
	uint8_t		accept_frame = 0;
	uint8_t		eop = 0;
	uint16_t 	len, desc_len, prev_len_adj;
	int		i;

	/* Pointer to the receive descriptor being examined. */
	struct em_rx_desc   *current_desc;
	uint8_t		status;

	ifp = adapter->ifp;
	i = adapter->next_rx_desc_to_check;
	current_desc = &adapter->rx_desc_base[i];
	bus_dmamap_sync(adapter->rxdma.dma_tag, adapter->rxdma.dma_map,
	    BUS_DMASYNC_POSTREAD);

	if (!((current_desc->status) & E1000_RXD_STAT_DD))
		return (0);

	while ((current_desc->status & E1000_RXD_STAT_DD) &&
	    (count != 0) &&
	    (ifp->if_drv_flags & IFF_DRV_RUNNING)) {
		struct mbuf *m = NULL;

		mp = adapter->rx_buffer_area[i].m_head;
		/*
		 * Can't defer bus_dmamap_sync(9) because TBI_ACCEPT
		 * needs to access the last received byte in the mbuf.
		 */
		bus_dmamap_sync(adapter->rxtag, adapter->rx_buffer_area[i].map,
		    BUS_DMASYNC_POSTREAD);

		accept_frame = 1;
		prev_len_adj = 0;
		desc_len = le16toh(current_desc->length);
		status = current_desc->status;
		if (status & E1000_RXD_STAT_EOP) {
			count--;
			eop = 1;
			if (desc_len < ETHER_CRC_LEN) {
				len = 0;
				prev_len_adj = ETHER_CRC_LEN - desc_len;
			} else
				len = desc_len - ETHER_CRC_LEN;
		} else {
			eop = 0;
			len = desc_len;
		}

		if (current_desc->errors & E1000_RXD_ERR_FRAME_ERR_MASK) {
			uint8_t		last_byte;
			uint32_t	pkt_len = desc_len;

			if (adapter->fmp != NULL)
				pkt_len += adapter->fmp->m_pkthdr.len;

			last_byte = *(mtod(mp, caddr_t) + desc_len - 1);			
			if (TBI_ACCEPT(&adapter->hw, status,
			    current_desc->errors, pkt_len, last_byte)) {
				em_tbi_adjust_stats(&adapter->hw,
				    &adapter->stats, pkt_len,
				    adapter->hw.mac_addr);
				if (len > 0)
					len--;
			} else
				accept_frame = 0;
		}

		if (accept_frame) {
			if (em_get_buf(adapter, i) != 0) {
				ifp->if_iqdrops++;
				goto discard;
			}

			/* Assign correct length to the current fragment */
			mp->m_len = len;

			if (adapter->fmp == NULL) {
				mp->m_pkthdr.len = len;
				adapter->fmp = mp; /* Store the first mbuf */
				adapter->lmp = mp;
			} else {
				/* Chain mbuf's together */
				mp->m_flags &= ~M_PKTHDR;
				/*
				 * Adjust length of previous mbuf in chain if
				 * we received less than 4 bytes in the last
				 * descriptor.
				 */
				if (prev_len_adj > 0) {
					adapter->lmp->m_len -= prev_len_adj;
					adapter->fmp->m_pkthdr.len -=
					    prev_len_adj;
				}
				adapter->lmp->m_next = mp;
				adapter->lmp = adapter->lmp->m_next;
				adapter->fmp->m_pkthdr.len += len;
			}

			if (eop) {
				adapter->fmp->m_pkthdr.rcvif = ifp;
				ifp->if_ipackets++;
				em_receive_checksum(adapter, current_desc,
				    adapter->fmp);
#ifndef __NO_STRICT_ALIGNMENT
				if (adapter->hw.max_frame_size >
				    (MCLBYTES - ETHER_ALIGN) &&
				    em_fixup_rx(adapter) != 0)
					goto skip;
#endif
				if (status & E1000_RXD_STAT_VP) {
					adapter->fmp->m_pkthdr.ether_vtag =
					    (le16toh(current_desc->special) &
					    E1000_RXD_SPC_VLAN_MASK);
					adapter->fmp->m_flags |= M_VLANTAG;
				}
#ifndef __NO_STRICT_ALIGNMENT
skip:
#endif
				m = adapter->fmp;
				adapter->fmp = NULL;
				adapter->lmp = NULL;
			}
		} else {
			ifp->if_ierrors++;
discard:
			/* Reuse loaded DMA map and just update mbuf chain */
			mp = adapter->rx_buffer_area[i].m_head;
			mp->m_len = mp->m_pkthdr.len = MCLBYTES;
			mp->m_data = mp->m_ext.ext_buf;
			mp->m_next = NULL;
			if (adapter->hw.max_frame_size <= (MCLBYTES - ETHER_ALIGN))
				m_adj(mp, ETHER_ALIGN);
			if (adapter->fmp != NULL) {
				m_freem(adapter->fmp);
				adapter->fmp = NULL;
				adapter->lmp = NULL;
			}
			m = NULL;
		}

		/* Zero out the receive descriptors status. */
		current_desc->status = 0;
		bus_dmamap_sync(adapter->rxdma.dma_tag, adapter->rxdma.dma_map,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Advance our pointers to the next descriptor. */
		if (++i == adapter->num_rx_desc)
			i = 0;
		if (m != NULL) {
			adapter->next_rx_desc_to_check = i;
#ifdef DEVICE_POLLING
			EM_UNLOCK(adapter);
			(*ifp->if_input)(ifp, m);
			EM_LOCK(adapter);
#else
			(*ifp->if_input)(ifp, m);
#endif
			i = adapter->next_rx_desc_to_check;
		}
		current_desc = &adapter->rx_desc_base[i];
	}
	adapter->next_rx_desc_to_check = i;

	/* Advance the E1000's Receive Queue #0  "Tail Pointer". */
	if (--i < 0)
		i = adapter->num_rx_desc - 1;
	E1000_WRITE_REG(&adapter->hw, RDT, i);
	if (!((current_desc->status) & E1000_RXD_STAT_DD))
		return (0);

	return (1);
}

#ifndef __NO_STRICT_ALIGNMENT
/*
 * When jumbo frames are enabled we should realign entire payload on
 * architecures with strict alignment. This is serious design mistake of 8254x
 * as it nullifies DMA operations. 8254x just allows RX buffer size to be
 * 2048/4096/8192/16384. What we really want is 2048 - ETHER_ALIGN to align its
 * payload. On architecures without strict alignment restrictions 8254x still
 * performs unaligned memory access which would reduce the performance too.
 * To avoid copying over an entire frame to align, we allocate a new mbuf and
 * copy ethernet header to the new mbuf. The new mbuf is prepended into the
 * existing mbuf chain.
 *
 * Be aware, best performance of the 8254x is achived only when jumbo frame is
 * not used at all on architectures with strict alignment.
 */
static int
em_fixup_rx(struct adapter *adapter)
{
	struct mbuf *m, *n;
	int error;

	error = 0;
	m = adapter->fmp;
	if (m->m_len <= (MCLBYTES - ETHER_HDR_LEN)) {
		bcopy(m->m_data, m->m_data + ETHER_HDR_LEN, m->m_len);
		m->m_data += ETHER_HDR_LEN;
	} else {
		MGETHDR(n, M_DONTWAIT, MT_DATA);
		if (n != NULL) {
			bcopy(m->m_data, n->m_data, ETHER_HDR_LEN);
			m->m_data += ETHER_HDR_LEN;
			m->m_len -= ETHER_HDR_LEN;
			n->m_len = ETHER_HDR_LEN;
			M_MOVE_PKTHDR(n, m);
			n->m_next = m;
			adapter->fmp = n;
		} else {
			adapter->ifp->if_iqdrops++;
			adapter->mbuf_alloc_failed++;
			m_freem(adapter->fmp);
			adapter->fmp = NULL;
			adapter->lmp = NULL;
			error = ENOBUFS;
		}
	}

	return (error);
}
#endif

/*********************************************************************
 *
 *  Verify that the hardware indicated that the checksum is valid.
 *  Inform the stack about the status of checksum so that stack
 *  doesn't spend time verifying the checksum.
 *
 *********************************************************************/
static void
em_receive_checksum(struct adapter *adapter, struct em_rx_desc *rx_desc,
		    struct mbuf *mp)
{
	/* 82543 or newer only */
	if ((adapter->hw.mac_type < em_82543) ||
	    /* Ignore Checksum bit is set */
	    (rx_desc->status & E1000_RXD_STAT_IXSM)) {
		mp->m_pkthdr.csum_flags = 0;
		return;
	}

	if (rx_desc->status & E1000_RXD_STAT_IPCS) {
		/* Did it pass? */
		if (!(rx_desc->errors & E1000_RXD_ERR_IPE)) {
			/* IP Checksum Good */
			mp->m_pkthdr.csum_flags = CSUM_IP_CHECKED;
			mp->m_pkthdr.csum_flags |= CSUM_IP_VALID;

		} else {
			mp->m_pkthdr.csum_flags = 0;
		}
	}

	if (rx_desc->status & E1000_RXD_STAT_TCPCS) {
		/* Did it pass? */
		if (!(rx_desc->errors & E1000_RXD_ERR_TCPE)) {
			mp->m_pkthdr.csum_flags |=
			(CSUM_DATA_VALID | CSUM_PSEUDO_HDR);
			mp->m_pkthdr.csum_data = htons(0xffff);
		}
	}
}


static void
em_enable_vlans(struct adapter *adapter)
{
	uint32_t ctrl;

	E1000_WRITE_REG(&adapter->hw, VET, ETHERTYPE_VLAN);

	ctrl = E1000_READ_REG(&adapter->hw, CTRL);
	ctrl |= E1000_CTRL_VME;
	E1000_WRITE_REG(&adapter->hw, CTRL, ctrl);
}

static void
em_enable_intr(struct adapter *adapter)
{
	E1000_WRITE_REG(&adapter->hw, IMS, (IMS_ENABLE_MASK));
}

static void
em_disable_intr(struct adapter *adapter)
{
	/*
	 * The first version of 82542 had an errata where when link was forced
	 * it would stay up even up even if the cable was disconnected.
	 * Sequence errors were used to detect the disconnect and then the
	 * driver would unforce the link. This code in the in the ISR. For this
	 * to work correctly the Sequence error interrupt had to be enabled
	 * all the time.
	 */

	if (adapter->hw.mac_type == em_82542_rev2_0)
	    E1000_WRITE_REG(&adapter->hw, IMC,
		(0xffffffff & ~E1000_IMC_RXSEQ));
	else
	    E1000_WRITE_REG(&adapter->hw, IMC,
		0xffffffff);
}

static int
em_is_valid_ether_addr(uint8_t *addr)
{
	char zero_addr[6] = { 0, 0, 0, 0, 0, 0 };

	if ((addr[0] & 1) || (!bcmp(addr, zero_addr, ETHER_ADDR_LEN))) {
		return (FALSE);
	}

	return (TRUE);
}

void
em_write_pci_cfg(struct em_hw *hw, uint32_t reg, uint16_t *value)
{
	pci_write_config(((struct em_osdep *)hw->back)->dev, reg, *value, 2);
}

void
em_read_pci_cfg(struct em_hw *hw, uint32_t reg, uint16_t *value)
{
	*value = pci_read_config(((struct em_osdep *)hw->back)->dev, reg, 2);
}

void
em_pci_set_mwi(struct em_hw *hw)
{
	pci_write_config(((struct em_osdep *)hw->back)->dev, PCIR_COMMAND,
	    (hw->pci_cmd_word | CMD_MEM_WRT_INVALIDATE), 2);
}

void
em_pci_clear_mwi(struct em_hw *hw)
{
	pci_write_config(((struct em_osdep *)hw->back)->dev, PCIR_COMMAND,
	    (hw->pci_cmd_word & ~CMD_MEM_WRT_INVALIDATE), 2);
}

int32_t
em_read_pcie_cap_reg(struct em_hw *hw, uint32_t reg, uint16_t *value)
{
	int32_t  rc;
	uint16_t pectl;
	device_t dev;

	dev = ((struct em_osdep *)hw->back)->dev;

	/* find the PCIe link width and set max read request to 4KB*/
	if (pci_find_extcap(dev, PCIY_EXPRESS, &reg) == 0) {
		em_read_pci_cfg(hw, reg + 0x12, value);

		em_read_pci_cfg(hw, reg + 0x8, &pectl);
		pectl = (pectl & ~0x7000) | (5 << 12);
		em_write_pci_cfg(hw, reg + 0x8, &pectl);
		rc = 0;
	} else
		rc = -1;

	return (rc);
}

/*********************************************************************
* 82544 Coexistence issue workaround.
*    There are 2 issues.
*       1. Transmit Hang issue.
*    To detect this issue, following equation can be used...
*	  SIZE[3:0] + ADDR[2:0] = SUM[3:0].
*	  If SUM[3:0] is in between 1 to 4, we will have this issue.
*
*       2. DAC issue.
*    To detect this issue, following equation can be used...
*	  SIZE[3:0] + ADDR[2:0] = SUM[3:0].
*	  If SUM[3:0] is in between 9 to c, we will have this issue.
*
*
*    WORKAROUND:
*	  Make sure we do not have ending address as 1,2,3,4(Hang) or 9,a,b,c (DAC)
*
*** *********************************************************************/
static uint32_t
em_fill_descriptors (bus_addr_t address, uint32_t length,
		PDESC_ARRAY desc_array)
{
	/* Since issue is sensitive to length and address.*/
	/* Let us first check the address...*/
	uint32_t safe_terminator;
	if (length <= 4) {
		desc_array->descriptor[0].address = address;
		desc_array->descriptor[0].length = length;
		desc_array->elements = 1;
		return (desc_array->elements);
	}
	safe_terminator = (uint32_t)((((uint32_t)address & 0x7) + (length & 0xF)) & 0xF);
	/* if it does not fall between 0x1 to 0x4 and 0x9 to 0xC then return */
	if (safe_terminator == 0   ||
	(safe_terminator > 4   &&
	safe_terminator < 9)   ||
	(safe_terminator > 0xC &&
	safe_terminator <= 0xF)) {
		desc_array->descriptor[0].address = address;
		desc_array->descriptor[0].length = length;
		desc_array->elements = 1;
		return (desc_array->elements);
	}

	desc_array->descriptor[0].address = address;
	desc_array->descriptor[0].length = length - 4;
	desc_array->descriptor[1].address = address + (length - 4);
	desc_array->descriptor[1].length = 4;
	desc_array->elements = 2;
	return (desc_array->elements);
}

/**********************************************************************
 *
 *  Update the board statistics counters.
 *
 **********************************************************************/
static void
em_update_stats_counters(struct adapter *adapter)
{
	struct ifnet   *ifp;

	if(adapter->hw.media_type == em_media_type_copper ||
	   (E1000_READ_REG(&adapter->hw, STATUS) & E1000_STATUS_LU)) {
		adapter->stats.symerrs += E1000_READ_REG(&adapter->hw, SYMERRS);
		adapter->stats.sec += E1000_READ_REG(&adapter->hw, SEC);
	}
	adapter->stats.crcerrs += E1000_READ_REG(&adapter->hw, CRCERRS);
	adapter->stats.mpc += E1000_READ_REG(&adapter->hw, MPC);
	adapter->stats.scc += E1000_READ_REG(&adapter->hw, SCC);
	adapter->stats.ecol += E1000_READ_REG(&adapter->hw, ECOL);

	adapter->stats.mcc += E1000_READ_REG(&adapter->hw, MCC);
	adapter->stats.latecol += E1000_READ_REG(&adapter->hw, LATECOL);
	adapter->stats.colc += E1000_READ_REG(&adapter->hw, COLC);
	adapter->stats.dc += E1000_READ_REG(&adapter->hw, DC);
	adapter->stats.rlec += E1000_READ_REG(&adapter->hw, RLEC);
	adapter->stats.xonrxc += E1000_READ_REG(&adapter->hw, XONRXC);
	adapter->stats.xontxc += E1000_READ_REG(&adapter->hw, XONTXC);
	adapter->stats.xoffrxc += E1000_READ_REG(&adapter->hw, XOFFRXC);
	adapter->stats.xofftxc += E1000_READ_REG(&adapter->hw, XOFFTXC);
	adapter->stats.fcruc += E1000_READ_REG(&adapter->hw, FCRUC);
	adapter->stats.prc64 += E1000_READ_REG(&adapter->hw, PRC64);
	adapter->stats.prc127 += E1000_READ_REG(&adapter->hw, PRC127);
	adapter->stats.prc255 += E1000_READ_REG(&adapter->hw, PRC255);
	adapter->stats.prc511 += E1000_READ_REG(&adapter->hw, PRC511);
	adapter->stats.prc1023 += E1000_READ_REG(&adapter->hw, PRC1023);
	adapter->stats.prc1522 += E1000_READ_REG(&adapter->hw, PRC1522);
	adapter->stats.gprc += E1000_READ_REG(&adapter->hw, GPRC);
	adapter->stats.bprc += E1000_READ_REG(&adapter->hw, BPRC);
	adapter->stats.mprc += E1000_READ_REG(&adapter->hw, MPRC);
	adapter->stats.gptc += E1000_READ_REG(&adapter->hw, GPTC);

	/* For the 64-bit byte counters the low dword must be read first. */
	/* Both registers clear on the read of the high dword */

	adapter->stats.gorcl += E1000_READ_REG(&adapter->hw, GORCL);
	adapter->stats.gorch += E1000_READ_REG(&adapter->hw, GORCH);
	adapter->stats.gotcl += E1000_READ_REG(&adapter->hw, GOTCL);
	adapter->stats.gotch += E1000_READ_REG(&adapter->hw, GOTCH);

	adapter->stats.rnbc += E1000_READ_REG(&adapter->hw, RNBC);
	adapter->stats.ruc += E1000_READ_REG(&adapter->hw, RUC);
	adapter->stats.rfc += E1000_READ_REG(&adapter->hw, RFC);
	adapter->stats.roc += E1000_READ_REG(&adapter->hw, ROC);
	adapter->stats.rjc += E1000_READ_REG(&adapter->hw, RJC);

	adapter->stats.torl += E1000_READ_REG(&adapter->hw, TORL);
	adapter->stats.torh += E1000_READ_REG(&adapter->hw, TORH);
	adapter->stats.totl += E1000_READ_REG(&adapter->hw, TOTL);
	adapter->stats.toth += E1000_READ_REG(&adapter->hw, TOTH);

	adapter->stats.tpr += E1000_READ_REG(&adapter->hw, TPR);
	adapter->stats.tpt += E1000_READ_REG(&adapter->hw, TPT);
	adapter->stats.ptc64 += E1000_READ_REG(&adapter->hw, PTC64);
	adapter->stats.ptc127 += E1000_READ_REG(&adapter->hw, PTC127);
	adapter->stats.ptc255 += E1000_READ_REG(&adapter->hw, PTC255);
	adapter->stats.ptc511 += E1000_READ_REG(&adapter->hw, PTC511);
	adapter->stats.ptc1023 += E1000_READ_REG(&adapter->hw, PTC1023);
	adapter->stats.ptc1522 += E1000_READ_REG(&adapter->hw, PTC1522);
	adapter->stats.mptc += E1000_READ_REG(&adapter->hw, MPTC);
	adapter->stats.bptc += E1000_READ_REG(&adapter->hw, BPTC);

	if (adapter->hw.mac_type >= em_82543) {
		adapter->stats.algnerrc += E1000_READ_REG(&adapter->hw, ALGNERRC);
		adapter->stats.rxerrc += E1000_READ_REG(&adapter->hw, RXERRC);
		adapter->stats.tncrs += E1000_READ_REG(&adapter->hw, TNCRS);
		adapter->stats.cexterr += E1000_READ_REG(&adapter->hw, CEXTERR);
		adapter->stats.tsctc += E1000_READ_REG(&adapter->hw, TSCTC);
		adapter->stats.tsctfc += E1000_READ_REG(&adapter->hw, TSCTFC);
	}
	ifp = adapter->ifp;

	ifp->if_collisions = adapter->stats.colc;

	/* Rx Errors */
	ifp->if_ierrors = adapter->stats.rxerrc + adapter->stats.crcerrs +
	    adapter->stats.algnerrc + adapter->stats.ruc + adapter->stats.roc +
	    adapter->stats.mpc + adapter->stats.cexterr;

	/* Tx Errors */
	ifp->if_oerrors = adapter->stats.ecol + adapter->stats.latecol +
	    adapter->watchdog_events;
}


/**********************************************************************
 *
 *  This routine is called only when em_display_debug_stats is enabled.
 *  This routine provides a way to take a look at important statistics
 *  maintained by the driver and hardware.
 *
 **********************************************************************/
static void
em_print_debug_info(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	uint8_t *hw_addr = adapter->hw.hw_addr;

	device_printf(dev, "Adapter hardware address = %p \n", hw_addr);
	device_printf(dev, "CTRL = 0x%x RCTL = 0x%x \n",
	    E1000_READ_REG(&adapter->hw, CTRL),
	    E1000_READ_REG(&adapter->hw, RCTL));
	device_printf(dev, "Packet buffer = Tx=%dk Rx=%dk \n",
	    ((E1000_READ_REG(&adapter->hw, PBA) & 0xffff0000) >> 16),\
	    (E1000_READ_REG(&adapter->hw, PBA) & 0xffff) );
	device_printf(dev, "Flow control watermarks high = %d low = %d\n",
	    adapter->hw.fc_high_water,
	    adapter->hw.fc_low_water);
	device_printf(dev, "tx_int_delay = %d, tx_abs_int_delay = %d\n",
	    E1000_READ_REG(&adapter->hw, TIDV),
	    E1000_READ_REG(&adapter->hw, TADV));
	device_printf(dev, "rx_int_delay = %d, rx_abs_int_delay = %d\n",
	    E1000_READ_REG(&adapter->hw, RDTR),
	    E1000_READ_REG(&adapter->hw, RADV));
	device_printf(dev, "fifo workaround = %lld, fifo_reset_count = %lld\n",
	    (long long)adapter->tx_fifo_wrk_cnt,
	    (long long)adapter->tx_fifo_reset_cnt);
	device_printf(dev, "hw tdh = %d, hw tdt = %d\n",
	    E1000_READ_REG(&adapter->hw, TDH),
	    E1000_READ_REG(&adapter->hw, TDT));
	device_printf(dev, "Num Tx descriptors avail = %d\n",
	    adapter->num_tx_desc_avail);
	device_printf(dev, "Tx Descriptors not avail1 = %ld\n",
	    adapter->no_tx_desc_avail1);
	device_printf(dev, "Tx Descriptors not avail2 = %ld\n",
	    adapter->no_tx_desc_avail2);
	device_printf(dev, "Std mbuf failed = %ld\n",
	    adapter->mbuf_alloc_failed);
	device_printf(dev, "Std mbuf cluster failed = %ld\n",
	    adapter->mbuf_cluster_failed);
}

static void
em_print_hw_stats(struct adapter *adapter)
{
	device_t dev = adapter->dev;

	device_printf(dev, "Excessive collisions = %lld\n",
	    (long long)adapter->stats.ecol);
	device_printf(dev, "Symbol errors = %lld\n",
	    (long long)adapter->stats.symerrs);
	device_printf(dev, "Sequence errors = %lld\n",
	    (long long)adapter->stats.sec);
	device_printf(dev, "Defer count = %lld\n", (long long)adapter->stats.dc);

	device_printf(dev, "Missed Packets = %lld\n", (long long)adapter->stats.mpc);
	device_printf(dev, "Receive No Buffers = %lld\n",
	    (long long)adapter->stats.rnbc);
	/* RLEC is inaccurate on some hardware, calculate our own. */
	device_printf(dev, "Receive Length Errors = %lld\n",
	    ((long long)adapter->stats.roc + (long long)adapter->stats.ruc));
	device_printf(dev, "Receive errors = %lld\n",
	    (long long)adapter->stats.rxerrc);
	device_printf(dev, "Crc errors = %lld\n", (long long)adapter->stats.crcerrs);
	device_printf(dev, "Alignment errors = %lld\n",
	    (long long)adapter->stats.algnerrc);
	device_printf(dev, "Carrier extension errors = %lld\n",
	    (long long)adapter->stats.cexterr);
	device_printf(dev, "RX overruns = %ld\n", adapter->rx_overruns);
	device_printf(dev, "watchdog timeouts = %ld\n", adapter->watchdog_events);

	device_printf(dev, "XON Rcvd = %lld\n", (long long)adapter->stats.xonrxc);
	device_printf(dev, "XON Xmtd = %lld\n", (long long)adapter->stats.xontxc);
	device_printf(dev, "XOFF Rcvd = %lld\n", (long long)adapter->stats.xoffrxc);
	device_printf(dev, "XOFF Xmtd = %lld\n", (long long)adapter->stats.xofftxc);

	device_printf(dev, "Good Packets Rcvd = %lld\n",
	    (long long)adapter->stats.gprc);
	device_printf(dev, "Good Packets Xmtd = %lld\n",
	    (long long)adapter->stats.gptc);
	device_printf(dev, "TSO Contexts Xmtd = %lld\n",
	    (long long)adapter->stats.tsctc);
	device_printf(dev, "TSO Contexts Failed = %lld\n",
	    (long long)adapter->stats.tsctfc);
}

static int
em_sysctl_debug_info(SYSCTL_HANDLER_ARGS)
{
	struct adapter *adapter;
	int error;
	int result;

	result = -1;
	error = sysctl_handle_int(oidp, &result, 0, req);

	if (error || !req->newptr)
		return (error);

	if (result == 1) {
		adapter = (struct adapter *)arg1;
		em_print_debug_info(adapter);
	}

	return (error);
}


static int
em_sysctl_stats(SYSCTL_HANDLER_ARGS)
{
	struct adapter *adapter;
	int error;
	int result;

	result = -1;
	error = sysctl_handle_int(oidp, &result, 0, req);

	if (error || !req->newptr)
		return (error);

	if (result == 1) {
		adapter = (struct adapter *)arg1;
		em_print_hw_stats(adapter);
	}

	return (error);
}

static int
em_sysctl_int_delay(SYSCTL_HANDLER_ARGS)
{
	struct em_int_delay_info *info;
	struct adapter *adapter;
	uint32_t regval;
	int error;
	int usecs;
	int ticks;

	info = (struct em_int_delay_info *)arg1;
	usecs = info->value;
	error = sysctl_handle_int(oidp, &usecs, 0, req);
	if (error != 0 || req->newptr == NULL)
		return (error);
	if (usecs < 0 || usecs > E1000_TICKS_TO_USECS(65535))
		return (EINVAL);
	info->value = usecs;
	ticks = E1000_USECS_TO_TICKS(usecs);

	adapter = info->adapter;
	
	EM_LOCK(adapter);
	regval = E1000_READ_OFFSET(&adapter->hw, info->offset);
	regval = (regval & ~0xffff) | (ticks & 0xffff);
	/* Handle a few special cases. */
	switch (info->offset) {
	case E1000_RDTR:
	case E1000_82542_RDTR:
		regval |= E1000_RDT_FPDB;
		break;
	case E1000_TIDV:
	case E1000_82542_TIDV:
		if (ticks == 0) {
			adapter->txd_cmd &= ~E1000_TXD_CMD_IDE;
			/* Don't write 0 into the TIDV register. */
			regval++;
		} else
			adapter->txd_cmd |= E1000_TXD_CMD_IDE;
		break;
	}
	E1000_WRITE_OFFSET(&adapter->hw, info->offset, regval);
	EM_UNLOCK(adapter);
	return (0);
}

static void
em_add_int_delay_sysctl(struct adapter *adapter, const char *name,
	const char *description, struct em_int_delay_info *info,
	int offset, int value)
{
	info->adapter = adapter;
	info->offset = offset;
	info->value = value;
	SYSCTL_ADD_PROC(device_get_sysctl_ctx(adapter->dev),
	    SYSCTL_CHILDREN(device_get_sysctl_tree(adapter->dev)),
	    OID_AUTO, name, CTLTYPE_INT|CTLFLAG_RW,
	    info, 0, em_sysctl_int_delay, "I", description);
}

#ifndef DEVICE_POLLING
static void
em_add_int_process_limit(struct adapter *adapter, const char *name,
	const char *description, int *limit, int value)
{
	*limit = value;
	SYSCTL_ADD_INT(device_get_sysctl_ctx(adapter->dev),
	    SYSCTL_CHILDREN(device_get_sysctl_tree(adapter->dev)),
	    OID_AUTO, name, CTLTYPE_INT|CTLFLAG_RW, limit, value, description);
}
#endif
