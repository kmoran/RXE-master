#ifndef BE_COMPAT_H
#define BE_COMPAT_H

#include <linux/netdevice.h>

#ifndef VLAN_N_VID
#define VLAN_N_VID              4096
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
#define USE_NEW_VLAN_MODEL
#endif

#if defined(USE_NEW_VLAN_MODEL) || LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
/* vlan_gro_frags() can be safely called when vlan_group is NULL
 *  * for kernels >= 3.0 or when kernels uses USE_NEW_VLAN_MODEL.
 */
#define NULL_VLAN_GRP_SAFE
#endif

static inline struct sk_buff *__vlan_put_tag_fixed(struct sk_buff *skb,
						__be16 vlan_proto,
						ushort vlan_tag)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	struct sk_buff *new_skb = __vlan_put_tag(skb, vlan_proto, vlan_tag);
#else
	struct sk_buff *new_skb = __vlan_put_tag(skb, vlan_tag);
#endif
	return new_skb;
}

#ifdef USE_NEW_VLAN_MODEL
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0)
struct vlan_group {
	char dummy;
};
#endif

static inline int vlan_hwaccel_receive_skb_compat(struct sk_buff *skb,
						  struct vlan_group *grp,
						  u16 vlan_tci)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vlan_tci);
#else
	__vlan_hwaccel_put_tag(skb, vlan_tci);
#endif
	return netif_receive_skb(skb);
}

static inline gro_result_t vlan_gro_frags_compat(struct napi_struct *napi,
						 struct vlan_group *grp,
						 unsigned int vlan_tci)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	__vlan_hwaccel_put_tag(napi->skb, htons(ETH_P_8021Q), vlan_tci);
#else
	__vlan_hwaccel_put_tag(napi->skb, vlan_tci);
#endif
	return napi_gro_frags(napi);
}
#define vlan_hwaccel_receive_skb                vlan_hwaccel_receive_skb_compat
#define vlan_gro_frags                          vlan_gro_frags_compat
#endif

#ifdef CONFIG_PCI_IOV
int be_find_vfs(struct pci_dev *pdev, int vf_state);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 10, 0)
int pci_vfs_assigned(struct pci_dev *pdev);

int pci_num_vf(struct pci_dev *pdev);
int pci_sriov_get_totalvfs(struct pci_dev *pdev);
#endif
#else
#define pci_vfs_assigned(x)                     0
#define pci_num_vf(x)                           0
#endif /* CONFIG_PCI_IOV */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
#define NETIF_F_HW_VLAN_CTAG_TX         NETIF_F_HW_VLAN_TX
#define NETIF_F_HW_VLAN_CTAG_RX         NETIF_F_HW_VLAN_RX
#define NETIF_F_HW_VLAN_CTAG_FILTER     NETIF_F_HW_VLAN_FILTER
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
#define hw_features                     features
#endif

#ifndef netdev_for_each_mc_addr
#define netdev_for_each_mc_addr(h, n)   for (h = (n)->mc_list; h; h = h->next)
#endif

/* When new mc-list macros were used in 2.6.35, dev_mc_list was dropped */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
#define DMI_ADDR                        addr
#else
#define DMI_ADDR                        dmi_addr
#endif /* dev_mc_list */

#ifndef DUPLEX_UNKNOWN
#define DUPLEX_UNKNOWN 0xFF
#endif

#endif                          /* BE_COMPAT_H */
