Design:

1)       You read the master file table bytes for bytes 
            depending on the data you need, the number of bytes will vary. You allocate memory for 
            the inode using malloc to allocate memory on the heap. You start with reading the root inode. Afterwards you recursively read the children inodes for each directory under root.
            Make sure to check for potential errors and free up memory when that occurs
    
    
2)  All the requirements where met
       
3) 
     -Verified_delete_in_parent():
        we used the function in delete_file() to check if the given node is a child of a given parent

    -find_inode_by_name():
        takes a parent inode and a name as input and searches through the children of the parent to find the inode with the specified name.

    -loade_inode():
        takes in a parent node and reads through its children.
        the functions runs recursively through all the children for directories

4) No tests failed
