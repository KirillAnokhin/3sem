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

enum KEYS {
	L_KEY,
	N_KEY,
	I_KEY,
	A_KEY,
	R_KEY,
	NUM_KEYS,
};

int parse_keys_ex_dir(int argc, char* argv[], char keys[]);
int display_dir(char* dname, char keys[]);
int display_l_n_opt(struct dirent* entry, char* d_name, struct stat* sb, char keys[]);
char* getmod_str(unsigned int* mode);
int handle_keys(int argc, char* argv[], char keys[], int* num_dir);
int handle_dir(int argc, char* argv[], char keys[]);

////////

int main(int argc, char* argv[]) {
	
	//printf("\x1b[32maaaaa\x1b[0m\n");	
	char keys[NUM_KEYS] = {};
	if(handle_dir(argc, argv, keys) == -1)
		printf("Error\n");
	return 0;
}

int handle_keys(int argc, char* argv[], char keys[], int* num_dir) {
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-l") == 0) {
			keys[L_KEY] = 1;
			(*num_dir)--;
		}
		else if(strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric-uid-gid") == 0) {
			keys[N_KEY] = 1;
			(*num_dir)--;
		}
		else if(strcmp(argv[i], "-i") == 0) {
			keys[I_KEY] = 1;
			(*num_dir)--;
		}
		else if(strcmp(argv[i], "-a") == 0) {
			keys[A_KEY] = 1;
			(*num_dir)--;
		}
		else if(strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--recursive") == 0) {
			keys[R_KEY] = 1;
			(*num_dir)--;
		}
		
	}
	return 0;
}

int handle_dir(int argc, char* argv[], char keys[]) {
	int flag = 0;
	int num_dir = argc - 1;
	handle_keys(argc, argv, keys, &num_dir);
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] != '-') {
			//printf("%s:\n", argv[i]);
			if(display_dir(argv[i], keys) == -1)
				return -1;
		}
		if(num_dir == 0) {
			flag = 1;
			if(display_dir(".", keys) == -1)
				return -1;
			break;
		}
	}
	if(num_dir == 0 && flag != 1) {
		if(display_dir(".", keys) == -1)
			return -1;
	}
	return 0;

}

int display_dir(char* dname, char keys[]) {
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

		if((entry->d_name[0] == '.') && !keys[A_KEY])
		{
			close(fd);
			continue;
		}	
		if(keys[I_KEY])
			printf("%7ld ", sb.st_ino);
		if(keys[L_KEY] && !keys[N_KEY]) {	
			if(display_l_n_opt(entry, dname, &sb, keys) == -1)
				return -1;
		}
		if(keys[N_KEY]) {
			if(display_l_n_opt(entry, dname, &sb, keys) == -1)
				return -1;	
		}
		printf("%10s\n", entry->d_name);
		close(fd);
	}

	if (!keys[R_KEY])
		return closedir(dir);

	rewinddir(dir); 
	while((entry = readdir(dir)) != NULL) {	
		fd = open(dname, O_RDONLY);
		if(fstatat(fd, entry->d_name, &sb, 0) == -1) {	
			perror("stat failure");
			return -1;
		}
			
		if((entry->d_name[0] == '.') && !keys[A_KEY]) {
			close(fd);
			continue;	
		}
		if (S_ISDIR(sb.st_mode)) {	
			if(strcmp(entry->d_name, ".")  == 0 || strcmp(entry->d_name, "..") == 0) {
				close(fd);
				continue;
			}	
			char* buf = NULL;
			asprintf(&buf, "%s%s%s", dname, "/", entry->d_name);
			printf("\n%s\n", buf);
		       	display_dir(buf, keys);
			free(buf);
		}
		else {
			close(fd);
		
			continue;
		}
		close(fd);
	}		
	return closedir(dir);
}

int display_l_n_opt(struct dirent* entry, char* dname, struct stat* sb, char keys[]) {
	struct passwd* uid = getpwuid(sb->st_uid);
	struct group* gid = getgrgid(sb->st_uid);
	char* uid_str;
	char* gid_str; 
	char* mod_str = getmod_str(&sb->st_mode);
	char* data = ctime(&sb->st_mtime);
	data[strlen(data) - 1] = 0;

	printf("%10s", mod_str);	
	
	if(uid != NULL && !keys[N_KEY]) {
		uid_str = uid->pw_name;
		printf("%10s", uid_str);
	}
       	else {
		uid_str = NULL;
		printf("%6u", sb->st_uid);
	}

	if(gid != NULL && !keys[N_KEY]) {
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
