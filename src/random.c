/**************************************************************************
*   File: random.c                                         Part of tbaMUD *
*  Usage: Pseudo-random number generator.                                 *
**************************************************************************/

/* I am bothered by the non-portablility of 'rand' and 'random' -- rand is ANSI
 * C, but on some systems such as Suns, rand has seriously tragic spectral 
 * properties (the low bit alternates between 0 and 1!).  random is better but 
 * isn't supported by all systems.  So, in my quest for portability, I decided 
 * to include this code for a simple but relatively effective random number 
 * generator.  It's not the best RNG code around, but I like it because it's 
 * very short and simple, and for our purposes it's "random enough". -JE */

/* This program is public domain and was written by William S. England. It is 
 * based on an article by: Stephen K. Park and Keith W. Miller.
 * The following is a portable c program for generating random numbers. The 
 * modulus and multipilier have been extensively tested and should not be 
 * changed except by someone who is a professional Lehmer generator writer. 
 * THIS GENERATOR REPRESENTS THE MINIMUM STANDARD AGAINST WHICH OTHER 
 * GENERATORS SHOULD BE JUDGED. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h" /* for function prototypes */

#define	m  (unsigned long)2147483647
#define	q  (unsigned long)127773

#define	a (unsigned int)16807
#define	r (unsigned int)2836

/* F(z)	= (az)%m
**	= az-m(az/m)
**
** F(z)  = G(z)+mT(z)
** G(z)  = a(z%q)- r(z/q)
** T(z)  = (z/q) - (az/m)
**
** F(z)  = a(z%q)- rz/q+ m((z/q) - a(z/m))
** 	 = a(z%q)- rz/q+ m(z/q) - az */

static unsigned long seed;

void circle_srandom(unsigned long initial_seed)
{
    seed = initial_seed;
}

unsigned long circle_random(void)
{
   int lo, hi, test;

    hi   = seed/q;
    lo   = seed%q;

    test = a*lo - r*hi;

    if (test > 0)
	seed = test;
    else
	seed = test+ m;

    return (seed);
}
