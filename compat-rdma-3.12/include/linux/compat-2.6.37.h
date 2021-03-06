#ifndef LINUX_26_37_COMPAT_H
#define LINUX_26_37_COMPAT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37))

#include <linux/skbuff.h>
#include <linux/leds.h>
#include <linux/in.h>
#include <linux/rcupdate.h>
#include <linux/netdevice.h>
#include <linux/errno.h>

static inline int proto_ports_offset(int proto)
{
	switch (proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_DCCP:
	case IPPROTO_ESP:	/* SPI */
	case IPPROTO_SCTP:
	case IPPROTO_UDPLITE:
		return 0;
	case IPPROTO_AH:	/* SPI */
		return 4;
	default:
		return -EINVAL;
	}
}

/* supports eipoib flags, priv_flags is short till that version */
#define CONFIG_COMPAT_IFF_EIPOIB_PIF 0x8000 /*== IFF_OVS_DATAPATH*/
#define CONFIG_COMPAT_IFF_EIPOIB_VIF 0x4000 /*IFF_MACVLAN_PORT*/

/* Definitions for tx_flags in struct skb_shared_info */
enum {
	/* generate hardware time stamp */
	SKBTX_HW_TSTAMP = 1 << 0,

	/* generate software time stamp */
	SKBTX_SW_TSTAMP = 1 << 1,

	/* device driver is going to provide hardware time stamp */
	SKBTX_IN_PROGRESS = 1 << 2,

	/* ensure the originating sk reference is available on driver level */
	SKBTX_DRV_NEEDS_SK_REF = 1 << 3,
};


#define SDIO_CLASS_BT_AMP	0x09	/* Type-A Bluetooth AMP interface */

#define VLAN_N_VID              4096

/*
 *     netif_set_real_num_rx_queues - set actual number of RX queues used
 *     @dev: Network device
 *     @rxq: Actual number of RX queues
 *
 *     This function actully sets the real_num_rx_queues field in struct
 *     net_device. Since real_num_rx_queues field is not present in net_device
 *     structure in 2.6.37 kernel making this function to set that field is not
 *     possible. Hence adding this function to avoid changes in driver source
 *     code and making this function to always return success.
 */
/* mask netif_set_real_num_rx_queues as RHEL6.4 backports this */
#define netif_set_real_num_rx_queues(a, b) compat_netif_set_real_num_rx_queues(a, b)
static inline int netif_set_real_num_rx_queues(struct net_device *dev,
        unsigned int rxq)
{
    return 0;
}

#define net_ns_type_operations LINUX_BACKPORT(net_ns_type_operations)
extern struct kobj_ns_type_operations net_ns_type_operations;

/* mask skb_checksum_none_assert as RHEL6 backports this */
#define skb_checksum_none_assert(a) compat_skb_checksum_none_assert(a)

/**
 * skb_checksum_none_assert - make sure skb ip_summed is CHECKSUM_NONE
 * @skb: skb to check
 *
 * fresh skbs have their ip_summed set to CHECKSUM_NONE.
 * Instead of forcing ip_summed to CHECKSUM_NONE, we can
 * use this helper, to document places where we make this assertion.
 */
static inline void skb_checksum_none_assert(struct sk_buff *skb)
{
#ifdef DEBUG
	BUG_ON(skb->ip_summed != CHECKSUM_NONE);
#endif
}

#include <net/genetlink.h>

struct compat_genl_info {
	struct genl_info *info;

	u32 snd_seq;
	u32 snd_pid;
	struct genlmsghdr *genlhdr;
	struct nlattr **attrs;
	void *user_ptr[2];
};
#define genl_info compat_genl_info

struct compat_genl_ops {
	struct genl_ops ops;

	u8 cmd;
	u8 internal_flags;
	unsigned int flags;
	const struct nla_policy *policy;

	int (*doit)(struct sk_buff *skb, struct genl_info *info);
	int (*dumpit)(struct sk_buff *skb, struct netlink_callback *cb);
	int (*done)(struct netlink_callback *cb);
};
#define genl_ops compat_genl_ops

struct compat_genl_family {
	struct genl_family family;

	struct list_head list;

	unsigned int id, hdrsize, version, maxattr;
	const char *name;
	bool netnsok;

	struct nlattr **attrbuf;

	int (*pre_doit)(struct genl_ops *ops, struct sk_buff *skb,
			struct genl_info *info);

	void (*post_doit)(struct genl_ops *ops, struct sk_buff *skb,
			  struct genl_info *info);
};

#define genl_family compat_genl_family

#define genl_register_family_with_ops compat_genl_register_family_with_ops

int genl_register_family_with_ops(struct genl_family *family,
				  struct genl_ops *ops, size_t n_ops);

#define genl_unregister_family compat_genl_unregister_family

int genl_unregister_family(struct genl_family *family);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32))
#define genl_info_net(_info) genl_info_net((_info)->info)
#endif

#define genlmsg_reply(_msg, _info) genlmsg_reply(_msg, (_info)->info)
#define genlmsg_put(_skb, _pid, _seq, _fam, _flags, _cmd) genlmsg_put(_skb, _pid, _seq, &(_fam)->family, _flags, _cmd)
#define genl_register_mc_group(_fam, _grp) genl_register_mc_group(&(_fam)->family, _grp)
#define genl_unregister_mc_group(_fam, _grp) genl_unregister_mc_group(&(_fam)->family, _grp)


#define led_blink_set LINUX_BACKPORT(led_blink_set)
extern void led_blink_set(struct led_classdev *led_cdev,
			  unsigned long *delay_on,
			  unsigned long *delay_off);

#define led_classdev_unregister compat_led_classdev_unregister
extern void compat_led_classdev_unregister(struct led_classdev *led_cdev);

#define led_brightness_set compat_led_brightness_set
extern void compat_led_brightness_set(struct led_classdev *led_cdev,
				      enum led_brightness brightness);

#define alloc_ordered_workqueue(name, flags) create_singlethread_workqueue(name)

#define netdev_refcnt_read(a) atomic_read(&a->refcnt)

#define vzalloc LINUX_BACKPORT(vzalloc)
#define vzalloc_node LINUX_BACKPORT(vzalloc_node)

extern void *vzalloc(unsigned long size);
extern void *vzalloc_node(unsigned long size, int node);

#ifndef rtnl_dereference
#define rtnl_dereference(p)                                     \
        rcu_dereference_protected(p, lockdep_rtnl_is_held())
#endif

#ifndef rcu_dereference_protected
#define rcu_dereference_protected(p, c) \
		rcu_dereference((p))
#endif

#ifndef rcu_dereference_bh
#define rcu_dereference_bh(p) \
		rcu_dereference((p))
#endif

/**
 * RCU_INIT_POINTER() - initialize an RCU protected pointer
 *
 * Initialize an RCU-protected pointer in such a way to avoid RCU-lockdep
 * splats.
 */
#define RCU_INIT_POINTER(p, v) \
		p = (typeof(*v) __force __rcu *)(v)

#define skb_has_frag_list LINUX_BACKPORT(skb_has_frag_list)
static inline bool skb_has_frag_list(const struct sk_buff *skb)
{
	return skb_shinfo(skb)->frag_list != NULL;
}

/* boolean flags controlling per-interface behavior characteristics.
 * When reading, the flag indicates whether or not a certain behavior
 * is enabled/present.  When writing, the flag indicates whether
 * or not the driver should turn on (set) or off (clear) a behavior.
 *
 * Some behaviors may read-only (unconditionally absent or present).
 * If such is the case, return EINVAL in the set-flags operation if the
 * flag differs from the read-only value.
 *
 * Adding missing enums for ethtool_flags in 2.6.32 kernel.
 */
enum additional_ethtool_flags {
    ETH_FLAG_TXVLAN         = (1 << 7),     /* TX VLAN offload enabled */
    ETH_FLAG_RXVLAN         = (1 << 8),     /* RX VLAN offload enabled */
};

extern void             unregister_netdevice_queue(struct net_device *dev,
						   struct list_head *head);

#ifndef max3
#define max3(x, y, z) ({			\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	typeof(z) _max3 = (z);			\
	(void) (&_max1 == &_max2);		\
	(void) (&_max1 == &_max3);		\
	_max1 > _max2 ? (_max1 > _max3 ? _max1 : _max3) : \
		(_max2 > _max3 ? _max2 : _max3); })
#endif

#ifndef CONFIG_COMPAT_XPRTRDMA_NEEDED
struct rpc_xprt *	xprt_alloc(int size, int max_req);
void			xprt_free(struct rpc_xprt *);
#endif /* CONFIG_COMPAT_XPRTRDMA_NEEDED */

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)) */

#endif /* LINUX_26_37_COMPAT_H */
