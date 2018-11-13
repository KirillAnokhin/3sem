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

////////


struct keys {
	int l;
	int n;
	int i;
	int a;
	int R;
	int d;
};

int parse_keys_ex_dir(int argc, char* argv[], char keys[]);
int display_dir(char* dname, struct keys* opts);
int display_l_n_opt(struct dirent* entry, char* d_name, struct stat* sb, struct keys* opts);
char* getmod_str(unsigned int* mode);
int handle_keys(int argc, char* argv[], char keys[], int* num_dir, struct keys* opts);
int handle_dir(int argc, char* argv[], struct keys* opts);
int handle_opts(int argc, char* argv[], struct keys* opts, int* num_dir);
int crutch(int argc, char* argv[], int *num_dir);

////////

int main(int argc, char* argv[]) {
	
	struct keys opts = {};

	if(handle_dir(argc, argv, &opts) == -1)
		printf("Error\n");

	return 0;
}

int handle_opts(int argc, char* argv[], struct keys* opts, int* num_dir)
{
        int opt = 0;
        while (opt != -1 ) {
                opt = getopt(argc, argv, "alnRid");
                switch(opt) {
                        case ('l'):
			{
                                opts->l = 1;
				(*num_dir)--;
                                break;
                        }
                        case ('n'):
                        {
                                opts->n = 1;
				(*num_dir)--;
				break;
                        }
                        case ('i'):
                        {
                                opts->i = 1;
				(*num_dir)--;
				break;
                        }
                        case ('a'):
                        {
                                opts->a = 1;
				(*num_dir)--;
				break;
                        }
                        case ('R'):
                        {
                                opts->R = 1;
				(*num_dir)--;
				break;
                        }
                        case ('d'):
                        {
                                opts->d = 1;
				(*num_dir)--;
				break;
                        }
                }
        }

	return 0;
}

int crutch(int argc, char* argv[], int *num_dir)
{
	for(int i = 0; i < argc; i++)
		if((argv[i][0] == '-') && strlen(argv[i]) > 2)
			(*num_dir) += (strlen(argv[i]) - 2);

	return 0;
}

int handle_dir(int argc, char* argv[], struct keys* opts) {
	int flag = 0;
	int num_dir = argc - 1;
	crutch(argc, argv, &num_dir);
	handle_opts(argc, argv, opts, &num_dir);
	
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] != '-') {
			//printf("%s:\n", argv[i]);
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

int display_dir(char* dname, struct keys* opts) {
	DIR* dir = opendir(dname);
	int fd;
	if(!dir) {
		perror("diropen failure");
		return -1;
	}
	struct dirent* entry;
	struct stat sb;
	int dflag = 0;
	while((entry = readdir(dir)) != NULL) {
		fd = open(dname, 0);
		if(fstatat(fd, entry->d_name, &sb, 0) == -1) {	
			perror("stat failure");
			return -1;
		}
		/*
		if(S_ISLNK())
			close(fd);
		*/
		if((entry->d_name[0] == '.') && !(opts->a))
			continue;
		if(opts->i)
			printf("%7ld ", sb.st_ino);
		if(opts->l && !(opts->n)) {	
			if(display_l_n_opt(entry, dname, &sb, opts) == -1)
				return -1;
		}
		if(opts->n) {
			if(display_l_n_opt(entry, dname, &sb, opts) == -1)
				return -1;	
		}
		printf("%10s\n", entry->d_name);
	}

	if (!(opts->R))
		return closedir(dir);

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
			if(strcmp(entry->d_name, ".")  == 0 || strcmp(entry->d_name, "..") == 0) 
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

int display_l_n_opt(struct dirent* entry, char* dname, struct stat* sb, struct keys* opts) {
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

char* getmod_str(unsigned int* mode){
	char* mod_str = calloc(12, sizeof(char));
	if(*mode & 1)
		mod_str[9] = 'x';
	else	
		mod_str[9] = '-';

	if(*mode & 2) 
		mod_str[8] = 'w';
	else
		mod_str[8] = '-';

	if(*mode & 4)
		mod_str[7] = 'r';
	else
		mod_str[7] = '-';

	if(*mode & 8)
		mod_str[6] = 'x';
	else
		mod_str[6] = '-';

	if(*mode & 16)
		mod_str[5] = 'w';
	else
		mod_str[5] = '-';

	if(*mode & 32)
		mod_str[4] = 'r';
	else
		mod_str[4] = '-';
		
	if(*mode & 64)
		mod_str[3] = 'x';
	else
		mod_str[3] = '-';

	if(*mode & 128)
		mod_str[2] = 'w';
	else
		mod_str[2] = '-';

	if(*mode & 256)
		mod_str[1] = 'r';
	else
		mod_str[1] = '-';

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
