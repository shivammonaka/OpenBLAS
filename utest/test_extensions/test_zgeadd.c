/*****************************************************************************
Copyright (c) 2023, The OpenBLAS Project
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
   3. Neither the name of the OpenBLAS project nor the names of
      its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**********************************************************************************/

#include "utest/openblas_utest.h"
#include "common.h"

#define N 100
#define M 100

struct DATA_ZGEADD {
    double a_test[M * N * 2];
    double c_test[M * N * 2];
    double c_verify[M * N * 2];
};

#ifdef BUILD_COMPLEX16
static struct DATA_ZGEADD data_zgeadd;

/**
 * zgeadd reference implementation
 *
 * param m - number of rows of A and C
 * param n - number of columns of A and C
 * param alpha - scaling factor for matrix A
 * param aptr - refer to matrix A
 * param lda - leading dimension of A
 * param beta - scaling factor for matrix C
 * param cptr - refer to matrix C
 * param ldc - leading dimension of C
 */
static void zgeadd_trusted(blasint m, blasint n, double *alpha, double *aptr,
                           blasint lda, double *beta, double *cptr, blasint ldc)
{
    blasint i;

    lda *= 2;
    ldc *= 2;

    for (i = 0; i < n; i++)
    {
        cblas_zaxpby(m, alpha, aptr, 1, beta, cptr, 1);
        aptr += lda;
        cptr += ldc;
    }
}

/**
 * Test zgeadd by comparing it against reference
 * Compare with the following options:
 *
 * param api - specifies Fortran or C API
 * param order - specifies whether A and C stored in
 * row-major order or column-major order
 * param m - number of rows of A and C
 * param n - number of columns of A and C
 * param alpha - scaling factor for matrix A
 * param lda - leading dimension of A
 * param beta - scaling factor for matrix C
 * param ldc - leading dimension of C
 * return norm of differences
 */
static double check_zgeadd(char api, OPENBLAS_CONST enum CBLAS_ORDER order,
                           blasint m, blasint n, double *alpha, blasint lda,
                           double *beta, blasint ldc)
{
    blasint i;
    blasint cols = m, rows = n;

    if (order == CblasRowMajor)
    {
        rows = m;
        cols = n;
    }

    // Fill matrix A, C
    drand_generate(data_zgeadd.a_test, lda * rows * 2);
    drand_generate(data_zgeadd.c_test, ldc * rows * 2);

    // Copy matrix C for zgeadd
    for (i = 0; i < ldc * rows * 2; i++)
        data_zgeadd.c_verify[i] = data_zgeadd.c_test[i];

    zgeadd_trusted(cols, rows, alpha, data_zgeadd.a_test, lda,
                   beta, data_zgeadd.c_verify, ldc);

    if (api == 'F')
        BLASFUNC(zgeadd)(&m, &n, alpha, data_zgeadd.a_test, &lda,
                         beta, data_zgeadd.c_test, &ldc);
    else
        cblas_zgeadd(order, m, n, alpha, data_zgeadd.a_test, lda,
                     beta, data_zgeadd.c_test, ldc);

    // Find the differences between output matrix caculated by zgeadd and sgemm
    return dmatrix_difference(data_zgeadd.c_test, data_zgeadd.c_verify, cols, rows, ldc * 2);
}

/**
 * Check if error function was called with expected function name
 * and param info
 *
 * param api - specifies Fortran or C API
 * param order - specifies whether A and C stored in
 * row-major order or column-major order
 * param m - number of rows of A and C
 * param n - number of columns of A and C
 * param lda - leading dimension of A
 * param ldc - leading dimension of C
 * param expected_info - expected invalid parameter number in zgeadd
 * return TRUE if everything is ok, otherwise FALSE
 */
static int check_badargs(char api, OPENBLAS_CONST enum CBLAS_ORDER order,
                         blasint m, blasint n, blasint lda,
                         blasint ldc, int expected_info)
{
    double alpha[] = {1.0, 1.0};
    double beta[] = {1.0, 1.0};

    set_xerbla("ZGEADD ", expected_info);

    if (api == 'F')
        BLASFUNC(zgeadd)(&m, &n, alpha, data_zgeadd.a_test, &lda,
                         beta, data_zgeadd.c_test, &ldc);
    else
        cblas_zgeadd(order, m, n, alpha, data_zgeadd.a_test, lda,
                     beta, data_zgeadd.c_test, ldc);

    return check_error();
}

/**
 * Fortran API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 */
CTEST(zgeadd, matrix_n_100_m_100)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {3.0, 2.0};
    double beta[] = {1.0, 3.0};

    double norm = check_zgeadd('F', order, m, n, alpha, lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * Fortran API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 * Scalar alpha is zero (operation is C:=beta*C)
 */
CTEST(zgeadd, matrix_n_100_m_100_alpha_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {0.0, 0.0};
    double beta[] = {1.0, 1.0};

    double norm = check_zgeadd('F', order, m, n, alpha, lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * Fortran API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 * Scalar beta is zero (operation is C:=alpha*A)
 */
CTEST(zgeadd, matrix_n_100_m_100_beta_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {3.0, 1.5};
    double beta[] = {0.0, 0.0};

    double norm = check_zgeadd('F', order, m, n, alpha, lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * Fortran API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 * Scalars alpha, beta is zero (operation is C:= 0)
 */
CTEST(zgeadd, matrix_n_100_m_100_alpha_beta_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {0.0, 0.0};
    double beta[] = {0.0, 0.0};

    double norm = check_zgeadd('F', order, m, n, alpha, lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * Fortran API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * For A number of rows is 50, number of colums is 100
 * For C number of rows is 50, number of colums is 100
 */
CTEST(zgeadd, matrix_n_100_m_50)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M / 2;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {1.0, 1.0};
    double beta[] = {1.0, 1.0};

    double norm = check_zgeadd('F', order, m, n, alpha, lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * Fortran API specific test
 * Test error function for an invalid param n -
 * number of columns of A and C
 * Must be at least zero.
 */
CTEST(zgeadd, xerbla_n_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = INVALID;
    blasint m = 1;

    blasint lda = m;
    blasint ldc = m;

    int expected_info = 2;

    int passed = check_badargs('F', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * Fortran API specific test
 * Test error function for an invalid param m -
 * number of rows of A and C
 * Must be at least zero.
 */
CTEST(zgeadd, xerbla_m_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = INVALID;

    blasint lda = 1;
    blasint ldc = 1;

    int expected_info = 1;

    int passed = check_badargs('F', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * Fortran API specific test
 * Test error function for an invalid param lda -
 * specifies the leading dimension of A. Must be at least MAX(1, m).
 */
CTEST(zgeadd, xerbla_lda_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = 1;

    blasint lda = INVALID;
    blasint ldc = 1;

    int expected_info = 6;

    int passed = check_badargs('F', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * Fortran API specific test
 * Test error function for an invalid param ldc -
 * specifies the leading dimension of C. Must be at least MAX(1, m).
 */
CTEST(zgeadd, xerbla_ldc_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = INVALID;

    int expected_info = 8;

    int passed = check_badargs('F', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * Fortran API specific test
 * Check if n - number of columns of A, C equal zero.
 */
CTEST(zgeadd, n_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 0;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = 1;

    double alpha[] = {1.0, 1.0};
    double beta[] = {1.0, 1.0};

    double norm = check_zgeadd('F', order, m, n, alpha, lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * Fortran API specific test
 * Check if m - number of rows of A and C equal zero.
 */
CTEST(zgeadd, m_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = 0;

    blasint lda = 1;
    blasint ldc = 1;

    double alpha[] = {1.0, 1.0};
    double beta[] = {1.0, 1.0};

    double norm = check_zgeadd('F', order, m, n, alpha, lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * c api option order is column-major order
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 */
CTEST(zgeadd, c_api_matrix_n_100_m_100)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {2.0, 1.0};
    double beta[] = {1.0, 3.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * c api option order is row-major order
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 */
CTEST(zgeadd, c_api_matrix_n_100_m_100_row_major)
{
    CBLAS_ORDER order = CblasRowMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {4.0, 1.5};
    double beta[] = {2.0, 1.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * c api option order is row-major order
 * For A number of rows is 50, number of colums is 100
 * For C number of rows is 50, number of colums is 100
 */
CTEST(zgeadd, c_api_matrix_n_50_m_100_row_major)
{
    CBLAS_ORDER order = CblasRowMajor;

    blasint n = N / 2;
    blasint m = M;

    blasint lda = n;
    blasint ldc = n;

    double alpha[] = {3.0, 2.5};
    double beta[] = {1.0, 2.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * c api option order is column-major order
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 * Scalar alpha is zero (operation is C:=beta*C)
 */
CTEST(zgeadd, c_api_matrix_n_100_m_100_alpha_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {0.0, 0.0};
    double beta[] = {1.0, 1.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * c api option order is column-major order
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 * Scalar beta is zero (operation is C:=alpha*A)
 */
CTEST(zgeadd, c_api_matrix_n_100_m_100_beta_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {3.0, 1.5};
    double beta[] = {0.0, 0.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * c api option order is column-major order
 * For A number of rows is 100, number of colums is 100
 * For C number of rows is 100, number of colums is 100
 * Scalars alpha, beta is zero (operation is C:= 0)
 */
CTEST(zgeadd, c_api_matrix_n_100_m_100_alpha_beta_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {0.0, 0.0};
    double beta[] = {0.0, 0.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test zgeadd by comparing it against reference
 * with the following options:
 *
 * For A number of rows is 50, number of colums is 100
 * For C number of rows is 50, number of colums is 100
 */
CTEST(zgeadd, c_api_matrix_n_100_m_50)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = N;
    blasint m = M / 2;

    blasint lda = m;
    blasint ldc = m;

    double alpha[] = {2.0, 3.0};
    double beta[] = {2.0, 4.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Test error function for an invalid param order -
 * specifies whether A and C stored in
 * row-major order or column-major order
 */
CTEST(zgeadd, c_api_xerbla_invalid_order)
{
    CBLAS_ORDER order = INVALID;

    blasint n = 1;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = 1;

    int expected_info = 0;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param n -
 * number of columns of A and C.
 * Must be at least zero.
 *
 * c api option order is column-major order
 */
CTEST(zgeadd, c_api_xerbla_n_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = INVALID;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = 1;

    int expected_info = 2;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param n -
 * number of columns of A and C.
 * Must be at least zero.
 *
 * c api option order is row-major order
 */
CTEST(zgeadd, c_api_xerbla_n_invalid_row_major)
{
    CBLAS_ORDER order = CblasRowMajor;

    blasint n = INVALID;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = 1;

    int expected_info = 1;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param m -
 * number of rows of A and C
 * Must be at least zero.
 *
 * c api option order is column-major order
 */
CTEST(zgeadd, c_api_xerbla_m_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = INVALID;

    blasint lda = 1;
    blasint ldc = 1;

    int expected_info = 1;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param m -
 * number of rows of A and C
 * Must be at least zero.
 *
 * c api option order is row-major order
 */
CTEST(zgeadd, c_api_xerbla_m_invalid_row_major)
{
    CBLAS_ORDER order = CblasRowMajor;

    blasint n = 1;
    blasint m = INVALID;

    blasint lda = 1;
    blasint ldc = 1;

    int expected_info = 2;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param lda -
 * specifies the leading dimension of A. Must be at least MAX(1, m).
 *
 * c api option order is column-major order
 */
CTEST(zgeadd, c_api_xerbla_lda_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = 1;

    blasint lda = INVALID;
    blasint ldc = 1;

    int expected_info = 5;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param lda -
 * specifies the leading dimension of A. Must be at least MAX(1, m).
 *
 * c api option order is row-major order
 */
CTEST(zgeadd, c_api_xerbla_lda_invalid_row_major)
{
    CBLAS_ORDER order = CblasRowMajor;

    blasint n = 1;
    blasint m = 1;

    blasint lda = INVALID;
    blasint ldc = 1;

    int expected_info = 5;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param ldc -
 * specifies the leading dimension of C. Must be at least MAX(1, m).
 *
 * c api option order is column-major order
 */
CTEST(zgeadd, c_api_xerbla_ldc_invalid)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = INVALID;

    int expected_info = 8;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Test error function for an invalid param ldc -
 * specifies the leading dimension of C. Must be at least MAX(1, m).
 *
 * c api option order is row-major order
 */
CTEST(zgeadd, c_api_xerbla_ldc_invalid_row_major)
{
    CBLAS_ORDER order = CblasRowMajor;

    blasint n = 1;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = INVALID;

    int expected_info = 8;

    int passed = check_badargs('C', order, m, n, lda, ldc, expected_info);
    ASSERT_EQUAL(TRUE, passed);
}

/**
 * C API specific test
 * Check if n - number of columns of A, C equal zero.
 *
 * c api option order is column-major order
 */
CTEST(zgeadd, c_api_n_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 0;
    blasint m = 1;

    blasint lda = 1;
    blasint ldc = 1;

    double alpha[] = {1.0, 1.0};
    double beta[] = {1.0, 1.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}

/**
 * C API specific test
 * Check if m - number of rows of A and C equal zero.
 *
 * c api option order is column-major order
 */
CTEST(zgeadd, c_api_m_zero)
{
    CBLAS_ORDER order = CblasColMajor;

    blasint n = 1;
    blasint m = 0;

    blasint lda = 1;
    blasint ldc = 1;

    double alpha[] = {1.0, 1.0};
    double beta[] = {1.0, 1.0};

    double norm = check_zgeadd('C', order, m, n, alpha,
                               lda, beta, ldc);

    ASSERT_DBL_NEAR_TOL(0.0, norm, DOUBLE_EPS);
}
#endif