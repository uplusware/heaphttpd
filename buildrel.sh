cd src
make clean
make
cd ..
chmod a+x ./release.sh
./release.sh 0.3 beta ubuntu
sudo ./ubuntu-niuhttpd-bin-beta-x86_64-linux/install.sh
sudo /etc/init.d/niuhttpd restart
