# **Step :** <br />
cmake -S . -B build -G Ninja -DWOUOUI_SKETCH=../src/main.ino <br />
export PATH=/mingw64/bin:$PATH <br />
cd build && ninja <br />
./Emulator.exe <br />


# **Remove build :** <br />
rm -rf build
