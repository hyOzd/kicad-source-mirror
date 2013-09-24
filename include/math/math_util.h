/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (c) 2005 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (C) CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __MATH_UTIL_H
#define __MATH_UTIL_H

#include <cmath>
#include <cstdlib>
#include <stdint.h>

/**
 * Function rescale()
 *
 * Scales a number (value) by rational (numerator/denominator). Numerator must be <= denominator.
 */

template<typename T> static T rescale( T numerator, T value, T denominator )
{
    return numerator * value / denominator;
}


// explicit specializations for integer types, taking care of overflow.
template<> int rescale( int numerator, int value, int denominator )
{
    return (int) ( (int64_t) numerator * (int64_t) value / (int64_t) denominator );
}

template<> int64_t rescale( int64_t numerator, int64_t value, int64_t denominator )
{
    int64_t r = 0;
    int64_t sign = ( ( numerator < 0) ? -1 : 1 ) * ( denominator < 0 ? - 1: 1 ) * (value < 0 ? - 1 : 1);

    int64_t a = std::abs( numerator );
    int64_t b = std::abs( value );
    int64_t c = std::abs( denominator );

    r = c / 2;

    if( b <= INT_MAX && c <= INT_MAX )
    {
        if( a <= INT_MAX )
            return sign * ( (a * b + r ) / c );
        else
            return sign * (a / c * b + (a % c * b + r) / c);
    } else {
        uint64_t a0 = a & 0xFFFFFFFF;
        uint64_t a1 = a >> 32;
        uint64_t b0 = b & 0xFFFFFFFF;
        uint64_t b1 = b >> 32;
        uint64_t t1 = a0 * b1 + a1 * b0;
        uint64_t t1a = t1 << 32;
        int i;
   
        a0 = a0 * b0 + t1a;
        a1 = a1 * b1 + (t1 >> 32) + (a0 < t1a);
        a0 += r;
        a1 += ((uint64_t)a0) < r;

        for( i = 63; i >= 0; i-- )
        {
            a1  += a1 + ( (a0 >> i) & 1 );
            t1  += t1;

            if( (uint64_t)c <= a1 )
            {
                a1 -= c;
                t1++;
            }
        }

        return t1 * sign;
    }
};

#endif // __MATH_UTIL_H