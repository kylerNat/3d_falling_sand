mkdir "../build"
pushd "../build"

rm "spellcrafting.exe"
clang -g -o "spellcrafting.exe" -D DEBUG ../code/"win32_gl_main.cpp"

./"spellcrafting.exe"
popd
