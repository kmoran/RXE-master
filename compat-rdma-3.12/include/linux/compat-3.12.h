#ifndef LINUX_3_12_COMPAT_H
#define LINUX_3_12_COMPAT_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0))

#ifndef PTR_ERR_OR_ZERO
#define PTR_ERR_OR_ZERO(p) PTR_RET(p)
#endif

#define MODULE_ALIAS_FS(NAME) MODULE_ALIAS("fs-" NAME)

#define file_inode LINUX_BACKPORT(file_inode)
static inline struct inode *file_inode(struct file *f)
{
	return f->f_dentry->d_inode;
}

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)) */

#endif /* LINUX_3_12_COMPAT_H */
