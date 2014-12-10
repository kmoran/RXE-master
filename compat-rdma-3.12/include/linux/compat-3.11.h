#ifndef LINUX_3_11_COMPAT_H
#define LINUX_3_11_COMPAT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0))

#ifndef AF_IB
#define AF_IB		27      /* Native InfiniBand address    */
#define PF_IB		AF_IB
#endif /* AF_IB */

#define netdev_notifier_info_to_dev LINUX_BACKPORT(netdev_notifier_info_to_dev)
static inline struct net_device *
netdev_notifier_info_to_dev(void *ptr)
{
	return (struct net_device *)ptr;
}

#if !defined(CONFIG_COMPAT_IFLA_VF_LINK_STATE_MAX)
enum {
	IFLA_VF_LINK_STATE_AUTO,	/* link state of the uplink */
	IFLA_VF_LINK_STATE_ENABLE,	/* link always up */
	IFLA_VF_LINK_STATE_DISABLE,	/* link always down */
	__IFLA_VF_LINK_STATE_MAX,
};
#endif

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0)) */
#endif /* LINUX_3_11_COMPAT_H */
