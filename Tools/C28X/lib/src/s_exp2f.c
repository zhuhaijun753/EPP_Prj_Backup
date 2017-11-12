/*
 * Copyright (c) 2015-2015 Texas Instruments Incorporated
 *
 * Copyright (c) 2005 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <float.h>
#include <stdint.h>
#include "math.h"
#include "math_private.h"

#define	TBLBITS	4
#define	TBLSIZE	(1 << TBLBITS)

_DATA_ACCESS static const float
    redux   = 0x1.8p23f / TBLSIZE,
    P1	    = 0x1.62e430p-1f,
    P2	    = 0x1.ebfbe0p-3f,
    P3	    = 0x1.c6b348p-5f,
    P4	    = 0x1.3b2c9cp-7f;

_DATA_ACCESS static const long double exp2ft[TBLSIZE] = {
	0x1.6a09e667f3bcdp-1L,
	0x1.7a11473eb0187p-1L,
	0x1.8ace5422aa0dbp-1L,
	0x1.9c49182a3f090p-1L,
	0x1.ae89f995ad3adp-1L,
	0x1.c199bdd85529cp-1L,
	0x1.d5818dcfba487p-1L,
	0x1.ea4afa2a490dap-1L,
	0x1.0000000000000p+0L,
	0x1.0b5586cf9890fp+0L,
	0x1.172b83c7d517bp+0L,
	0x1.2387a6e756238p+0L,
	0x1.306fe0a31b715p+0L,
	0x1.3dea64c123422p+0L,
	0x1.4bfdad5362a27p+0L,
	0x1.5ab07dd485429p+0L,
};
	
/*
 * exp2f(x): compute the base 2 exponential of x
 *
 * Accuracy: Peak error < 0.501 ulp; location of peak: -0.030110927.
 *
 * Method: (equally-spaced tables)
 *
 *   Reduce x:
 *     x = 2**k + y, for integer k and |y| <= 1/2.
 *     Thus we have exp2f(x) = 2**k * exp2(y).
 *
 *   Reduce y:
 *     y = i/TBLSIZE + z for integer i near y * TBLSIZE.
 *     Thus we have exp2(y) = exp2(i/TBLSIZE) * exp2(z),
 *     with |z| <= 2**-(TBLSIZE+1).
 *
 *   We compute exp2(i/TBLSIZE) via table lookup and exp2(z) via a
 *   degree-4 minimax polynomial with maximum error under 1.4 * 2**-33.
 *   Using long double precision for everything except the reduction makes
 *   roundoff error insignificant and simplifies the scaling step.
 *
 *   This method is due to Tang, but I do not use his suggested parameters:
 *
 *	Tang, P.  Table-driven Implementation of the Exponential Function
 *	in IEEE Floating-Point Arithmetic.  TOMS 15(2), 144-157 (1989).
 */
float
exp2f(float x)
{
	long double tv, twopk, u, z;
	float t;
	uint32_t hx, ix, i0;
	int32_t k;

	/* Filter out exceptional cases. */
	GET_FLOAT_WORD(hx, x);
	ix = hx & 0x7fffffff;		/* high word of |x| */
	if(ix >= 0x43000000) {			/* |x| >= 128 */
		if(ix >= 0x7f800000) {
			if ((ix & 0x7fffff) != 0 || (hx & 0x80000000) == 0)
				return (x + x);	/* x is NaN or +Inf */
			else 
				return (0.0);	/* x is -Inf */
		}
		if(x >= 0x1.0p7f)
                {
                    __raise_overflow();
                    return INFINITY;
                }
		if(x <= -0x1.2cp7f)
                {
                    __raise_underflow();
                    return 0;
                }
	} else if (ix <= 0x33000000) {		/* |x| <= 0x1p-25 */
		return (1.0f + x);
	}

	/* Reduce x, computing z, i0, and k. */
	STRICT_ASSIGN(float, t, x + redux);
	GET_FLOAT_WORD(i0, t);
	i0 += TBLSIZE / 2;
#if LDBL_MANT_DIG >= 53
        /* k is not an integer, it is the top 32 bits of the
           IEEE-format 64-bit long double.  The shift by 20 is to
           align it correctly (53 bits of mantissa - 32 bits in the
           low half) */
	k = (i0 >> TBLBITS) << 20;
#else
        /* k is not an integer, it is the bits of the IEEE-format
           32-bit long double.  The shift by 23 is to align it
           correctly (23 mantissa bits) */
	k = (i0 >> TBLBITS) << 23;
#endif
	i0 &= TBLSIZE - 1;
	t -= redux;
	z = x - t;
#if LDBL_MANT_DIG >= 53
	INSERT_WORDS(twopk, 0x3ff00000 + k, 0);
#else
	SET_FLOAT_WORD(twopk, 0x3f800000 + k);
#endif

	/* Compute r = exp2(y) = exp2ft[i0] * p(z). */
	tv = exp2ft[i0];
	u = tv * z;
	tv = tv + u * (P1 + z * P2) + u * (z * z) * (P3 + z * P4);

	/* Scale by 2**(k>>20). */
	return (tv * twopk);
}

#if DBL_MANT_DIG == FLT_MANT_DIG
double exp2(double x) __attribute__((__alias__("exp2f")));
#endif

#if LDBL_MANT_DIG == FLT_MANT_DIG
long double exp2l(long double x) __attribute__((__alias__("exp2f")));
#endif
