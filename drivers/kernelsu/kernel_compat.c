#include "linux/version.h"
#include "linux/fs.h"
#include "linux/nsproxy.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
#include "linux/sched/task.h"
#include "linux/uaccess.h"
#else
#include "linux/sched.h"
#endif
#include "klog.h" // IWYU pragma: keep

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
#include "linux/key.h"
#include "linux/errno.h"
struct key *init_session_keyring = NULL;
#endif
ssize_t ksu_kernel_read_compat(struct file *p, void *buf, size_t count, loff_t *pos){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
    return kernel_read(p, buf, count, pos);
#else
    loff_t offset = pos ? *pos : 0;
    ssize_t result = kernel_read(p, offset, (char *)buf, count);
    if (pos && result > 0)
    {
        *pos = offset + result;
    }
    return result;
#endif
}

ssize_t ksu_kernel_write_compat(struct file *p, const void *buf, size_t count, loff_t *pos){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
    return kernel_write(p, buf, count, pos);
#else
    loff_t offset = pos ? *pos : 0;
    ssize_t result = kernel_write(p, buf, count, offset);
    if (pos && result > 0)
    {
        *pos = offset + result;
    }
    return result;
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
long ksu_strncpy_from_user_nofault(char *dst, const void __user *unsafe_addr,
				   long count)
{
	return strncpy_from_user_nofault(dst, unsafe_addr, count);
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
long ksu_strncpy_from_user_nofault(char *dst, const void __user *unsafe_addr,
				   long count)
{
	return strncpy_from_unsafe_user(dst, unsafe_addr, count);
}
#else
// Copied from: https://elixir.bootlin.com/linux/v4.9.337/source/mm/maccess.c#L201
long ksu_strncpy_from_user_nofault(char *dst, const void __user *unsafe_addr,
				   long count)
{
	mm_segment_t old_fs = get_fs();
	long ret;

	if (unlikely(count <= 0))
		return 0;

	set_fs(USER_DS);
	pagefault_disable();
	ret = strncpy_from_user(dst, unsafe_addr, count);
	pagefault_enable();
	set_fs(old_fs);

	if (ret >= count) {
		ret = count;
		dst[ret - 1] = '\0';
	} else if (ret > 0) {
		ret++;
	}

	return ret;
}
#endif
