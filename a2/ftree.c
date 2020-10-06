#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// Add your system includes here.

#include "ftree.h"
struct TreeNode *generate_ftree_helper(const char *fname);
struct TreeNode *make_tree(const char *name, int permissions);
char *join_name_path(const char *path, const char *name);
void change_tree_type(struct stat stat_buf, struct TreeNode *tree);
/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname) {

    // Your implementation here.
    struct stat stat_buf;
    struct TreeNode *tree;
    if(lstat(fname, &stat_buf) == -1){
        fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
        return NULL;
    }
    tree = make_tree((char *)fname, stat_buf.st_mode & 0777);
    change_tree_type(stat_buf, tree);
    if(S_ISDIR(stat_buf.st_mode)){
        tree->contents = generate_ftree_helper(fname);
    }
    // Hint: consider implementing a recursive helper function that
    // takes fname and a path.  For the initial call on the 
    // helper function, the path would be "", since fname is the root
    // of the FTree.  For files at other depths, the path would be the
    // file path from the root to that file.
   // printf("%s %s\n", tree->fname, tree->contents->fname);
    return tree;
}

/*
 * Helper function for generate_ftree. Return a build linked list of ftree.
 */
struct TreeNode *generate_ftree_helper(const char *fname){
    struct dirent *entry_ptr;
    struct stat stat_buf;
    DIR *d_ptr = opendir(fname);
    struct TreeNode *next_tree = NULL, *root = NULL, *tree = NULL;

    if(d_ptr == NULL){
        perror("opendir");
        exit(1);
  
    }
    entry_ptr = readdir(d_ptr);
    while(entry_ptr != NULL){
        char *tmp = join_name_path(fname, "/");
	char *filepath = join_name_path(tmp, entry_ptr->d_name);
	free(tmp);
        if(lstat(filepath, &stat_buf) == -1){
            perror("lstat");
            exit(1);
        }
        if(entry_ptr->d_name[0] != '.'){
	    next_tree = make_tree(entry_ptr->d_name, stat_buf.st_mode & 0777);
	    change_tree_type(stat_buf, next_tree);
	    if (root == NULL){
		    root = tree = next_tree;
	    }
	    if(S_ISDIR(stat_buf.st_mode)){
                next_tree->contents = generate_ftree_helper(filepath);
            }
            tree->next = next_tree;
	    tree = next_tree;
	}
        entry_ptr = readdir(d_ptr);
	free(filepath);
    }
    if(root && next_tree == root){
   	root->next = NULL;
    }
    if(closedir(d_ptr) == -1){
    	perror("closedir");
	exit(1);
    }
    return root;
}


/*
 * Return a new TreeNode with pararmeter name and permission.
 */
struct TreeNode *make_tree(const char *name, int permissions){
    struct TreeNode *tree = malloc(sizeof(struct TreeNode));
    if(tree == NULL){
    	perror("malloc tree");
	exit(1);
    }
    tree->fname = malloc(sizeof(char) * (strlen(name) + 1));
    if(tree->fname == NULL){
    	perror("malloc fname");
	exit(1);
    }
    strcpy(tree->fname, name);
    tree->permissions = permissions;
    tree->next = NULL;
    tree->contents = NULL;
    return tree;
}

/*
 * Change the tree type to d if its a directory, l if its a link and - if its a regular file.
 */
void change_tree_type(struct stat stat_buf, struct TreeNode *tree){
    if(S_ISREG(stat_buf.st_mode)){
    	tree->type = '-';
    }else if(S_ISLNK(stat_buf.st_mode)){
    	tree->type = 'l';
    }else{
    	tree->type = 'd';
    }
}

/*
 * Join the old path name with the new file name and return a new joied path.
 */
char *join_name_path(const char *path, const char *name){
    char *combine = malloc(sizeof(char) * (strlen(path) + strlen(name)+1));
    if(combine == NULL){
    	perror("combine string malloc error");
	exit(1);
    }
    combine[0] = '\0';
    strcat(combine, path);
    strcat(combine, name);
    return combine;
}

/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions)
 *
 */
void print_ftree(struct TreeNode *root) {
	
    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;

    // Your implementation here.
    while(root != NULL){
	printf("%*s", depth * 2, "");
        if(root->type != 'd'){
            printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
        }else{
            printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
            depth += 1;
            print_ftree(root->contents);
        }
        root = root->next;
    }
    depth -= 1;
}


/* 
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 * 
 */
void deallocate_ftree (struct TreeNode *node) {
   
   // Your implementation here.
   struct TreeNode *next = NULL;
   while(node != NULL){
   	next = node->next;
	deallocate_ftree(node->contents);
	free(node->fname);
	free(node);
	node = next;
   }
}
