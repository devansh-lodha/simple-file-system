#include "simplefs-ops.h"
#include <stdbool.h>

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
    if (!filename || strlen(filename) >= MAX_NAME_STRLEN) {
        return -1;
    }

    struct inode_t temp_inode;
    for (int i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, &temp_inode);
        if (temp_inode.status == INODE_IN_USE && strcmp(temp_inode.name, filename) == 0) {
            return -1; // File already exists
        }
    }

    int inodenum = simplefs_allocInode();
    if (inodenum == -1) {
        return -1; // No free inodes
    }

    struct inode_t new_inode;
    new_inode.status = INODE_IN_USE;
    strncpy(new_inode.name, filename, MAX_NAME_STRLEN);
    new_inode.file_size = 0;
    for (int i = 0; i < MAX_FILE_SIZE; i++) {
        new_inode.direct_blocks[i] = -1;
    }

    simplefs_writeInode(inodenum, &new_inode);
    return inodenum;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/
    if (!filename) {
        return;
    }

    int inodenum = -1;
    struct inode_t inode;

    for (int i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, &inode);
        if (inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0) {
            inodenum = i;
            break;
        }
    }

    if (inodenum == -1) {
        return; // File not found
    }

    for (int i = 0; i < MAX_FILE_SIZE; i++) {
        if (inode.direct_blocks[i] != -1) {
            simplefs_freeDataBlock(inode.direct_blocks[i]);
        }
    }

    simplefs_freeInode(inodenum);
}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
    if (!filename) {
        return -1;
    }

    int inodenum = -1;
    struct inode_t inode;
    for (int i = 0; i < NUM_INODES; i++) {
        simplefs_readInode(i, &inode);
        if (inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0) {
            inodenum = i;
            break;
        }
    }

    if (inodenum == -1) {
        return -1; // File not found
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_handle_array[i].inode_number == -1) {
            file_handle_array[i].inode_number = inodenum;
            file_handle_array[i].offset = 0;
            return i;
        }
    }

    return -1; // No free file handles
}

void simplefs_close(int file_handle){
    /*
	    close file pointed by `file_handle`
	*/
    if (file_handle < 0 || file_handle >= MAX_OPEN_FILES) {
        return;
    }
    file_handle_array[file_handle].inode_number = -1;
    file_handle_array[file_handle].offset = 0;
}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
    if (file_handle < 0 || file_handle >= MAX_OPEN_FILES || file_handle_array[file_handle].inode_number == -1 || !buf) {
        return -1;
    }

    if (nbytes == 0) {
        return 0;
    }

    struct filehandle_t *fh = &file_handle_array[file_handle];
    struct inode_t inode;
    simplefs_readInode(fh->inode_number, &inode);

    if (fh->offset + nbytes > inode.file_size) {
        return -1; // Read beyond end of file
    }

    int bytes_read = 0;
    int current_offset = fh->offset;
    
    while (bytes_read < nbytes) {
        int block_index = current_offset / BLOCKSIZE;
        int offset_in_block = current_offset % BLOCKSIZE;
        int data_block_num = inode.direct_blocks[block_index];

        char block_buf[BLOCKSIZE];
        simplefs_readDataBlock(data_block_num, block_buf);

        int bytes_to_read_from_block = BLOCKSIZE - offset_in_block;
        if (bytes_to_read_from_block > (nbytes - bytes_read)) {
            bytes_to_read_from_block = nbytes - bytes_read;
        }

        memcpy(buf + bytes_read, block_buf + offset_in_block, bytes_to_read_from_block);
        
        bytes_read += bytes_to_read_from_block;
        current_offset += bytes_to_read_from_block;
    }
    
    return 0;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/
    if (file_handle < 0 || file_handle >= MAX_OPEN_FILES || file_handle_array[file_handle].inode_number == -1 || !buf) {
        return -1;
    }

    if (nbytes == 0) {
        return 0;
    }

    struct filehandle_t *fh = &file_handle_array[file_handle];
    struct inode_t inode;
    simplefs_readInode(fh->inode_number, &inode);

    if (fh->offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE) {
        return -1; // Write exceeds maximum file size
    }
    
    int new_blocks_allocated[MAX_FILE_SIZE];
    int num_new_blocks = 0;

    int bytes_written = 0;
    int current_offset = fh->offset;

    while (bytes_written < nbytes) {
        int block_index = current_offset / BLOCKSIZE;
        int offset_in_block = current_offset % BLOCKSIZE;

        if (block_index >= MAX_FILE_SIZE) {
             for (int i = 0; i < num_new_blocks; i++) {
                simplefs_freeDataBlock(new_blocks_allocated[i]);
            }
            return -1;
        }

        int data_block_num = inode.direct_blocks[block_index];
        char block_buf[BLOCKSIZE];
        
        if (data_block_num == -1) {
            data_block_num = simplefs_allocDataBlock();
            if (data_block_num == -1) {
                // Transactional cleanup: free any blocks allocated during this failed write.
                for (int i = 0; i < num_new_blocks; i++) {
                    simplefs_freeDataBlock(new_blocks_allocated[i]);
                }
                return -1; // No free data blocks
            }
            inode.direct_blocks[block_index] = data_block_num;
            new_blocks_allocated[num_new_blocks++] = data_block_num;
            memset(block_buf, 0, BLOCKSIZE);
        } else {
            simplefs_readDataBlock(data_block_num, block_buf);
        }

        int bytes_to_write_in_block = BLOCKSIZE - offset_in_block;
        if (bytes_to_write_in_block > (nbytes - bytes_written)) {
            bytes_to_write_in_block = nbytes - bytes_written;
        }

        memcpy(block_buf + offset_in_block, buf + bytes_written, bytes_to_write_in_block);
        simplefs_writeDataBlock(data_block_num, block_buf);
        
        bytes_written += bytes_to_write_in_block;
        current_offset += bytes_to_write_in_block;
    }
    
    if (fh->offset + nbytes > inode.file_size) {
        inode.file_size = fh->offset + nbytes;
    }

    simplefs_writeInode(fh->inode_number, &inode);

    return 0;
}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/
    if (file_handle < 0 || file_handle >= MAX_OPEN_FILES || file_handle_array[file_handle].inode_number == -1) {
        return -1;
    }
    
    struct filehandle_t *fh = &file_handle_array[file_handle];
    struct inode_t inode;
    simplefs_readInode(fh->inode_number, &inode);

    int new_offset = fh->offset + nseek;

    if (new_offset < 0 || new_offset > inode.file_size) {
        return -1;
    }

    fh->offset = new_offset;
    return 0;
}