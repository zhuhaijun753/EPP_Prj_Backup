/*
 * Copyright (c) 2015-2015 Texas Instruments Incorporated
 *
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/* sinh(x)
 * Method : 
 * mathematically sinh(x) if defined to be (exp(x)-exp(-x))/2
 *	1. Replace x by |x| (sinh(-x) = -sinh(x)). 
 *	2. 
 *		                                    E + E/(E+1)
 *	    0        <= x <= 22     :  sinh(x) := --------------, E=expm1(x)
 *			       			        2
 *
 *	    22       <= x <= lnovft :  sinh(x) := exp(x)/2 
 *	    lnovft   <= x <= ln2ovft:  sinh(x) := exp(x/2)/2 * exp(x/2)
 *	    ln2ovft  <  x	    :  sinh(x) := x*shuge (overflow)
 *
 * Special cases:
 *	sinh(x) is x if x is +INF, -INF, or NaN.
 *	only sinh(0)=0 is exact for finite x.
 */

#include <float.h>
#include <stdint.h>
#include "math.h"
#include "math_private.h"

_DATA_ACCESS static const __ieee64_t one = 1.0, shuge = C(1.0e307);

__ieee64_t
F(sinh)(__ieee64_t x)
{
	__ieee64_t t,h;
	int32_t ix,jx;

    /* High word of |x|. */
	GET_HIGH_WORD(jx,x);
	ix = jx&0x7fffffff;

    /* x is INF or NaN */
	if(ix>=0x7ff00000) return x;

	h = 0.5;
	if (jx<0) h = -h;
    /* |x| in [0,22], return sign(x)*0.5*(E+E/(E+1))) */
	if (ix < 0x40360000) {		/* |x|<22 */
	    if (ix<0x3e300000) 		/* |x|<2**-28 */
		if(shuge+x>one) return x;/* sinh(tiny) = tiny with inexact */
	    t = F(expm1)(F(fabs)(x));
	    if(ix<0x3ff00000) return h*(2.0*t-t*t/(t+one));
	    return h*(t+t/(t+one));
	}

    /* |x| in [22, log(maxdouble)] return 0.5*exp(|x|) */
	if (ix < 0x40862E42)  return h*F(exp)(F(fabs)(x));

    /* |x| in [log(maxdouble), overflowthresold] */
	if (ix<=0x408633CE)
	    return h*2.0*__ldexp_exp(F(fabs)(x), -1);

    /* |x| > overflowthresold, sinh(x) overflow */
        __raise_overflow();
        return F(copysign)(INFINITY, x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
long double sinhl(long double x) __attribute__((__alias__("sinh")));
#endif
