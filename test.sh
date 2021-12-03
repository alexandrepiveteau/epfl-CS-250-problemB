#!/bin/bash

# Clean the existing build folder
rm -r ./build

# Build the latest program
mkdir -p build
cmake -S. -B./build
cmake --build ./build

echo "--- RUNNING TESTS ... ---"
diff -u ./data/01.a <(cat ./data/01 | ./build/ex2)
diff -u ./data/02.a <(cat ./data/02 | ./build/ex2)
diff -u ./data/03.a <(cat ./data/03 | ./build/ex2)
diff -u ./data/04.a <(cat ./data/04 | ./build/ex2)
diff -u ./data/05.a <(cat ./data/05 | ./build/ex2)
echo "--- DONE ! ---"
