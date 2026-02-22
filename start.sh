#!/bin/bash
cd "$(dirname "$0")"

git pull || true

conan install . --output-folder=build --build=missing
conan build .

exec ./build/Release/kiddo
