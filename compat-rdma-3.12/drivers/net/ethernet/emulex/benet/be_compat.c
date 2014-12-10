#include "be.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32) && defined(CONFIG_PCI_IOV)
#define sriov_kernel                            true
#else
#define sriov_kernel                            false
#endif

#ifndef PCI_DEV_FLAGS_ASSIGNED
#define PCI_DEV_FLAGS_ASSIGNED 0x04
#endif

#ifdef CONFIG_PCI_IOV
int be_find_vfs(struct pci_dev *pdev, int vf_state)
{
	struct pci_dev *dev = pdev;
	int vfs = 0, assigned_vfs = 0, pos;

	if (!sriov_kernel)
		return 0;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_SRIOV);
	if (!pos)
		return 0;

	dev = pci_get_device(pdev->vendor, PCI_ANY_ID, NULL);
	while (dev) {
		if (dev->is_virtfn && pci_physfn(dev) == pdev) {
			vfs++;
			if (dev->dev_flags & PCI_DEV_FLAGS_ASSIGNED)
				assigned_vfs++;
		}
		dev = pci_get_device(pdev->vendor, PCI_ANY_ID, dev);
	}
	return (vf_state == ASSIGNED) ? assigned_vfs : vfs;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 10, 0)
/**
 * pci_vfs_assigned - returns number of VFs are assigned to a guest
 * @dev: the PCI device
 *
 * Returns number of VFs belonging to this device that are assigned to a guest.
 * If device is not a physical function returns -ENODEV.
 */
int pci_vfs_assigned(struct pci_dev *pdev)
{
	return be_find_vfs(pdev, ASSIGNED);
}
/**
 * pci_num_vf - return number of VFs associated with a PF device_release_driver
 * @dev: the PCI device
 *
 * Returns number of VFs, or 0 if SR-IOV is not enabled.
 */
int pci_num_vf(struct pci_dev *pdev)
{
	return be_find_vfs(pdev, ENABLED);
}
int pci_sriov_get_totalvfs(struct pci_dev *pdev)
{
	u16 num = 0;
	int pos;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_SRIOV);
	if (pos)
		pci_read_config_word(pdev, pos + PCI_SRIOV_TOTAL_VF, &num);
	return num;
}

#endif
#endif /* CONFIG_PCI_IOV */
