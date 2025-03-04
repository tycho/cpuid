/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2025, Steven Noonan <steven@uplinklabs.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef __vendor_h
#define __vendor_h

typedef enum
{
	VENDOR_UNKNOWN      = 0x0000,
	VENDOR_INTEL        = 0x0001,
	VENDOR_AMD          = 0x0002,
	VENDOR_CYRIX        = 0x0004,
	VENDOR_TRANSMETA    = 0x0008,
	VENDOR_HYGON        = 0x0010, /* Chinese-manufactured AMD EPYC clone. */
	VENDOR_CENTAUR      = 0x0020,

	VENDOR_HV_XEN       = 0x0100,
	VENDOR_HV_VMWARE    = 0x0200,
	VENDOR_HV_KVM       = 0x0400,
	VENDOR_HV_HYPERV    = 0x0800,
	VENDOR_HV_PARALLELS = 0x1000,
	VENDOR_HV_BHYVE     = 0x2000,
	VENDOR_HV_GENERIC   = 0x4000,

	VENDOR_CPU_MASK     = 0x00ff,
	VENDOR_HV_MASK      = 0xff00,

	VENDOR_ANY          = (int)-1
} cpu_vendor_t;

#endif

/* vim: set ts=4 sts=4 sw=4 noet: */
