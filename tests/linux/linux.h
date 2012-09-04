#pragma once

#define __user

#define NULL		((void *)0)

#define ENOMEM		12
#define EFAULT		14
#define EBUSY		16
#define EINVAL		22
#define EMSGSIZE	90
#define ENOPROTOOPT	92

#define UINT_MAX	(~0U)

#define GFP_KERNEL	(0x10u | 0x40u | 0x80u)

#define BUG_ON(x)	if (x) __builtin_trap();

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

typedef unsigned char		u8;
typedef u8			uint8_t;

typedef unsigned short		u16;
typedef u16			uint16_t;

typedef unsigned int		u32;
typedef u32			uint32_t;

typedef unsigned long long	u64;
typedef u64			uint64_t;

typedef unsigned long		size_t;
typedef long			off_t;
typedef long long		loff_t;

#define INT_LIMIT(x)		(~((x)1 << (sizeof(x)*8 - 1)))
#define OFFSET_MAX		INT_LIMIT(loff_t)
#define OFFT_OFFSET_MAX		INT_LIMIT(off_t)

#define PATH_MAX		4096

#define VERIFY_READ		0
#define VERIFY_WRITE		1

#define ALIGN(x, a)		__ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1UL << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))

#define PAGE_ALIGN(addr)	ALIGN(addr, PAGE_SIZE)

typedef unsigned gfp_t;

void *kmalloc(size_t size, gfp_t flags);
void *kzalloc(size_t size, gfp_t flags);
void kfree(void *);

void *vmalloc(unsigned long size);
void vfree(const void *addr);

void *memcpy(void *dest, const void *src, size_t count);

unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);

#define access_ok(type, addr, size)	__access_ok((unsigned long)addr,size)

int __access_ok(unsigned long addr, unsigned long size);

struct sock;
struct socket;
struct sk_buff;

struct sk_buff *sock_alloc_send_skb(struct sock *sk,
				    unsigned long size,
				    int noblock,
				    int *errcode);