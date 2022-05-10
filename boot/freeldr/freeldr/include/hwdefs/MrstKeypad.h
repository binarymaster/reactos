#ifndef __MRST_KEYPAD_H__
#define __MRST_KEYPAD_H__

#define MrstKeypadVendorId 0x8086
#define MrstKeypadDeviceId 0x0805

/*
 * Keypad Controller registers
 */
#define KPC             0x0000 /* Keypad Control register */
#define KPDK            0x0004 /* Keypad Direct Key register */
#define KPREC           0x0008 /* Keypad Rotary Encoder register */
#define KPMK            0x000C /* Keypad Matrix Key register */
#define KPAS            0x0010 /* Keypad Automatic Scan register */

/* Keypad Automatic Scan Multiple Key Presser register 0-3 */
#define KPASMKP0        0x0014
#define KPASMKP1        0x0018
#define KPASMKP2        0x001C
#define KPASMKP3        0x0020
#define KPKDI           0x0024

/* bit definitions */
#define KPC_MKRN(n)	((((n) - 1) & 0x7) << 26) /* matrix key row number */
#define KPC_MKCN(n)	((((n) - 1) & 0x7) << 23) /* matrix key col number */
#define KPC_DKN(n)	((((n) - 1) & 0x7) << 6)  /* direct key number */

#define KPC_AS          (0x1 << 30)  /* Automatic Scan bit */
#define KPC_ASACT       (0x1 << 29)  /* Automatic Scan on Activity */
#define KPC_MI          (0x1 << 22)  /* Matrix interrupt bit */
#define KPC_IMKP        (0x1 << 21)  /* Ignore Multiple Key Press */

#define KPC_MS(n)	(0x1 << (13 + (n)))	/* Matrix scan line 'n' */
#define KPC_MS_ALL      (0xff << 13)

#define KPC_ME          (0x1 << 12)  /* Matrix Keypad Enable */
#define KPC_MIE         (0x1 << 11)  /* Matrix Interrupt Enable */
#define KPC_DK_DEB_SEL	(0x1 <<  9)  /* Direct Keypad Debounce Select */
#define KPC_DI          (0x1 <<  5)  /* Direct key interrupt bit */
#define KPC_RE_ZERO_DEB (0x1 <<  4)  /* Rotary Encoder Zero Debounce */
#define KPC_REE1        (0x1 <<  3)  /* Rotary Encoder1 Enable */
#define KPC_REE0        (0x1 <<  2)  /* Rotary Encoder0 Enable */
#define KPC_DE          (0x1 <<  1)  /* Direct Keypad Enable */
#define KPC_DIE         (0x1 <<  0)  /* Direct Keypad interrupt Enable */

#define KPDK_DKP        (0x1 << 31)
#define KPDK_DK(n)	((n) & 0xff)

#define KPREC_OF1       (0x1 << 31)
#define kPREC_UF1       (0x1 << 30)
#define KPREC_OF0       (0x1 << 15)
#define KPREC_UF0       (0x1 << 14)

#define KPREC_RECOUNT0(n)	((n) & 0xff)
#define KPREC_RECOUNT1(n)	(((n) >> 16) & 0xff)

#define KPMK_MKP        (0x1 << 31)
#define KPAS_SO         (0x1 << 31)
#define KPASMKPx_SO     (0x1 << 31)

#define KPAS_MUKP(n)	(((n) >> 26) & 0x1f)
#define KPAS_RP(n)	(((n) >> 4) & 0xf)
#define KPAS_CP(n)	((n) & 0xf)

#define KPASMKP_MKC_MASK	(0xff)

#define	KEYPAD_MATRIX_GPIO_IN_PIN	24
#define	KEYPAD_MATRIX_GPIO_OUT_PIN	32
#define KEYPAD_DIRECT_GPIO_IN_PIN	40

#define keypad_readl(off)	readl(keypad->mmio_base + (off))
#define keypad_writel(off, v)	writel((v), keypad->mmio_base + (off))

#define MAX_MATRIX_KEY_NUM	(8 * 8)
#define MAX_DIRECT_KEY_NUM	(4)

#define MAX_MATRIX_KEY_ROWS	(8)
#define MAX_MATRIX_KEY_COLS	(8)
#define DEBOUNCE_INTERVAL	100

#define DEFAULT_KPREC	(0x007f007f)


#endif
