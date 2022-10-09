# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux Random Number Generator.
#

ccflags-y += -DCONFIG_CRYPTO_LIB_SHA256=true

obj-y				+= lrng_es_mgr.o lrng_aux.o \
				   lrng_drng.o lrng_chacha20.o \
				   lrng_interfaces.o lrng_es_aux.o


obj-$(CONFIG_LRNG_IRQ)		+= lrng_es_irq.o
obj-$(CONFIG_SYSCTL)		+= lrng_proc.o
obj-$(CONFIG_NUMA)		+= lrng_numa.o
obj-$(CONFIG_LRNG_CPU)		+= lrng_es_archrandom.o
obj-$(CONFIG_LRNG_DRNG_SWITCH)	+= lrng_switch.o
obj-$(CONFIG_LRNG_KCAPI_HASH)	+= lrng_kcapi_hash.o
obj-$(CONFIG_LRNG_DRBG)		+= lrng_drbg.o
obj-$(CONFIG_LRNG_KCAPI)	+= lrng_kcapi.o
obj-$(CONFIG_LRNG_JENT)		+= lrng_es_jent.o
obj-$(CONFIG_LRNG_HEALTH_TESTS)	+= lrng_health.o
obj-$(CONFIG_LRNG_TESTING)	+= lrng_testing.o
obj-$(CONFIG_LRNG_SELFTEST)	+= lrng_selftest.o

obj-$(CONFIG_LRNG_SWITCH)		+= lrng_switch.o
obj-$(CONFIG_LRNG_HASH_KCAPI)		+= lrng_hash_kcapi.o
obj-$(CONFIG_LRNG_DRNG_CHACHA20)	+= lrng_drng_chacha20.o

obj-$(CONFIG_LRNG_DRNG_KCAPI)		+= lrng_drng_kcapi.o
obj-$(CONFIG_LRNG_DRNG_ATOMIC)		+= lrng_drng_atomic.o

obj-$(CONFIG_LRNG_TIMER_COMMON)		+= lrng_es_timer_common.o
obj-$(CONFIG_LRNG_IRQ)			+= lrng_es_irq.o
obj-$(CONFIG_LRNG_KERNEL_RNG)		+= lrng_es_krng.o
obj-$(CONFIG_LRNG_SCHED)		+= lrng_es_sched.o

obj-$(CONFIG_LRNG_COMMON_DEV_IF)	+= lrng_interface_dev_common.o
obj-$(CONFIG_LRNG_RANDOM_IF)		+= lrng_interface_random_user.o \
					   lrng_interface_random_kernel.o \
					   lrng_interface_aux.o

obj-$(CONFIG_LRNG_KCAPI_IF)		+= lrng_interface_kcapi.o
