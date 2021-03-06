#ifdef DUMMY_IFDEF_FOR_FORTRAN
// -*- c -*-
// $Id: power.h,v 1.2 2005/05/12 04:27:34 olsonse Exp $
/*@HEADER
 *         olson-tools:  A variety of routines and algorithms that
 *      I've developed and collected over the past few years.  This collection
 *      represents tools that are most useful for scientific and numerical
 *      software.  This software is released under the LGPL license except
 *      otherwise explicitly stated in individual files included in this
 *      package.  Generally, the files in this package are copyrighted by
 *      Spencer Olson--exceptions will be noted.   
 *                 Copyright 2004-2009 Spencer E. Olson
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *  
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *                                                                                 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.                                                                           .
 * 
 * Questions? Contact Spencer Olson (olsonse@umich.edu) 
 */

/*
 * $Log: power.h,v $
 * Revision 1.2  2005/05/12 04:27:34  olsonse
 * Fixed to for Intel 8.1 compilers.
 * Found (using intel compiler) and fixed an array overflow in BField::potential.
 * Didn't find it earlier because the array is on the stack for the function.
 *
 * Added fmacros.h file to simplify mixing fortran code with others.
 * Added alias function names for Fortran interoperability.
 *
 * Revision 1.1.1.1  2005/01/08 04:27:24  olsonse
 * Initial import
 *
 */
#endif /* DUMMY_IFDEF_FOR_FORTRAN */

/** \file
 * pow(x,y) == \f$ x^{y} \f$.
 * Copyright Spencer Olson 2004-2009 -- All rights reserved.
 *
 * Insanity drove me to write my own pow function.
 * This was actually my first assembly project.
 *
 * Note that this does not necessarily perform exactly according to ieee754 standards,
 * but oh well. 
 * 
 * Timing: the examples compare the timing for my own pow() function with the
 * pow() function that is built in with whatever compiler is used.  In my own
 * experience, I haven't yet found an implementation that beats my with
 * respect to execution time.  As an example, I typically observe that my
 * routine is several times faster than the one provided by GCC. Although
 * better, the Portland Group compiler does not close the gap much.  The
 * closest comparison I've found is from Intel's compiler suite.  
 * 
 * Doing the following showed a total error of ~1.0e-12:
 * \verbatim
   x = 1.9
   for y = -1000:.01:1000
      tot_err += fabs( (pow(x,y) - fast_pow(x,y))/pow(x,y) )
   end
   \endverbatim
 *
 * With another test, I was able to get the total error up to 1.70605e-10 by
 * \verbatim
   for x = -100:0.1:110
       for y = -150:0.01:150
           tot_err += fabs( (pow(x,y) - fast_pow(x,y))/pow(x,y) )
       end
   end
   \endverbatim
 *
 */

#ifndef xylose_power_h
#define xylose_power_h

#ifdef __cplusplus

/* include cmath so that we get log2 and std::pow */
#  if !defined(USE_SPENCERS_FAST_POW) && !defined(DOXYGEN_SKIP)

#    include <xylose/compat/math.hpp>

#    if defined(log2) || defined(_MSC_VER)
       /* some compilers use a define for log2.
        * Microsoft just doesn't seem to provide it at all...
        */
#      undef log2
       template < typename T >
       inline T log2( const T & arg ) {
         return std::log(arg) / M_LN2;
       }
#    elif defined( __PGIC__ )
       /* PGI sucks.  This is only needed for their c++ compiler. */
       extern "C" double log2(double);
#    endif

#  endif // not (USE_SPENCERS_FAST_POW)

namespace xylose {
#else
#  if !defined(USE_SPENCERS_FAST_POW) && !defined(DOXYGEN_SKIP)
#     include <xylose/compat/math.hpp>
#  endif
#endif 




#ifdef SQR
!  error  SQR already defined.  Better figure out what to do here now.
#else
   /** A faster way of computing \f$ x^{2} \f$. */
#  ifdef __cplusplus
    template <class T>
    inline T SQR(const T & t) { return t * t; }
#  else 
#    define SQR(x)	((x)*(x))
#  endif
#endif

#ifdef CUBE
!  error  CUBE already defined.  Better figure out what to do here now.
#else
   /** A faster way of computing \f$ x^{3} \f$. */
#  ifdef __cplusplus
    template <class T>
    inline T CUBE(const T & t) { return t * t * t; }
#  else 
#    define CUBE(x)	((x)*(x)*(x))
#  endif
#endif


/* **** BEGIN FAST_POW **** */
#if !defined(LANGUAGE_FORTRAN__) || LANGUAGE_FORTRAN__ != 1

# if defined(USE_SPENCERS_FAST_POW) || defined(DOXYGEN_SKIP)

#   define FAST_POW_CODE(derefd_x, derefd_y)                          \
    register double value;                                            \
                                                                      \
    /*                                                                \
      here's the pseudo code for the following:                       \
                                                                      \
    if (y == 0) return 1;                                             \
    if (x == 0) return 0;                                             \
                                                                      \
    return x^y;                                                       \
    */                                                                \
    asm (                                                             \
      /* first test for y == 0 */                                     \
      "fxam                    # st(0) == 0 ?                     \n" \
      "fnstsw %%ax             # mov ax, fpu status word          \n" \
      /* the fpu status word condition bits after fnstsw are          \
       * x1xxx0x0 xxxxxxxx                                            \
       * if st(0) == +-0                                              \
       *  Also note that 0x4500 ==  01000101 00000000                 \
       *  and            0x4000 ==  01000000 00000000                 \
       */                                                             \
      "and $0x45,%%ah          # fpu status word & ???            \n" \
      "cmp $0x40,%%ah          #                                  \n" \
      "je 1f                   # jne (y == 0; return1;)           \n" \
      /* now test the x value */                                      \
      "fxch                    #                                  \n" \
      "fxam                    # st(0) == 0 ?                     \n" \
      "fnstsw %%ax             # mov ax, fpu status word          \n" \
      /* the fpu status word condition bits after fnstsw are          \
       * x1xxx0x0 xxxxxxxx                                            \
       * if st(0) == +-0                                              \
       *  Also note that 0x4500 ==  01000101 00000000                 \
       *  and            0x4000 ==  01000000 00000000                 \
       */                                                             \
      "and $0x45,%%ah          # fpu status word & ???            \n" \
      "cmp $0x40,%%ah          #                                  \n" \
      "je 2f                   # jne (x == 0; return 0;)          \n" \
      /* ***now finally do the hard work of computing x^y*** */       \
      "fyl2x                   # yl2x: st(0) = st(1) * log2(st(0))\n" \
      "fld        %%st(0)      # st(1) = st(0) = yl2x             \n" \
      "frndint                 # int(yl2x): st(0) = int( st(0) )  \n" \
      "fxch                    # swap( st(0), st(1) )             \n" \
      "fsub        %%st(1)     # fract(yl2x): st(0) -= st(1)      \n" \
      "f2xm1                   # st(0) = 2^( st(0) ) - 1          \n" \
      "fld1                    # st(0) = 1, st(1) = (2^yl2x) - 1  \n" \
      "faddp  %%st(0),%%st(1)  # st(0) = 2^yl2x [st(1)= int(yl2x)]\n" \
      "fscale                  #                                  \n" \
      /* ***hard work done!*** */                                     \
      "jmp 3f                  # jmp finished                     \n" \
"1:                            # y == 0; return 1;                \n" \
      "fld1                    # put 1 in st(0)                   \n" \
      "fstp   %%st(1)          #                                  \n" \
      "jmp 3f                  # jmp finished                     \n" \
"2:                            # x == 0; return 0;                \n" \
      "fldz                    # put 0 in st(0)                   \n" \
      "fstp   %%st(1)          #                                  \n" \
    /*"jmp 3f                  # jmp finished \n" */                  \
"3:                            # finished                         \n" \
      : "=t" (value) : "f" ((derefd_x)), "0" ((derefd_y))             \
      : "eax"                                                         \
    );                                                                \
                                                                      \
    return value;                                                     \
    /*end fast pow code */


#   if defined(__cplusplus) || defined(DOXYGEN_SKIP)
/** My own pow function.
 * This function only really compiles for x86 based machines.  For other
 * machihnes USE_SPENCERS_FAST_POW should not be defined.  In this case,
 * fast_pow will revert to a simple alias for std::pow.
 *
 * This function can also be called from c.  The c-version will of course be
 * non-c++ name-mangled and takes values as arguments (not references).
 *
 * This function can also be called from fortran.  For fortran, there is a
 * version of this function with an extra underscore at the end.  The fortran
 * function will end up having at least two additional instructions (mov) for
 * dereferencing the pointers to x, and y.
 *
 * I regularly observe this function perform around 10 times faster than GCC's
 * libm::pow function and 6 times faster than that provided by PGI.  Intel seems
 * to be my only rival yet (that I've seen).
 *
 * @param x The base.
 * @param y The exponent.
 * @return \f$ x^{y} \f$
 * */
inline double fast_pow( const double & x, const double & y ) {
    FAST_POW_CODE(x,y);
}
#   else /* else lang = c */

double fast_pow(double x,double y);

#   endif /* else lang = c */

# else /* don't use spencers fast pow family of functions */
    /* the least we can do is provide an alias macro for the math lib pow
     * function. */
#   define fast_pow pow
#   ifdef __cplusplus
      /* import std::pow into this namespace for aliasing */
      using std::pow;
#   endif

# endif /* USE_SPENCERS_FAST_POW */
#endif /*  if LANGUAGE_FORTRAN__ != 1 */
/* ****   END FAST_POW **** */



/* **** BEGIN FAST_LOG2 **** */
#if !defined(LANGUAGE_FORTRAN__) || LANGUAGE_FORTRAN__ != 1
# if defined(USE_SPENCERS_FAST_POW) || defined(DOXYGEN_SKIP)

#   define FAST_LOG2_CODE(derefd_x)                                   \
    register double value;                                            \
    asm (                                                             \
      "fld1       # st(0) = 1                   \n"                   \
      "fxch       # swap( st(0), st(1) )        \n"                   \
      "fyl2x      # st(0) = st(1) * log2(st(0)) \n"                   \
      : "=t" (value) : "0" (derefd_x)                                 \
    );                                                                \
    return value;                                                     \
    /*end fast log2 code */

#   if defined(__cplusplus) || defined(DOXYGEN_SKIP)

/** My own log2 function.
 * This function only really compiles for x86 based machines.  For other
 * machihnes USE_SPENCERS_FAST_POW should not be defined.  In this case,
 * fast_log2 will revert to a simple macro alias for log2.
 *
 * This function can also be called from c.  The c-version will of course be
 * non-c++ name-mangled and takes values as arguments (not references).
 *
 * This function can also be called from fortran.  For fortran, there is a
 * version of this function with an extra underscore at the end.  The fortran
 * function will end up having at least one additional instruction (mov) for
 * dereferencing the pointer to x.
 *
 * I regularly observe this function perform around 40 times faster than GCC's
 * libm::log2 function.
 * */
inline double fast_log2 ( const double & x ) {
  FAST_LOG2_CODE(x);
}

#   else /* else lang = c */
double fast_log2 ( const double x );
#   endif /* else lang = c */
# else /* don't use spencers fast pow family of functions */

    /* At least define an alias macro for math lib log2 */
#   define fast_log2 log2
#   ifdef __cplusplus
      /* import std::log2 into this namespace for aliasing */
      using ::log2;
#   endif
# endif /* USE_SPENCERS_FAST_POW || DOXYGEN_SKIP */


# ifdef __cplusplus
/** Simple function to compute the log of a number in the base of another
 * number.  This function uses fast_log2 when available (log2 from math lib
 * otherwise).  
 *
 * This function can also be called from c.  The c-version will of course be
 * non-c++ name-mangled and takes values as arguments (not references).
 *
 * This function can also be called from fortran.  For fortran, there is a
 * version of this function with an extra underscore at the end.
 *
 * @returns \f$ \log_{n}\right(x\left) \f$
 * */
inline double logn ( const double & n, const double & x ) {
  return fast_log2( x ) / fast_log2( n );
}

# else /* else lang = c */
double logn ( double n, double x );
# endif /* else lang = c */
#endif /* if LANGUAGE_FORTRAN__ != 1 */
/* ****   END FAST_LOG2 **** */



#ifdef __cplusplus
  namespace detail {
    /** generic implementation of x^y where y is an int.  @see powN */
    template < int p >
    struct powNImpl {
      template < typename T >
      inline T operator() ( const T & t ) {
        return fast_pow(t,p);
      }
    };

    /** Implementation of x^y where y = (int)1.  @see powN */
    template<>
    struct powNImpl<1> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return t;
      }
    };

    /** Implementation of x^y where y = (int)2.  @see powN */
    template<>
    struct powNImpl<2> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return t*t;
      }
    };

    /** Implementation of x^y where y = (int)3.  @see powN */
    template<>
    struct powNImpl<3> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return t*t*t;
      }
    };

    /** Implementation of x^y where y = (int)4.  @see powN */
    template<>
    struct powNImpl<4> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return t*t*t*t;
      }
    };

    /** Implementation of x^y where y = (int)5.  @see powN */
    template<>
    struct powNImpl<5> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return t*t*t*t*t;
      }
    };

    /** Implementation of x^y where y = (int)-1.  @see powN */
    template<>
    struct powNImpl<-1> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return 1/t;
      }
    };

    /** Implementation of x^y where y = (int)-2.  @see powN */
    template<>
    struct powNImpl<-2> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return 1/(t*t);
      }
    };

    /** Implementation of x^y where y = (int)-3.  @see powN */
    template<>
    struct powNImpl<-3> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return 1/(t*t*t);
      }
    };

    /** Implementation of x^y where y = (int)-4.  @see powN */
    template<>
    struct powNImpl<-4> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return 1/(t*t*t*t);
      }
    };

    /** Implementation of x^y where y = (int)-5.  @see powN */
    template<>
    struct powNImpl<-5> {
      template < typename T >
      inline T operator() ( const T & t ) {
        return 1/(t*t*t*t*t);
      }
    };

  }/*namespace xylose::detail */

  /** x^y where y = (int)y.
   *
   * Note:  This actually calls the correct implementation of detail::powNImpl.
   *
   * @param p
   *   Integer power (y) used in x^y.
   * @param t
   *   Value to be exponentiated by power (y).
   */
  template < int p, typename T >
  inline T powN( const T & t ) {
    return detail::powNImpl<p>()(t);
  }
#endif 



#ifdef __cplusplus
}/* namespace xylose */
#endif 

#endif /* xylose_power_h */
