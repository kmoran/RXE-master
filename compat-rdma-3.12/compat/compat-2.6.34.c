/*
 * Copyright 2012    Luis R. Rodriguez <mcgrof@do-not-panic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Compatibility file for Linux wireless for kernels 2.6.34.
 */

#include <linux/mmc/sdio_func.h>
#include <linux/pci.h>
#include <linux/export.h>

#include "compat-2.6.34.h"

static mmc_pm_flag_t backport_mmc_pm_flags;

void backport_init_mmc_pm_flags(void)
{
	backport_mmc_pm_flags = 0;
}

mmc_pm_flag_t sdio_get_host_pm_caps(struct sdio_func *func)
{
	return backport_mmc_pm_flags;
}

int sdio_set_host_pm_flags(struct sdio_func *func, mmc_pm_flag_t flags)
{
	return -EINVAL;
}

int pci_vpd_find_tag(const u8 *buf, unsigned int off, unsigned int len, u8 rdt)
{
	int i;

	for (i = off; i < len; ) {
		u8 val = buf[i];

		if (val & PCI_VPD_LRDT) {
			/* Don't return success of the tag isn't complete */
			if (i + PCI_VPD_LRDT_TAG_SIZE > len)
				break;

			if (val == rdt)
				return i;

			i += PCI_VPD_LRDT_TAG_SIZE +
			     pci_vpd_lrdt_size(&buf[i]);
		} else {
			u8 tag = val & ~PCI_VPD_SRDT_LEN_MASK;

			if (tag == rdt)
				return i;

			if (tag == PCI_VPD_SRDT_END)
				break;

			i += PCI_VPD_SRDT_TAG_SIZE +
			     pci_vpd_srdt_size(&buf[i]);
		}
	}

	return -ENOENT;
}
EXPORT_SYMBOL_GPL(pci_vpd_find_tag);

int pci_vpd_find_info_keyword(const u8 *buf, unsigned int off,
			      unsigned int len, const char *kw)
{
	int i;

	for (i = off; i + PCI_VPD_INFO_FLD_HDR_SIZE <= off + len;) {
		if (buf[i + 0] == kw[0] &&
		    buf[i + 1] == kw[1])
			return i;

		i += PCI_VPD_INFO_FLD_HDR_SIZE +
		     pci_vpd_info_field_size(&buf[i]);
	}

	return -ENOENT;
}
EXPORT_SYMBOL_GPL(pci_vpd_find_info_keyword);
