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
