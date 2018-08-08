#!/bin/sh

if [[ $DEBUG = true ]]
then
    echo "DEBUG MODE"
    cargo build --manifest-path $1/Cargo.toml && cp $1/target/debug/libgepub_internals.a $2
else
    echo "RELEASE MODE"
    cargo build --manifest-path $1/Cargo.toml --release && cp $1/target/release/libgepub_internals.a $2.a
fi
