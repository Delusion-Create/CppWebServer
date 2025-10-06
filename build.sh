# 自动编译项目的脚本

mkdir -p build
cd build
rm -r ./*
cmake ..
make -j8