/**
 * @file src/cxx/math/test/linear.cc
 * @author <a href="mailto:Laurent.El-Shafey@idiap.ch">Laurent El Shafey</a> 
 *
 * @brief Test basic linear algebra operations
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE math-linear Tests
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "core/logging.h"
#include "math/linear.h"


struct T {
  blitz::Array<double,2> A_24, A_43, A_23;
  blitz::Array<double,1> b_4, b_2, b_5a, b_5b;
  double b5_dot;
  double eps;

  T(): A_24(2,4), A_43(4,3), A_23(2,3), b_4(4), b_2(2), b_5a(5), b_5b(5), 
       b5_dot(99.), eps(1e-3)
  {
    A_24 = 1., 2., 3., 4., 5., 6., 7., 8.;
    A_43 = 12., 11., 10., 9., 8., 7., 6., 5., 4., 3., 2., 1.;
    A_23 = 60., 50., 40., 180., 154., 128.;

    b_4 = 4., 3., 2., 1.;
    b_2 = 20., 60.;

    b_5a = 3., 2., 1., 2., 3.;
    b_5b = 7., 8., 9., 10., 11.;
  }

  ~T() {}
};

template<typename T, typename U, int d>  
void check_dimensions( blitz::Array<T,d>& t1, blitz::Array<U,d>& t2) 
{
  BOOST_REQUIRE_EQUAL(t1.dimensions(), t2.dimensions());
  for( int i=0; i<t1.dimensions(); ++i)
    BOOST_CHECK_EQUAL(t1.extent(i), t2.extent(i));
}

template<typename T, typename U>  
void checkBlitzEqual( blitz::Array<T,1>& t1, blitz::Array<U,1>& t2)
{
  check_dimensions( t1, t2);
  for( int i=0; i<t1.extent(0); ++i)
    BOOST_CHECK_EQUAL(t1(i), Torch::core::cast<T>(t2(i)));
}

template<typename T, typename U>  
void checkBlitzEqual( blitz::Array<T,2>& t1, blitz::Array<U,2>& t2)
{
  check_dimensions( t1, t2);
  for( int i=0; i<t1.extent(0); ++i)
    for( int j=0; j<t1.extent(1); ++j)
      BOOST_CHECK_EQUAL(t1(i,j), Torch::core::cast<T>(t2(i,j)));
}

template<typename T, typename U>  
void checkBlitzEqual( blitz::Array<T,3>& t1, blitz::Array<U,3>& t2) 
{
  check_dimensions( t1, t2);
  for( int i=0; i<t1.extent(0); ++i)
    for( int j=0; j<t1.extent(1); ++j)
      for( int k=0; k<t1.extent(2); ++k)
        BOOST_CHECK_EQUAL(t1(i,j,k), Torch::core::cast<T>(t2(i,j,k)));
}

template<typename T>  
void checkBlitzClose( blitz::Array<T,1>& t1, blitz::Array<T,1>& t2, 
  const double eps )
{
  check_dimensions( t1, t2);
  for( int i=0; i<t1.extent(0); ++i)
    BOOST_CHECK_SMALL( fabs( t2(i)-t1(i) ), eps);
}

template<typename T>  
void checkBlitzClose( blitz::Array<T,2>& t1, blitz::Array<T,2>& t2, 
  const double eps )
{
  check_dimensions( t1, t2);
  for( int i=0; i<t1.extent(0); ++i)
    for( int j=0; j<t1.extent(1); ++j)
      BOOST_CHECK_SMALL( fabs( t2(i,j)-t1(i,j) ), eps);
}

BOOST_FIXTURE_TEST_SUITE( test_setup, T )

BOOST_AUTO_TEST_CASE( test_matrix_matrix_prod )
{
  blitz::Array<double,2> sol(2,3);

  Torch::math::prod( A_24, A_43, sol);
  checkBlitzClose( A_23, sol, eps);
}

BOOST_AUTO_TEST_CASE( test_matrix_vector_prod )
{
  blitz::Array<double,1> sol(2);

  Torch::math::prod( A_24, b_4, sol);
  checkBlitzClose( b_2, sol, eps);
}

BOOST_AUTO_TEST_CASE( test_vector_vector_dot )
{
  double sol = Torch::math::dot( b_5a, b_5b);
  BOOST_CHECK_SMALL( fabs(b5_dot - sol), eps);
}

BOOST_AUTO_TEST_SUITE_END()