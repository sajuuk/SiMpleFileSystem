insmod SiMpleFileSystem.ko
./mkfs test.img
mount -o loop -t SMFS test.img test
