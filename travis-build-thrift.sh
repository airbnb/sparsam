cd /tmp 
tar xvf thrift.tar.gz 
cd thrift-0.11.0
cmake . -DBUILD_COMPILER=OFF -DBUILD_C_GLIB=OFF -DBUILD_PYTHON=OFF -DBUILD_TESTING=OFF -DBUILD_JAVA=OFF 
make -j && sudo make install
sudo ldconfig
