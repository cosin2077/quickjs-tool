wget https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz -O quickjs.tar.xz
mkdir quickjs
tar -xvf quickjs.tar.xz -C quickjs
cd quickjs
mv quickjs-2024-01-13/* .
make
cd ..
