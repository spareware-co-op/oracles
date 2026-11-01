#!/bin/sh
# Fetches NLib at its pinned commit, cuts out the routines decision 32.23 bars, and compiles the rest
# with the harness. Only the `NL_*` sources are fetched: SMLib's own `Sm*` classes, whose matrix,
# integrator and Brent routines are barred too, never reach this image. No part of solidcore is
# needed, which keeps its port of `NL_Math.cpp` (barred by 29.6) out as well.
set -eu

commit=c9c5979f04cea8c38213004d4f04df0dcfb40d05
# NL_Math.cpp after the cut below; a commit with different text cannot compile by accident.
cut=d0d8cd2bef1c6db5de988ffff9df0b6fa4fa566af167b130c9c71e3179bc2655

cd /oracle
git init -q usd-brep
git -C usd-brep fetch -q --depth 1 https://github.com/NVIDIA-Omniverse/usd-brep "$commit"
git -C usd-brep checkout -q FETCH_HEAD -- source/SMLib/inc 'source/SMLib/src/NL_*.cpp'
src=usd-brep/source/SMLib/src
inc=usd-brep/source/SMLib/inc

# Each routine runs from its first line to the closing brace at the left margin. `ST_pythag` is
# matched by its parameter's name, so its prototype stays and only its definition goes.
sed -i \
    -e '/^NL_FLAG N_SolveQuadraticEq(/,/^}/d' \
    -e '/^NL_FLAG N_SingleValueDecomposeSolve(/,/^}/d' \
    -e '/^NL_FLAG N_RealMatrixSingleValueDecompose(/,/^}/d' \
    -e '/^NL_VOID N_FuncFindMinima(/,/^}/d' \
    -e '/^NL_FLAG N_RealMatrixEigenValuesVectors(/,/^}/d' \
    -e '/^NL_REAL ST_pythag( NL_REAL a/,/^}/d' \
    -e '/^NL_FLAG M_TridiagonalSystem(/,/^}/d' \
    -e '/^NL_FLAG M_CorneredTridiagonalSystem(/,/^}/d' \
    "$src/NL_Math.cpp"
echo "$cut  $src/NL_Math.cpp" | sha256sum -c -

# The reference build's flags, each file's functions in their own section: the fitting files
# compile but name `Sm*` classes that are not here, so the link keeps only what the harness reaches.
flags="-std=c++17 -O2 -DNDEBUG -DNOMINMAX -DSM_GRAPHICS_CALLBACKS=1 -D_GLIBCXX_USE_CXX11_ABI=1
       -ffp-contract=off -fno-fast-math -fexceptions -frtti -w -ffunction-sections -fdata-sections
       -I$inc -I/oracle"
export flags
mkdir objects
ls "$src"/NL_*.cpp harness.cpp stubs.cpp |
    xargs -P "$(nproc)" -I{} sh -c 'g++ $flags -c "$1" -o "objects/$(basename "$1").o"' _ {}
g++ -Wl,--gc-sections objects/*.o -o harness

# Nothing from the source tree stays beside the program.
rm -rf usd-brep objects
