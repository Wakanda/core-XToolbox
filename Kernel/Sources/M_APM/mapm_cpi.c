
/* 
 *  M_APM  -  mapm_cpi.c
 *
 *  Copyright (C) 2001  Michael C. Ring
 *
 *  Permission to use, copy, and distribute this software and its
 *  documentation for any purpose with or without fee is hereby granted, 
 *  provided that the above copyright notice appear in all copies and 
 *  that both that copyright notice and this permission notice appear 
 *  in supporting documentation.
 *
 *  Permission to modify the software is granted, but not the right to
 *  distribute the modified code.  Modifications are to be distributed 
 *  as patches to released version.
 *  
 *  This software is provided "as is" without express or implied warranty.
 */

/*
 *      $Id: mapm_cpi.c,v 1.1 2001/03/25 21:01:53 mike Exp $
 *
 *      This file contains the PI related functions.
 *
 *      $Log: mapm_cpi.c,v $
 *      Revision 1.1  2001/03/25 21:01:53  mike
 *      Initial revision
 *
 */

#include "m_apm_lc.h"

/****************************************************************************/
/*
 *	check if our local copy of PI is precise enough
 *	for our purpose. if not, calculate PI so it's
 *	as precise as desired, accurate to 'places' decimal
 *	places.
 */
void	M_check_PI_places(places)
int	places;
{
int     dplaces;

dplaces = places + 2;

if (dplaces > MM_lc_PI_digits)
  {
   MM_lc_PI_digits = dplaces + 2;

   /* compute PI using the AGM  (see right below) */

   M_calculate_PI_AGM(MM_lc_PI, (dplaces + 5));

   m_apm_multiply(MM_lc_HALF_PI, MM_0_5, MM_lc_PI);
   m_apm_multiply(MM_lc_2_PI, MM_Two, MM_lc_PI);
  }
}
/****************************************************************************/
/*
 *      Calculate PI using the AGM (Arithmetic-Geometric Mean)
 *
 *      Init :  A0  = 1
 *              B0  = 1 / sqrt(2)
 *              Sum = 1
 *
 *      Iterate: n = 1...
 *
 *
 *      A   =  0.5 * [ A    +  B   ]
 *       n              n-1     n-1
 *
 *
 *      B   =  sqrt [ A    *  B   ]
 *       n             n-1     n-1
 *
 *
 *
 *      C   =  0.5 * [ A    -  B   ]
 *       n              n-1     n-1
 *
 *
 *                      2      n+1
 *     Sum  =  Sum  -  C   *  2
 *                      n
 *
 *
 *      At the end when C  is 'small enough' :
 *                       n
 *
 *                    2 
 *      PI  =  4  *  A    /  Sum
 *                    n+1
 *
 */
void	M_calculate_PI_AGM(outv, places)
M_APM   outv;
int     places;
{
M_APM   tmp1, tmp2, a0, b0, c0, a1, b1, sum, pow_2;
int     dflag, dplaces, nn;

tmp1  = m_apm_init();
tmp2  = m_apm_init();
a0    = m_apm_init();
b0    = m_apm_init();
c0    = m_apm_init();
a1    = m_apm_init();
b1    = m_apm_init();
sum   = m_apm_init();
pow_2 = m_apm_init();

dplaces = places + 16;
dflag   = FALSE;

m_apm_copy(a0, MM_One);
m_apm_copy(sum, MM_One);
m_apm_copy(pow_2, MM_Four);

m_apm_sqrt(tmp1, dplaces, MM_Two);
m_apm_multiply(b0, tmp1, MM_0_5);

while (TRUE)
  {
   m_apm_add(tmp1, a0, b0);
   m_apm_multiply(a1, MM_0_5, tmp1);

   if (dflag)
     break;

   m_apm_multiply(tmp1, a0, b0);
   m_apm_sqrt(b1, dplaces, tmp1);

   m_apm_subtract(tmp1, a0, b0);
   m_apm_multiply(c0, MM_0_5, tmp1);

   /*
    *   the net 'PI' calculated from this iteration will
    *   be accurate to ~4 X the value of (c0)'s exponent.
    *   this was determined experimentally. we will be
    *   (overly?) conservative and assume the ratio is 
    *   only 3.8 ( or 19 / 5). 
    */

   nn = (-19 * c0->m_apm_exponent) / 5;

   if (nn >= dplaces)
     dflag = TRUE;

   m_apm_multiply(tmp1, c0, c0);
   m_apm_multiply(tmp2, tmp1, pow_2);
   m_apm_subtract(tmp1, sum, tmp2);
   m_apm_round(sum, dplaces, tmp1);

   m_apm_copy(a0, a1);
   m_apm_copy(b0, b1);

   m_apm_multiply(tmp1, pow_2, MM_Two);
   m_apm_copy(pow_2, tmp1);
  }

m_apm_multiply(tmp1, a1, a1);
m_apm_multiply(tmp2, tmp1, MM_Four);
m_apm_divide(tmp1, dplaces, tmp2, sum);
m_apm_round(outv, places, tmp1);

m_apm_free(pow_2);
m_apm_free(sum);
m_apm_free(b1);
m_apm_free(a1);
m_apm_free(c0);
m_apm_free(b0);
m_apm_free(a0);
m_apm_free(tmp2);
m_apm_free(tmp1);
}
/****************************************************************************/
