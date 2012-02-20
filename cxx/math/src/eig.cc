/**
 * @file cxx/math/src/eig.cc
 * @date Mon May 16 21:45:27 2011 +0200
 * @author Laurent El Shafey <Laurent.El-Shafey@idiap.ch>
 *
 * Copyright (C) 2011-2012 Idiap Research Institute, Martigny, Switzerland
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "math/eig.h"
#include "math/Exception.h"
#include "core/array_assert.h"
#include "core/array_check.h"
#if !defined (HAVE_BLITZ_TINYVEC2_H)
#include <blitz/tinyvec-et.h>
#endif
#include <vector>
#include <utility>
#include <algorithm>

namespace math = bob::math;
namespace ca = bob::core::array;

// Declaration of the external LAPACK functions
// Eigenvalue decomposition of real symmetric matrix (dsyev)
extern "C" void dsyev_( char *jobz, char *uplo, int *N, double *A, 
  int *lda, double *W, double *work, int *lwork, int *info);
// Generalized eigenvalue decomposition of real symmetric matrices (dsygv)
extern "C" void dsygv_( int *itype, char *jobz, char *uplo, int *N, double *A, 
  int *lda, double *B, int *ldb, double *W, double *work, int *lwork, 
  int *info);
// Generalized eigenvalue decomposition of matrices (dggev)
extern "C" void dggev_( char *jobvl, char *jobvr, int *N, double *A, 
  int *lda, double *B, int*ldb, double *alphar, double *alphai, double *beta,
  double *vl, int *ldvl, double *vr, int *ldvr, double *work, int *lwork, 
  int *info);

void math::eigSymReal(const blitz::Array<double,2>& A, 
  blitz::Array<double,2>& V, blitz::Array<double,1>& D)
{
  // Size variable
  int N = A.extent(0);
  const blitz::TinyVector<int,1> shape1(N);
  const blitz::TinyVector<int,2> shape2(N,N);
  ca::assertZeroBase(A);
  ca::assertZeroBase(V);
  ca::assertZeroBase(D);

  ca::assertSameShape(A,shape2);
  ca::assertSameShape(V,shape2);
  ca::assertSameShape(D,shape1);

  math::eigSymReal_(A, V, D);
}

void math::eigSymReal_(const blitz::Array<double,2>& A, 
  blitz::Array<double,2>& V, blitz::Array<double,1>& D)
{
  // Size variable
  int N = A.extent(0);


  ///////////////////////////////////
  // Prepare to call LAPACK function

  // Initialize LAPACK variables
  char jobz = 'V'; // Get both the eigenvalues and the eigenvectors
  char uplo = 'U';
  int info = 0;  
  int lda = N;
  int lwork = 2*std::max(1,3*N-1);

  // Initialize LAPACK arrays
  double *work = new double[lwork];
  double *A_lapack = new double[N*N];
  for(int j=0; j<N; ++j)
    for(int i=0; i<N; ++i)
      A_lapack[j+i*N] = A(j,i);
  double *D_lapack;
  bool D_direct_use = ca::isCZeroBaseContiguous(D);
  if( !D_direct_use )
    D_lapack = new double[N];
  else
    D_lapack = D.data();
 
  // Call the LAPACK function 
  dsyev_( &jobz, &uplo, &N, A_lapack, &lda, D_lapack, work, &lwork, &info);
 
  // Check info variable
  if( info != 0)
    throw bob::math::LapackError("The LAPACK dsyev function returned a non-zero value.");

  // Copy singular vectors back to V (column-major order)
  for(int j=0; j<N; ++j)
    for(int i=0; i<N; ++i)
      V(j,i) = A_lapack[j+i*N];

  // Copy result back to sigma if required
  if( !D_direct_use )
    for(int i=0; i<N; ++i)
      D(i) = D_lapack[i];

  // Free memory
  if( !D_direct_use )
    delete [] D_lapack;
  delete [] A_lapack;
  delete [] work;
}


void math::eigSym(const blitz::Array<double,2>& A, const blitz::Array<double,2>& B,
  blitz::Array<double,2>& V, blitz::Array<double,1>& D)
{
  // Size variable
  int N = A.extent(0);
  const blitz::TinyVector<int,1> shape1(N);
  const blitz::TinyVector<int,2> shape2(N,N);
  ca::assertZeroBase(A);
  ca::assertZeroBase(B);
  ca::assertZeroBase(V);
  ca::assertZeroBase(D);

  ca::assertSameShape(A,shape2);
  ca::assertSameShape(B,shape2);
  ca::assertSameShape(V,shape2);
  ca::assertSameShape(D,shape1);

  math::eigSym_(A, B, V, D);
}

void math::eigSym_(const blitz::Array<double,2>& A, const blitz::Array<double,2>& B,
  blitz::Array<double,2>& V, blitz::Array<double,1>& D)
{
  // Size variable
  int N = A.extent(0);


  ///////////////////////////////////
  // Prepare to call LAPACK function

  // Initialize LAPACK variables
  int itype = 1;
  char jobz = 'V'; // Get both the eigenvalues and the eigenvectors
  char uplo = 'U';
  int info = 0;  
  int lda = N;
  int ldb = N;
  int lwork = 2*std::max(1,3*N-1);

  // Initialize LAPACK arrays
  double *work = new double[lwork];
  double *A_lapack = new double[N*N];
  double *B_lapack = new double[N*N];
  for(int j=0; j<N; ++j)
    for(int i=0; i<N; ++i)
    {
      A_lapack[j+i*N] = A(j,i);
      B_lapack[j+i*N] = B(j,i);
    }
  double *D_lapack;
  bool D_direct_use = ca::isCZeroBaseContiguous(D);
  if( !D_direct_use )
    D_lapack = new double[N];
  else
    D_lapack = D.data();
 
  // Call the LAPACK function 
  dsygv_( &itype, &jobz, &uplo, &N, A_lapack, &lda, B_lapack, &ldb, D_lapack, 
    work, &lwork, &info);

  // Check info variable
  if( info != 0)
    throw bob::math::LapackError("The LAPACK dsygv function returned a \
      non-zero value. This might be caused by a non-positive definite B \
      matrix.");
 
  // Copy singular vectors back to V (column-major order)
  for(int j=0; j<N; ++j)
    for(int i=0; i<N; ++i)
      V(j,N-i-1) = A_lapack[j+i*N];

  // Copy result back to sigma if required
  if( !D_direct_use ) {
    for(int i=0; i<N; ++i) D(N-i-1) = D_lapack[i];
  }
  else { //order the blitz array directly using the C-style pointers
    std::reverse(&D_lapack[0], &D_lapack[N]);
  }

  // Free memory
  if( !D_direct_use )
    delete [] D_lapack;
  delete [] A_lapack;
  delete [] B_lapack;
  delete [] work;
}

/**
 * STL-conforming predicate to sort the eigen values by magnitude and preserve
 * the ordering information.
 */
static bool cmp_ev (const std::pair<double,int>& l, const std::pair<double,int>& r) {
  return l.first > r.first; 
}


void math::eig(const blitz::Array<double,2>& A, const blitz::Array<double,2>& B,
  blitz::Array<double,2>& V, blitz::Array<double,1>& D)
{
  // Size variable
  int N = A.extent(0);
  const blitz::TinyVector<int,1> shape1(N);
  const blitz::TinyVector<int,2> shape2(N,N);
  ca::assertZeroBase(A);
  ca::assertZeroBase(B);
  ca::assertZeroBase(V);
  ca::assertZeroBase(D);

  ca::assertSameShape(A,shape2);
  ca::assertSameShape(B,shape2);
  ca::assertSameShape(V,shape2);
  ca::assertSameShape(D,shape1);

  math::eig_(A, B, V, D);
}

void math::eig_(const blitz::Array<double,2>& A, const blitz::Array<double,2>& B,
  blitz::Array<double,2>& V, blitz::Array<double,1>& D)
{
  // Size variable
  int N = A.extent(0);


  ///////////////////////////////////
  // Prepare to call LAPACK function

  // Initialize LAPACK variables
  char jobvl = 'N'; // Ignore left eigenvalues/vectors
  char jobvr = 'V'; // Get both the right eigenvalues and the eigenvectors
  int info = 0;  
  int lda = N;
  int ldb = N;
  int ldvl = 1;
  int ldvr = N;
  int lwork = std::max(1,8*N);

  // Initialize LAPACK arrays
  double *work = new double[lwork];
  double *alphar = new double[N];
  double *alphai = new double[N];
  double *beta = new double[N];
  double *vl = new double[ldvl*N];
  double *vr = new double[ldvr*N];
  double *A_lapack = new double[N*N];
  double *B_lapack = new double[N*N];
  for(int j=0; j<N; ++j)
    for(int i=0; i<N; ++i)
    {
      A_lapack[j+i*N] = A(j,i);
      B_lapack[j+i*N] = B(j,i);
    }
 
  // Call the LAPACK function 
  dggev_( &jobvl, &jobvr, &N, A_lapack, &lda, B_lapack, &ldb, alphar, alphai, 
    beta, vl, &ldvl, vr, &ldvr, work, &lwork, &info);

  // Check info variable
  if( info != 0)
    throw bob::math::LapackError("The LAPACK dggev function returned a \
      non-zero value.");

  // AA: Re-ordering using std::vector<std::pair<value, index> >
  std::vector<std::pair<double, int> > eigv_index(N);
  for(int i=0; i<N; ++i) {
    // Check if alphai is zero (otherwise complex eigenvalue)
    if( fabs(alphai[i])>1e-12 )
      throw bob::math::LapackError("The LAPACK dggev function returned a \
        non-real (i.e. complex) eigenvalue.");
    eigv_index[i].first = alphar[i] / beta[i];
    eigv_index[i].second = i;
  }
  std::sort(eigv_index.begin(), eigv_index.end(), cmp_ev);
  //eigv_index should now be sorted by descending order

  // Copy singular vectors back to V (column-major order)
  for(int j=0; j<N; ++j)
    for(int i=0; i<N; ++i)
      V(j,eigv_index[i].second) = vr[j+i*N];

  // Copy eigen values back to D
  for(int i=0; i<N; ++i) { D(i) = eigv_index[i].first; }

  // Free memory
  delete [] work;
  delete [] alphar;
  delete [] alphai;
  delete [] beta;
  delete [] vl;
  delete [] vr;
  delete [] A_lapack;
  delete [] B_lapack;
}