#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>

////////

struct keys {
	int l;
	int n;
	int i;
	int a;
	int R;
	int d;
};

int display_dir(char* dname, struct keys* opts);
int display_l_n_opt(struct dirent* entry, char* d_name,
	            struct stat* sb, struct keys* opts);
char* getmod_str(unsigned int* mode);
int handle_dir(int argc, char* argv[], struct keys* opts);
int handle_opts(int argc, char* argv[], struct keys* opts, int* num_dir);
int counter(int argc, char* argv[], int *num_dir);
int display_d_opt(char* dname, struct keys* opts); 

////////

int main(int argc, char* argv[])
{	
	struct keys opts = {};
	if(handle_dir(argc, argv, &opts) == -1)
		printf("Error\n");
	return 0;
}    

#define macro(symb, field) \
case (symb): { \
	opts->field = 1;\
	(*num_dir)--;\
	break;\
}

int handle_opts(int argc, char* argv[], struct keys* opts, int* num_dir)
{
	static struct option long_options [] = {
		{"long", no_argument, NULL, 'l'},
		{"numeric-uid-gid", no_argument, NULL, 'n'},
		{"inode", no_argument, NULL, 'i'},
		{"all", no_argument, NULL, 'a'},
		{"recursive", no_argument, NULL, 'R'},
		{"directory", no_argument, NULL, 'd'},
		{NULL, 0, NULL, 0},
	};
	int opt = 0;
	int option_index;
        while ((opt = getopt_long(argc, argv, "alnRid",
		long_options, &option_index)) != -1 ) {
                switch(opt) {
			macro('l', l);
			macro('n', n);
			macro('i', i);
			macro('a', a);
			macro('R', R);
			macro('d', d);
                }
        }
	return 0;
}

int counter(int argc, char* argv[], int *num_dir)
{
	for(int i = 0; i < argc; i++)
		if((argv[i][0] == '-') && 
		strlen(argv[i]) > 2 && (argv[i][1] != '-'))
			(*num_dir) += (strlen(argv[i]) - 2);
	return 0;
}

int handle_dir(int argc, char* argv[], struct keys* opts)
{
	int flag = 0;
	int num_dir = argc - 1;
	counter(argc, argv, &num_dir);
	handle_opts(argc, argv, opts, &num_dir);	
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] != '-') {
			if(display_dir(argv[i], opts) == -1)
				return -1;
		}
		if(num_dir == 0) {
			flag = 1;
			if(display_dir(".", opts) == -1)
				return -1;
			break;
		}
	}
	if(num_dir == 0 && flag != 1) {
		if(display_dir(".", opts) == -1)
			return -1;
	}
	return 0;
}

int display_d_opt(char* dname, struct keys* opts)
{
	int fd;
	struct stat sb;
	struct dirent* entry;
	if(stat(dname, &sb) == -1) {
		perror("stat failure");
		return -1;
	}
	if(opts->i)
		printf("%7ld ", sb.st_ino);
	if(opts->l && !(opts->n)) {	
		if(display_l_n_opt(entry, dname, 
		   &sb, opts) == -1)
			return -1;
	}
	if(opts->n) {
		if(display_l_n_opt(entry, dname,
	           &sb, opts) == -1)
			return -1;	
	}
	printf("%s\n", dname);
	return 0;
}

int display_dir(char* dname, struct keys* opts)
{
	DIR* dir = opendir(dname);
	int fd;
	if(!dir) {
		perror("diropen failure");
		return -1;
	}
	struct dirent* entry;	
	struct stat sb;
	if(opts->d) {
		display_d_opt(dname, opts);
		return closedir(dir);
	}
	while((entry = readdir(dir)) != NULL) {
		fd = open(dname, 0); 
		char* buf = NULL;
		asprintf(&buf, "%s%s%s", dname, "/", entry->d_name);
		if(lstat(buf, &sb) == -1) {
			perror("stat failure");
			return -1;
		}
		close(fd);
		free(buf);	
		if((entry->d_name[0] == '.') && !(opts->a))
			continue;
		if(opts->i)
			printf("%7ld ", sb.st_ino);
		if(opts->l  || opts->n) {	
			if(display_l_n_opt(entry, dname,
		           &sb, opts) == -1)
				return -1;
		}
		printf("%10s ", entry->d_name);
		char link_path[256] = {};
		if((opts->l || opts-> n) && S_ISLNK(sb.st_mode)) {
			readlink(entry->d_name, link_path, 256);
			printf("->%s\n", link_path);
		}
		printf("\n");
	}
	if (!(opts->R))
		return closedir(dir);
	//if there is r-key
	rewinddir(dir); 
	while((entry = readdir(dir)) != NULL) {	
		fd = open(dname, O_RDONLY);
		if(fstatat(fd, entry->d_name, &sb, 0) == -1) {	
			perror("stat failure");
			return -1;
		}
		close(fd);	
		if((entry->d_name[0] == '.') && !(opts->a))
			continue;	
		if (S_ISDIR(sb.st_mode)) {	
			if(strcmp(entry->d_name, ".")  == 0 
			   || strcmp(entry->d_name, "..") == 0) 
				continue;
			
			char* buf = NULL;
			asprintf(&buf, "%s%s%s", dname, "/", entry->d_name);
			printf("\n%s\n", buf);
		       	display_dir(buf, opts);
			free(buf);
		}
		else 
			continue;
	}	
	return closedir(dir);
}

int display_l_n_opt(struct dirent* entry, char* dname, struct stat* sb,
	            struct keys* opts)
{
	struct passwd* uid = getpwuid(sb->st_uid);
	struct group* gid = getgrgid(sb->st_uid);
	char* uid_str;
	char* gid_str; 
	char* mod_str = getmod_str(&sb->st_mode);
	char* data = ctime(&sb->st_mtime);
	data[strlen(data) - 1] = 0;
	printf("%10s", mod_str);	
	if(uid != NULL && !(opts->n)) {
		uid_str = uid->pw_name;
		printf("%10s", uid_str);
	}
       	else {
		uid_str = NULL;
		printf("%6u", sb->st_uid);
	}
	if(gid != NULL && !(opts->n)) {
		gid_str = gid->gr_name;
		printf("%10s", gid_str);
	}
	else {
		gid_str = NULL;
		printf("%6u", sb->st_gid);
	}
	printf("%8ld ", sb->st_size);
	printf("%12s ", data);
	free(mod_str);
	return 0;
}
#define def(num1, num2, num3) \
	if (*mode & num1) \
		mod_str[num2] = num3; \
	else \
		mod_str[num2] = '-';	
char* getmod_str(unsigned int* mode)
{
	char* mod_str = calloc(12, sizeof(char));
	def(1, 9, 'x');
	def(2, 8, 'w');
	def(4, 7, 'r');
	def(8, 6, 'x');
	def(16, 5, 'w');
	def(32, 4, 'r');
	def(64, 3, 'x');
	def(128, 2, 'w');
	def(256, 1, 'r');
	if (S_ISREG(*mode))
		mod_str[0] = '-';
	if (S_ISDIR(*mode))
		mod_str[0] = 'd';
	if (S_ISCHR(*mode))
		mod_str[0] = 'c';	
	if (S_ISBLK(*mode))
		mod_str[0] = 'b';
	if (S_ISLNK(*mode))
		mod_str[0] = 'l';
	if (S_ISSOCK(*mode))
		mod_str[0] = 's';
	return mod_str;
}
