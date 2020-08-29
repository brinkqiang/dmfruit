
rmdir /S /Q __build
mkdir __build
pushd __build
cmake -A x64 -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . --config RelWithDebInfo
popd

rem pause