/* 
 * Copyright (c) 2011, Alex Krizhevsky (akrizhevsky@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MATRIX_H_
#define MATRIX_H_

#include <matrix_funcs.h>
#ifdef NUMPY_INTERFACE
#include <Python.h>
#include <arrayobject.h>
#endif
#include <limits>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(_WIN64) || defined(_WIN32)
#include <float.h>
#define isnan(_X) (_isnan(_X))
#define isinf(_X) (!_finite(_X)) 
#define uint unsigned int
double sqrt(int _X);
double log(int _X);
#endif

#ifdef USE_MKL
#include <mkl.h>
#include <mkl_cblas.h>
#include <mkl_vsl.h>
#include <mkl_vml.h>

#define IS_MKL true

#ifdef DOUBLE_PRECISION
#define MKL_UNIFORM vdRngUniform
#define MKL_NORMAL vdRngGaussian
#define MKL_UNIFORM_RND_METHOD VSL_METHOD_DUNIFORM_STD_ACCURATE
#define MKL_GAUSSIAN_RND_METHOD VSL_METHOD_DGAUSSIAN_BOXMULLER
#define MKL_EXP vdExp
#define MKL_RECIP vdInv
#define MKL_SQUARE vdSqr
#define MKL_TANH vdTanh
#define MKL_LOG vdLn
#define MKL_VECMUL vdMul
#define MKL_VECDIV vdDiv
#else
#define MKL_UNIFORM vsRngUniform
#define MKL_NORMAL vsRngGaussian
#define MKL_UNIFORM_RND_METHOD VSL_METHOD_SUNIFORM_STD_ACCURATE
#define MKL_GAUSSIAN_RND_METHOD VSL_METHOD_SGAUSSIAN_BOXMULLER
#define MKL_EXP vsExp
#define MKL_RECIP vsInv
#define MKL_SQUARE vsSqr
#define MKL_TANH vsTanh
#define MKL_LOG vsLn
#define MKL_VECMUL vsMul
#define MKL_VECDIV vsDiv
#endif /* DOUBLE_PRECISION */

#else
extern "C" {
#include <cblas.h>
}
#define IS_MKL false
#endif /* USE_MKL */

#ifdef DOUBLE_PRECISION
#define CBLAS_GEMM cblas_dgemm
#define CBLAS_SCAL cblas_dscal
#define CBLAS_AXPY cblas_daxpy
#else
#define CBLAS_GEMM cblas_sgemm
#define CBLAS_SCAL cblas_sscal
#define CBLAS_AXPY cblas_saxpy
#endif /* DOUBLE_PRECISION */

#define MTYPE_MAX numeric_limits<MTYPE>::max()

class Matrix {
private:
    MTYPE* _data;
    bool _ownsData;
    long long _numRows, _numCols;
    long long _numElements;
    CBLAS_TRANSPOSE _trans;

    void _init(MTYPE* data, long long numRows, long long numCols, bool transpose, bool ownsData);
    void _tileTo2(Matrix& target) const;
    void _copyAllTo(Matrix& target) const;
    MTYPE _sum_column(long long col) const;
    MTYPE _sum_row(long long row) const;
    MTYPE _aggregate(MTYPE(*agg_func)(MTYPE, MTYPE), MTYPE initialValue) const;
    void _aggregate(long long axis, Matrix& target, MTYPE(*agg_func)(MTYPE, MTYPE), MTYPE initialValue) const;
    MTYPE _aggregateRow(long long row, MTYPE(*agg_func)(MTYPE, MTYPE), MTYPE initialValue) const;
    MTYPE _aggregateCol(long long row, MTYPE(*agg_func)(MTYPE, MTYPE), MTYPE initialValue) const;
    void _updateDims(long long numRows, long long numCols);
    void _applyLoop(MTYPE(*func)(MTYPE));
    void _applyLoop(MTYPE (*func)(MTYPE), Matrix& target);
    void _applyLoop2(const Matrix& a, MTYPE(*func)(MTYPE, MTYPE), Matrix& target) const;
    void _applyLoop2(const Matrix& a, MTYPE (*func)(MTYPE,MTYPE, MTYPE), MTYPE scalar, Matrix& target) const;
    void _applyLoopScalar(const MTYPE scalar, MTYPE(*func)(MTYPE, MTYPE), Matrix& target) const;
    void _checkBounds(long long startRow, long long endRow, long long startCol, long long endCol) const;
    void _divideByVector(const Matrix& vec, Matrix& target);
    inline long long _getNumColsBackEnd() const {
        return _trans == CblasNoTrans ? _numCols : _numRows;
    }
public:
    enum FUNCTION {
        TANH, RECIPROCAL, SQUARE, ABS, EXP, LOG, ZERO, ONE, LOGISTIC1, LOGISTIC2, SIGN
    };
    Matrix();
    Matrix(long long numRows, long long numCols);
#ifdef NUMPY_INTERFACE
    Matrix(const PyArrayObject *src);
#endif
    Matrix(const Matrix &like);
    Matrix(MTYPE* data, long long numRows, long long numCols);
    Matrix(MTYPE* data, long long numRows, long long numCols, bool transpose);
    ~Matrix();

    inline MTYPE& getCell(long long i, long long j) const {
        assert(i >= 0 && i < _numRows);
        assert(j >= 0 && j < _numCols);
        if (_trans == CblasTrans) {
            return _data[j * _numRows + i];
        }
        return _data[i * _numCols + j];
    }

    MTYPE& operator()(long long i, long long j) const {
        return getCell(i, j);
    }

    inline MTYPE* getData() const {
        return _data;
    }

    inline bool isView() const {
        return !_ownsData;
    }

    inline long long getNumRows() const {
        return _numRows;
    }

    inline long long getNumCols() const {
        return _numCols;
    }

    inline long long getNumDataBytes() const {
        return _numElements * sizeof(MTYPE);
    }

    inline long long getNumElements() const {
        return _numElements;
    }

    inline long long getLeadingDim() const {
        return _trans == CblasTrans ? _numRows : _numCols;
    }

    inline long long getFollowingDim() const {
        return _trans == CblasTrans ? _numCols : _numRows;
    }

    inline CBLAS_TRANSPOSE getBLASTrans() const {
        return _trans;
    }

    inline bool isSameDims(const Matrix& a) const {
        return a.getNumRows() == getNumRows() && a.getNumCols() == getNumCols();
    }

    inline bool isTrans() const {
        return _trans == CblasTrans;
    }

    /*
     * Only use if you know what you're doing!
     * Does not update any dimensions. Just flips the _trans flag.
     *
     * Use transpose() if you want to get the transpose of this matrix.
     */
    inline void setTrans(bool trans) {
        assert(isTrans() == trans || !isView());
        _trans = trans ? CblasTrans : CblasNoTrans;
    }

    void apply(FUNCTION f);
    void apply(Matrix::FUNCTION f, Matrix& target);
    void subtractFromScalar(MTYPE scalar);
    void subtractFromScalar(MTYPE scalar, Matrix &target) const;
    void biggerThanScalar(MTYPE scalar);
    void smallerThanScalar(MTYPE scalar);
    void equalsScalar(MTYPE scalar);
    void biggerThanScalar(MTYPE scalar, Matrix& target) const;
    void smallerThanScalar(MTYPE scalar, Matrix& target) const;
    void equalsScalar(MTYPE scalar, Matrix& target) const;
    void biggerThan(Matrix& a);
    void biggerThan(Matrix& a, Matrix& target) const;
    void smallerThan(Matrix& a);
    void smallerThan(Matrix& a, Matrix& target) const;
    void minWith(Matrix &a);
    void minWith(Matrix &a, Matrix &target) const;
    void maxWith(Matrix &a);
    void maxWith(Matrix &a, Matrix &target) const;
    void equals(Matrix& a);
    void equals(Matrix& a, Matrix& target) const;
    void notEquals(Matrix& a) ;
    void notEquals(Matrix& a, Matrix& target) const;
    void add(const Matrix &m);
    void add(const Matrix &m, MTYPE scale);
    void add(const Matrix &m, Matrix& target);
    void add(const Matrix &m, MTYPE scale, Matrix& target);
    void subtract(const Matrix &m);
    void subtract(const Matrix &m, Matrix& target);
    void subtract(const Matrix &m, MTYPE scale);
    void subtract(const Matrix &m, MTYPE scale, Matrix& target);
    void addVector(const Matrix& vec, MTYPE scale);
    void addVector(const Matrix& vec, MTYPE scale, Matrix& target);
    void addVector(const Matrix& vec);
    void addVector(const Matrix& vec, Matrix& target);
    void addScalar(MTYPE scalar);
    void addScalar(MTYPE scalar, Matrix& target) const;
    void maxWithScalar(MTYPE scalar);
    void maxWithScalar(MTYPE scalar, Matrix &target) const;
    void minWithScalar(MTYPE scalar);
    void minWithScalar(MTYPE scalar, Matrix &target) const;
    void eltWiseMultByVector(const Matrix& vec);
    void eltWiseMultByVector(const Matrix& vec, Matrix& target);
    void eltWiseDivideByVector(const Matrix& vec);
    void eltWiseDivideByVector(const Matrix& vec, Matrix& target);
    void resize(long long newNumRows, long long newNumCols);
    void resize(const Matrix& like);
    Matrix& slice(long long startRow, long long endRow, long long startCol, long long endCol) const;
    void slice(long long startRow, long long endRow, long long startCol, long long endCol, Matrix &target) const;
    Matrix& sliceRows(long long startRow, long long endRow) const;
    void sliceRows(long long startRow, long long endRow, Matrix& target) const;
    Matrix& sliceCols(long long startCol, long long endCol) const;
    void sliceCols(long long startCol, long long endCol, Matrix& target) const;
    void rightMult(const Matrix &b, MTYPE scale);
    void rightMult(const Matrix &b, Matrix &target) const;
    void rightMult(const Matrix &b);
    void rightMult(const Matrix &b, MTYPE scaleAB, Matrix &target) const;
    void addProduct(const Matrix &a, const Matrix &b, MTYPE scaleAB, MTYPE scaleThis);
    void addProduct(const Matrix& a, const Matrix& b);
    void eltWiseMult(const Matrix& a);
    void eltWiseMult(const Matrix& a, Matrix& target) const;
    void eltWiseDivide(const Matrix& a);
    void eltWiseDivide(const Matrix& a, Matrix &target) const;
    Matrix& transpose() const;
    Matrix& transpose(bool hard) const;
    Matrix& tile(long long timesY, long long timesX) const;
    void tile(long long timesY, long long timesX, Matrix& target) const;
    void copy(Matrix &dest, long long srcStartRow, long long srcEndRow, long long srcStartCol, long long srcEndCol, long long destStartRow, long long destStartCol) const;
    Matrix& copy() const;
    void copy(Matrix& target) const;
    Matrix& sum(long long axis) const;
    void sum(long long axis, Matrix &target) const;
    MTYPE sum() const;
    MTYPE max() const;
    Matrix& max(long long axis) const;
    void max(long long axis, Matrix& target) const;
    MTYPE min() const;
    Matrix& min(long long axis) const;
    void min(long long axis, Matrix& target) const;
    MTYPE norm() const;
    MTYPE norm2() const;
    void scale(MTYPE scale);
    void scale(MTYPE alpha, Matrix& target);
    void reshape(long long numRows, long long numCols);
    Matrix& reshaped(long long numRows, long long numCols);
    void printShape(const char* name) const;
    bool hasNan() const;
    bool hasInf() const;
#ifdef USE_MKL
    void randomizeNormal(VSLStreamStatePtr stream, MTYPE mean, MTYPE stdev);
    void randomizeUniform(VSLStreamStatePtr stream);
    void randomizeNormal(VSLStreamStatePtr stream);
#else
    void randomizeNormal(MTYPE mean, MTYPE stdev);
    void randomizeUniform();
    void randomizeNormal();
#endif
    void print() const;
    void fprint(const char* filename) const;
    void print(long long startRow,long long rows, long long startCol,long long cols) const;
    void fprint(const char* filename, long long startRow,long long rows, long long startCol,long long cols) const;
    void print(long long rows, long long cols) const;
};

#endif /* MATRIX_H_ */
