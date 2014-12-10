#ifndef LINUX_3_9_COMPAT_H
#define LINUX_3_9_COMPAT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0))

extern u16 __netdev_pick_tx(struct net_device *dev, struct sk_buff *skb);

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)) */

#endif /* LINUX_3_9_COMPAT_H */
