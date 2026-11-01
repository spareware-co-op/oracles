// SPDX-License-Identifier: Apache-2.0
//
// What stands in for the NLib routines the kernel's architecture bars (29.6, decision 32.23):
// derived from Numerical Recipes, they are cut out of the source before it compiles (build.sh).
// Evaluation, knot operations and degree change call none of them, so reaching one stops the run,
// which leaves its job not covered and never passed. `ST_pythag` has no stand-in: only
// `N_RealMatrixEigenValuesVectors` called it.

#include <cstdio>
#include <cstdlib>

#include <nurbs.h>

namespace {

[[noreturn]] void barred(const char* routine) {
    std::fprintf(stderr, "not covered: a job reached %s, which decision 32.23 bars\n", routine);
    std::_Exit(3);
}

}  // namespace

NL_FLAG N_SolveQuadraticEq(NL_REAL, NL_REAL, NL_REAL, NL_REAL*, NL_REAL*, NL_INTEGER*) {
    barred("N_SolveQuadraticEq");
}

NL_FLAG N_SingleValueDecomposeSolve(NL_REAL**, NL_REAL*, NL_REAL**, NL_INDEX, NL_INDEX, NL_VOID*, NL_FLAG, NL_FLAG,
                                    NL_VOID*) {
    barred("N_SingleValueDecomposeSolve");
}

NL_FLAG N_RealMatrixSingleValueDecompose(NL_REAL**, NL_REAL*, NL_REAL**, NL_INDEX, NL_INDEX) {
    barred("N_RealMatrixSingleValueDecompose");
}

NL_VOID N_FuncFindMinima(NL_REAL, NL_REAL, NL_REAL, NL_REAL, NL_REAL (*)(NL_REAL), NL_REAL, NL_REAL, NL_REAL*,
                         NL_REAL*) {
    barred("N_FuncFindMinima");
}

NL_FLAG N_RealMatrixEigenValuesVectors(NL_RMATRIX*, NL_REAL*, NL_RMATRIX*, NL_STACKS*) {
    barred("N_RealMatrixEigenValuesVectors");
}

NL_FLAG M_TridiagonalSystem(NL_REAL*, NL_REAL*, NL_REAL*, NL_POINT*, NL_INDEX, NL_POINT*) {
    barred("M_TridiagonalSystem");
}

NL_FLAG M_CorneredTridiagonalSystem(NL_REAL*, NL_REAL*, NL_REAL*, NL_REAL, NL_REAL, NL_POINT*, NL_INDEX, NL_POINT*) {
    barred("M_CorneredTridiagonalSystem");
}
