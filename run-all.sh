#!/usr/bin/env bash

ASM_DIR=asm/cmake-build-release
mkdir -p $ASM_DIR
pushd $ASM_DIR
cmake -DCMAKE_BUILD_TYPE=Release ..
make
popd

C_DIR=c/cmake-build-release
mkdir -p $C_DIR
pushd $C_DIR
cmake -DCMAKE_BUILD_TYPE=Release ..
make
popd

RUST_DIR=rust
pushd $RUST_DIR
cargo build --release
popd

CSHARP_DIR=csharp
pushd $CSHARP_DIR
xbuild /p:Configuration=Release
popd

echo
echo ===== Assembly =====
time $ASM_DIR/knapsack-asm

echo
echo ===== C =====
time $C_DIR/knapsack-c

echo
echo ===== Rust =====
time $RUST_DIR/target/release/knapsack-rust

echo
echo ===== C# =====
time mono $CSHARP_DIR/bin/Release/knapsack-csharp.exe

