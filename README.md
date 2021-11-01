<!--
 * @Author: Corvo Attano(fkxzz001@qq.com)
 * @Description: 
 * @LastEditors: Corvo Attano(fkxzz001@qq.com)
-->
# SiMpleFileSystem
## Intro
whole project has 2 parts, the implement of SiMpleFileSystem and a disk formater.  
SiMpleFileSystem is a file system based on POSIX standard, can be installed as a module in Linux.  
disk formater can format a disk image, which can be managed by SiMpleFileSystem.  
### Current features
* Dir: create,remove,list
* file: create,read,write
### How to use
* install the linux headers
```shell
sudo apt-get install linux-headers-$(uname -r)
```
* create a directory and a disk image file
```shell
mkdir -p test
dd if=/dev/zero of=test.img bs=1M count=10
```
* build project
```shell
make
```
* install module, format disk image and mount file system
```shell
sudo ./setall.sh
```
* if you want to unmount file system and remove module installed
```shell
sudo ./undoall.sh
```
## Data Structure  
| section | size(KiB) |
| ----|----|
|super block | 4 |
|inodes | 4n |
|inodes bitmap | 4m |
|blocks bitmap | 4l|
|data blocks | ?|
```
The size of each blocks is 4KiB  
1st block is super block  
n blocks store the inodes  
m blocks is inodes bitmap  
l blocks is block bitmap  
rest of blocks are data blocks  
```
the data structure of super node shows below  
| section | size(B) |
| ----|----|
|Magic | 4 |
|Blocknum | 4 |
|inodenum | 4 |
|inodestoreblocks | 4 |
|inodebitmapblocks | 4 |
|databitmapblocks | 4 |
|inodefree | 4 |
|datafree | 4|

below is the layout of each inode
|inode member | size(B) |
| ---- | ---- |
|mode    | 4|  
|uid     | 4|  
|gid     |4 |
|size    |4 |
|ctime   |4 |
|atime   |4 |
|mtime   |4 |
|blocks  |4 |
|block list/directory pointer| 96  |
|total |128 |

which means the max size of a file is 96B/4B*4KiB=96KiB.  
If the inode store a directory, 

## Disk Formation  
Assume that the blocks in dist is 'B' the number of inode store blocks in a disk is 'x', which means there are x*128 inodes at most. Then we have this equation below:  
```
1+x+ceil(x*32/8/4096)+ceil(x*32/8/4096)+32*x=B

1 superblock
x inodeblocks 
ceil(x*32/8/4096) inode/data bitmap blocks
32x datablocks
```
then we can have `x=floor((B-1)K/(K+64+32K))`, where `K=8*4096=2^15`  
From the equation above, we can design the algorithm of disk formation.  
```
start
    open disk image
    get disk size
    if disk size < min size
        report error
    fi
    alloc super block
    alloc inode block
    alloc pitmaps
    write the data block 0
end
```

## Super Block Operations
* put_super
release the super block in memory
* alloc_inode
alloc the inode's memory, return the vfs inode
* destroy_inode
release the inode memory
* write_inode
copy the inode data from memory to buffer, and call cache synchronization function
* sync_fs
copy the super block and bitmaps data from memory to buffer, and call cache synchronization function
* statfs
return the file system's information, including magic number, block size,max file name length,etc
## Inode Operations
* lookup
find the dir or file by name, if found append to dentry
```
begin
    check the input name length
    read current directory's data block from page cache
    foreach members in directory 
    do
        if member's name is what we find
            get its inode
            append inode to dentry
        fi
    od
end
```
* create
create dir or file
```
begin
    check name length
    read parent dir data block
    check is parent dir's data block has enough space
    create a new inode and initialize
    if want to create a dir
        alloc new data block to the dir
        clean the data block
    fi
    add the new dentry to parent dir data block
    mark the parent inode, new inode, parent dir's data block dirty
end
```
* mkdir
create a new dir
```
begin
    call create
end
```
* rmdir
```
begin
    get the dir's data block
    check is the dir empty
    get the parent dir's data block
    find the dir's location in its parent data block
    delete this dentry and rearrange data block
    mark this dir's inode, its parent inode, and data block dirty
    set the bitmap unuesd
end
```
## Bitmap Operations
* _setBitmap
```
begin
    set byte mask
    targetByte = targetByte &(|) bytemask
end
```
* _findFirstfree
```
begin
    foreach byte in bitmap do
        if byte!=0
            mask=1
            while mask & byte == 0 do
                mask=mask<<1
            od
            return this byte's offset
        fi
    od
end      
```
## Dir Operations



