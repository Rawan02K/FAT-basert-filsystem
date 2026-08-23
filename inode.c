#include "allocation.h"
#include "inode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>



/* The number of bytes in a block.
 * Do not change.
 */
#define BLOCKSIZE 4096


/* The lowest unused node ID.
 * Do not change.
 */
static int num_inode_ids = 0;

/* This helper function computes the number of blocks that you must allocate
 * on the simulated disk for a give file system in bytes. You don't have to use
 * it.
 * Do not change.
 */
static int blocks_needed( int bytes )
{
    int blocks = bytes / BLOCKSIZE;
    if( bytes % BLOCKSIZE != 0 )
        blocks += 1;
    return blocks;
}

/* This helper function returns a new integer value when you create a new inode.
 * This helps you do avoid inode reuse before 2^32 inodes have been created. It
 * keeps the lowest unused inode ID in the global variable num_inode_ids.
 * Make sure to update num_inode_ids when you have loaded a simulated disk.
 * Do not change.
 */
static int next_inode_id( )
{
    int retval = num_inode_ids;
    num_inode_ids += 1;
    return retval;
}

struct inode* create_file( struct inode* parent, char* name, int size_in_bytes )
{
    if(parent == NULL) {
        return NULL;
    }
    if(!parent->is_directory) {
        return NULL;
    }

    if(find_inode_by_name(parent, name)) {
        return NULL;
    }

    int total = parent->num_children + 1;
    parent->children = realloc(parent->children, total * sizeof(struct inode*));

    parent->children[total - 1] = malloc(sizeof(struct inode));
    parent->children[total-1]->id = next_inode_id();
    parent->children[total-1]->name = malloc(strlen(name) +1);
    strcpy(parent->children[total-1]->name, name);

    parent->children[total-1]->filesize = size_in_bytes;
    parent->children[total-1]->is_directory = 0;
    parent->children[total-1]->num_children = 0;
    parent->children[total-1]->children = malloc(sizeof(struct inode*));
   // parent->children[total-1]->children = NULL;
    parent->children[total-1]->num_blocks = blocks_needed(size_in_bytes);
    parent->children[total-1]->num_children = 0;

    parent->children[total-1]->blocks = malloc(sizeof(size_t) * parent->children[total-1]->num_blocks);

    for(int i = 0; i < parent->children[total-1]->num_blocks; i++) {

        parent->children[total-1]->blocks[i] = allocate_block();

        if ((int)parent->children[total - 1]->blocks[i] == -1) {
            fprintf(stderr, "Error with allocating memory for file block %d\n", i);
            for (int j = 0; j < i; j++) {
                free_block(parent->children[total - 1]->blocks[j]);
            }
            free(parent->children[total - 1]->name);
            free(parent->children[total - 1]->children);
            free(parent->children[total - 1]->blocks);
            free(parent->children[total - 1]);
            return NULL;
        }
    
    }
    parent->num_children++;
    

    return parent->children[total - 1];
}


struct inode* create_dir(struct inode* parent, char* name) {
    if (parent == NULL) {
        parent = malloc(sizeof(struct inode));
        parent->id = next_inode_id();
         parent->name = malloc(strlen(name) +1);
        strcpy(parent->name, name);
        parent->is_directory = 1;
        parent->num_children = 0;
        parent->children = malloc(sizeof(struct inode*) * parent->num_children);
        parent->filesize = 0;
        parent->num_blocks = 0;
        parent->blocks = malloc(sizeof(size_t) * parent->num_blocks);
        return parent;
    }

    if (!parent->is_directory) {
        return NULL;
    }

    if (find_inode_by_name(parent, name)) {
        return NULL;
    }

    int total = parent->num_children + 1;
    parent->children = realloc(parent->children, total * sizeof(struct inode*));

    if (parent->children == NULL) {
        fprintf(stderr, "Error with allocating space for new dir\n");
        return NULL;
    }

    parent->children[total - 1] = malloc(sizeof(struct inode));
    parent->children[total - 1]->id = next_inode_id();
    parent->children[total-1]->name = malloc(strlen(name) +1);
    strcpy(parent->children[total-1]->name, name);
    parent->children[total - 1]->is_directory = '1';
    parent->children[total - 1]->num_children = 0;
    parent->children[total - 1]->children = malloc(sizeof(struct inode*) * parent->children[total - 1]->num_children);
   // parent->children[total - 1]->children = NULL;
    parent->children[total - 1]->filesize = 0;
    parent->children[total - 1]->num_blocks = 0;
    parent->children[total - 1]->blocks = malloc(sizeof(size_t) * parent->children[total - 1]->num_blocks);
    //parent->children[total - 1]->blocks = NULL;

    parent->num_children++;

    return parent->children[total - 1];
}

struct inode* find_inode_by_name( struct inode* parent, char* name )
{
    if (!parent->is_directory) {
        return NULL; 
    }
    for (int i = 0; i < parent->num_children; i++) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            return parent->children[i];
        }
    }
    return NULL;

}

static int verified_delete_in_parent( struct inode* parent, struct inode* node ){
    for (int i = 0; i < parent -> num_children; i++){
        if (parent -> children[i] -> id == node -> id){
            return 0;
        }
    }
    return 1;
}

int is_node_in_parent( struct inode* parent, struct inode* node ){
    if (!parent || !node) {
        return 0;
    }
    for (int i = 0; i < parent-> num_children; i++) {
        if (parent->children[i] == node) {
            return 1;
        }
    }
    return 0;
}

int delete_file( struct inode* parent, struct inode* node ){

    if(parent == NULL || node == NULL) {
        return -1;
    }
    if (verified_delete_in_parent(parent, node) != 0) {
        return -1;
    }
    //release allocated blocks
    for (int i = 0; i < node->num_blocks; i++) {
        int release = free_block(node->blocks[i]);
        if (release == -1) {
            return -1;  
        }
    }
    free(node->blocks);
    // Remove the node from the parent's children array
    int index = -1;
    for (int i = 0; i < parent->num_children; i++) {
        if (parent->children[i] == node) {
            index = i;
            break;
        }
    }
    //shift elements, fill the gap
    if (index != -1) {
        for (int i = index; i < parent->num_children - 1; i++) {
        parent->children[i] = parent->children[i + 1];
    }
    parent->num_children--;

    free(node->name);

    if (node->is_directory) {
        free(node->children);
    }
    free(node);

    return 0;  
    }
return -1;  
}


int delete_dir( struct inode* parent, struct inode* node ){
    if (verified_delete_in_parent(parent, node) != 0 || node->num_children > 0) {
        return -1;
    }
    //delete all children of the directory
    for (int i = 0; i < node->num_children; i++) {
        int resultat = delete_dir(node, node->children[i]);
        if (resultat == -1) {
            return -1;  
        }
    }
    // Free allocated blocks using free_block function
    for (int i = 0; i < node->num_blocks; i++) {
        int result = free_block(node->blocks[i]);
        if (result == -1) {
            return -1;  
        }
    }
    // Remove the node from the parent's children array
    int index = -1;
    for (int i = 0; i < parent->num_children; i++) {
        if (parent->children[i] == node) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        // replace the index with the last element
        if (index != parent->num_children - 1) {
            parent->children[index] = parent->children[parent->num_children - 1];
        }
        parent->num_children--;

        free(node->name);

        free(node->children);

        free(node->blocks);

        free(node);

        return 0;  
    }
    return -1;  
}


static struct inode** load_inode(FILE* file, struct inode *root) {
    int rc;

    //struct inode** children = malloc(root->num_children * sizeof(struct inode*));

    if (!root->children) {
        fprintf(stderr, "Failed to allocate memory for children\n");
        return NULL;
    }

    for (int i = 0; i < root->num_children; i++) {
        root->children[i] = malloc(sizeof(struct inode));

        if (!root->children[i]) {
            fprintf(stderr, "Failed to allocate memory for child %d\n", i);
            free(root->children);
            return NULL;
        }

        rc = fread(&root->children[i]->id, sizeof(int), 1, file);
        if (rc != 1) {
            fprintf(stderr, "Failed to read id for child %d\n", i);
            free(root->children);
            return NULL;
        }

        int name_length;
        rc = fread(&name_length, sizeof(int), 1, file);
        if (rc != 1) {
            fprintf(stderr, "Failed to read name_length\n");
            return NULL;
        }


        root->children[i]->name = malloc(name_length);
        if(root->children[i]->name == NULL) {
            fprintf(stderr, "Failed to allocate memory for name\n");
            free(root->children);
            return NULL;
        }

        rc = fread(root->children[i]->name, sizeof(char), name_length, file);
        if(rc != name_length) {
            fprintf(stderr, "Issue with reading name\n ");
            free(root->children[i]->name);
            free(root->children);
            return NULL;
        }

        char dir_flag;
        rc = fread(&dir_flag, sizeof(char), 1, file);
        if(rc != 1) {
            fprintf(stderr, "issue with reading dir_flag\n");
            free(root->children[i]->name);
            free(root->children);
            return NULL;
        }
        root->children[i]->is_directory = dir_flag;

        if (root->children[i]->is_directory) {
            int ant_children;
            rc = fread(&ant_children, sizeof(int), 1, file);

            if (rc != 1) {
                fprintf(stderr, "Failed to read num_children\n");
                return NULL;
            }
            root->children[i]->num_children = ant_children;

            root->children[i]->filesize = 0;
            root->children[i]->num_blocks = 0;
            root->children[i]->blocks = NULL;

            root->children[i]->children = malloc(sizeof(struct inode*) * root->children[i]->num_children);

            for(int j = 0; j< root->children[i]->num_children; j++) {
                root->children[i]->children[j] = malloc(sizeof(struct inode));  //here
                size_t id;
                fread(&id,sizeof(size_t), 1, file);
                root->children[i]->children[j]->id = (int)id;

                free(root->children[i]->children[j]);

            // fprintf(stderr, "root->children[%d] = %d\n", i, root->children[i]->id);
            }

            
            root->children[i]->children = load_inode(file, root->children[i]);  //


        } 

        else {
            root->children[i]->num_children = 0;
            root->children[i]->children = NULL;

            
            int fsize;
            rc = fread(&fsize, sizeof(int), 1, file);

            if (rc != 1) {
                fprintf(stderr, "Failed to read filesize\n");
                return NULL;
            }
            root->children[i]->filesize = fsize;

            int ant_blocks;
            rc = fread(&ant_blocks, sizeof(int), 1, file);

            if (rc != 1) {
                fprintf(stderr, "Failed to read ant_blocks\n");
                return NULL;
            }
            root->children[i]->num_blocks = ant_blocks;


            root->children[i]->blocks = malloc(root->children[i]->num_blocks * sizeof(size_t*));   //here

            for (int k = 0; k < root->children[i]->num_blocks; k++) {
                rc = fread(&root->children[i]->blocks[k], sizeof(size_t), 1, file);
                if (rc != 1) {
                    fprintf(stderr, "issue with reading blocks\n");
                    free(root->children[i]->name);
                    free(root->children[i]->blocks);
                    free(root->children[i]);
                    return NULL;
                }
            }
        }
    }

    return root->children;
}
//jeg har problemer med å lese inn data etter id. Namelength har en stor verdi noe som sikkert ødelegger for resten av verdiene


struct inode* load_inodes(char* master_file_table) {
    FILE* file = fopen(master_file_table, "rb");

    int rc;

    if (!file) {
        fprintf(stderr, "Failed to open file %s\n", master_file_table);
        return NULL;
    }

    struct inode* root = malloc(sizeof(struct inode));

    int id;
    rc = fread(&id, sizeof(int), 1, file);

    if (rc != 1) {
        fprintf(stderr, "Failed to read root->id\n");
        free(root);
        return NULL;
    }
    root->id = id;

    int name_length;
    rc = fread(&name_length, sizeof(int), 1, file);
    if (rc != 1) {
        free(root);
        fprintf(stderr, "Failed to read name_length\n");
        return NULL;
    }

    root->name = malloc(name_length);
    if(root->name == NULL) {
        fprintf(stderr, "Failed to allocate memory for name\n");
        free(root);
        return NULL;
    }

    rc = fread(root->name, sizeof(char), name_length, file);
    if(rc != name_length) {
        fprintf(stderr, "Issue with reading root->name\n");
        free(root->name);
        free(root);
        return NULL;
    }

    char dir_flag;
    rc = fread(&dir_flag, sizeof(char), 1, file);
    if(rc != 1) {
        fprintf(stderr, "issue with reading dir_flag\n");
        free(root->name);
        free(root);
        return NULL;
    }
    root->is_directory = dir_flag;


    int ant_children;
    rc = fread(&ant_children, sizeof(int), 1, file);

    if (rc != 1) {
        fprintf(stderr, "Failed to read num_children\n");
        free(root->name);
        free(root);
        return NULL;
    }
    root->num_children = ant_children;

    root->filesize = 0;
    root->num_blocks = 0;
    root->blocks = NULL;

    root->children = malloc(sizeof(struct inode*) * root->num_children);

    if (root->children == NULL) {
        fprintf(stderr, "Failed to allocate memory for children\n");
        free(root->name);
        free(root);
        return NULL;
    }

    for(int i = 0; i< root->num_children; i++) {
        root->children[i] = malloc(sizeof(struct inode)); //here
        if (root->children[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for child %d\n", i);
            // Free the previously allocated memory for children
            for (int j = 0; j < i; j++) {
                free(root->children[j]);
            }
            free(root->children);
            free(root->name);
            free(root);
            return NULL;
        }
        size_t id;
        fread(&id,sizeof(size_t), 1, file);
        root->children[i]->id = (int)id;

        free(root->children[i]);

       //fprintf(stderr, "root->children[%d] = %d\n", i, root->children[i]->id);
    }

     
    root->children = load_inode(file, root); //here

    
    //save_inodes(master_file_table, root);

    fclose(file);

    return root;
}


/* The function save_inode is a recursive functions that is
 * called by save_inodes to store a single inode on disk,
 * and call itself recursively for every child if the node
 * itself is a directory.
 */
static void save_inode( FILE* file, struct inode* node )
{
    if( !node ) return;

    int len = strlen( node->name ) + 1;

    fwrite( &node->id, 1, sizeof(int), file );
    fwrite( &len, 1, sizeof(int), file );
    fwrite( node->name, 1, len, file );
    fwrite( &node->is_directory, 1, sizeof(char), file );
    if( node->is_directory )
    {
        fwrite( &node->num_children, 1, sizeof(int), file );
        for( int i=0; i<node->num_children; i++ )
        {
            struct inode* child = node->children[i];
            size_t id = child->id;
            fwrite( &id, 1, sizeof(size_t), file );
        }

        for( int i=0; i<node->num_children; i++ )
        {
            struct inode* child = node->children[i];
            save_inode( file, child );
        }
    }
    else
    {
        fwrite( &node->filesize, 1, sizeof(int), file );
        fwrite( &node->num_blocks, 1, sizeof(int), file );
        for( int i=0; i<node->num_blocks; i++ )
        {
            fwrite( &node->blocks[i], 1, sizeof(size_t), file );
        }
    }
}

void save_inodes( char* master_file_table, struct inode* root )
{
    if( root == NULL )
    {
        fprintf( stderr, "root inode is NULL\n" );
        return;
    }

    FILE* file = fopen( master_file_table, "w" );
    if( !file )
    {
        fprintf( stderr, "Failed to open file %s\n", master_file_table );
        return;
    }
    

    save_inode( file, root );

    fclose( file );
}

/* This static variable is used to change the indentation while debug_fs
 * is walking through the tree of inodes and prints information.
 */
static int indent = 0;

/* Do not change.
 */
void debug_fs( struct inode* node )
{
    if( node == NULL ) return;
    for( int i=0; i<indent; i++ )
        printf("  ");

    if( node->is_directory )
    {
        printf("%s (id %d)\n", node->name, node->id );
        indent++;
        for( int i=0; i<node->num_children; i++ )
        {
            struct inode* child = (struct inode*)node->children[i];
            debug_fs( child );
        }
        indent--;
    }
    else
    {
        printf("%s (id %d size %db blocks ", node->name, node->id, node->filesize );
        for( int i=0; i<node->num_blocks; i++ )
        {
            printf("%d ", (int)node->blocks[i]);
        }
        printf(")\n");
    }
}

/* Do not change.
 */
void fs_shutdown( struct inode* inode )
{
    if( !inode ) return;

    //fprintf(stderr, "%s shudtown\n", inode->name);

    if( inode->is_directory )
    {
       // fprintf(stderr, "%s shudtown\n", inode->name);
        for( int i=0; i<inode->num_children; i++ )
        {
            fs_shutdown( inode->children[i] );
        }
    }

    if( inode->name )  free( inode->name );
    if( inode->children ) free( inode->children );
    if( inode->blocks )  free( inode->blocks );
    free( inode );
}

