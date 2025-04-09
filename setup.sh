rm -rf build
rm -rf builddir
meson setup build
meson setup builddir
ninja -C builddir
