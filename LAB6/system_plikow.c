#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define FMODE	S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#define KILO	(1024)
#define MEGA	(KILO*KILO)

#define DISK_SIZE			(8 * MEGA)
#define MAX_FILES			(64)
#define MAX_FILE_NAME_LENGTH	(64)

//after closing the file store file data in the block
#define FTABLE_SIZE			(MAX_FILES * sizeof(file_data) + sizeof(size_t))

typedef enum {TRUE = 1, FALSE = 0} bool; // for bool operations

static bool Is_Open = FALSE;

//static file descriptor for vdisk
static int descriptor_vdisk;

//disk file path in system
static char* Disk_Path;

//file data structure
typedef struct
{
	char name[MAX_FILE_NAME_LENGTH];
	off_t size;
	off_t pos;
} file_data;

//file list node
typedef struct node
{
	file_data data;
	
	struct node* next;
} node;

//file list
static node* Head;
static node* Tail;

//free space in bytes
static off_t Free_Space;

//write offset from the beginning of free space
static off_t Offset;

//occupied ftable slots
static size_t N_Slots;

//begin of list of functions used in program
bool is_unique(const char*);
void open_disk(const char* file_path);
void cp_sys_disk(const char* src_path, const char* file_name);
void cp_disk_sys(const char* dst_path, const char* file_name);
void remove_file(const char* file_name);
void close_disk();
void remove_disk();
void ls_disk();
void show_info();

//end of list of functions used in program

/*---IS FILENAME UNIQUE---*/

bool is_unique(const char* file_name)
{
	node* temp = Head;

	while(temp)
	{
		if(!strcmp(file_name, temp->data.name))
			return FALSE;
		temp = temp->next;
	}
	return TRUE;
}

/*---OPEN DISK---*/

void open_disk(const char* file_path)
{
	//opens a file to be used as a virtual disk
	//if the file does not exist, it is created
	if(Is_Open)
	{
		printf("%s is already opened\n", Disk_Path);
		return;
	}
	int error_code;

	descriptor_vdisk = open(file_path, O_RDWR | O_CREAT | O_EXCL, FMODE); 
	//open with RW rigths or create, or set error if exist 

	if(descriptor_vdisk == -1)
	{
		error_code = errno;

		//the file exists so load file data
		if(error_code == EEXIST)
		{
			descriptor_vdisk = open(file_path, O_RDWR);

			//read number of table slots
			lseek(descriptor_vdisk, MAX_FILES * sizeof(file_data), SEEK_SET);
			read(descriptor_vdisk, &N_Slots, sizeof(size_t));

			lseek(descriptor_vdisk, 0, SEEK_SET);
			Free_Space = DISK_SIZE;

			//read all nodes from disk table
			int i;
			node* temp;

			for(i=0; i < N_Slots; ++i)
			{
				temp = (node*)malloc(sizeof(node));

				read(descriptor_vdisk, &(temp->data), sizeof(file_data));

				temp->next = NULL;

				Free_Space -= temp->data.size;

				if(!Head)
					Tail = Head = temp;

				else
				{
					Tail->next = temp;
					Tail = temp;
				}
			}

			if(!N_Slots)
				Offset = 0;

			else
				Offset = Tail->data.pos + Tail->data.size;

			lseek(descriptor_vdisk, Offset + FTABLE_SIZE, SEEK_SET);

			Disk_Path = file_path;

			Is_Open = TRUE;

			return;
		}

		//other error
		else
		{
			perror("error with open");

			return;
		}
	}

	//file does not exist and will be created

	//check if ftruncate failed
	if(ftruncate(descriptor_vdisk, DISK_SIZE + FTABLE_SIZE) == -1)
	{
		perror("error with ftruncate");

		return;
	}

	//now we have a newly created file

	//set file offset to the beginning of free space
	lseek(descriptor_vdisk, FTABLE_SIZE, SEEK_SET);

	//make sure the pointer is never invalidated!
	Disk_Path = file_path;

	Offset = 0;
	N_Slots = 0;
	Free_Space = DISK_SIZE;

	Head = Tail = NULL;

	Is_Open = TRUE;
}


/*---COPY SYS-DISK---*/

void cp_sys_disk(const char* src_path, const char* file_name)
{
	
	struct stat info;
	
	if(stat(src_path, &info) == -1)
	{
		perror("error with struct stat");
		
		return;
	}
	
	//check if allocation table is full
	if(N_Slots == MAX_FILES)
	{
		fprintf(stderr, "file number limit reached\n");
		
		return;
	}
	
	//check if there is not enough free space
	if(Free_Space < info.st_size)
	{
		fprintf(stderr, " not enough free space\n");
		
		return;
	}
	
	//check if file_name length is too big
	if(strlen(file_name) >= MAX_FILE_NAME_LENGTH)
	{
		fprintf(stderr, "%s: file name too long\n", file_name);
		
		return;
	}
	
	//check if file_name already exists
	if(!is_unique(file_name))
	{
		fprintf(stderr, "%s: file name already exists\n", file_name);
		
		return;
	}
	
	//create new list node
	node* temp = (node*)malloc(sizeof(node));
	
	++N_Slots;
	
	strcpy(temp->data.name, file_name);
	
	temp->data.size = info.st_size;
	temp->data.pos =	Offset;
	temp->next = NULL;
	
	//if disk empty initialize list
	if(!Head)
		Tail = Head = temp;
	
	//else add new node at the end of file list
	else
	{
		Tail->next = temp;
		Tail = temp;
	}
	
	//open file to be copied
	int src_fd = open(src_path, O_RDONLY);
	
	char* buffer = (char*)malloc(info.st_size);
	
	//copy file
	read(src_fd, buffer, info.st_size);
	write(descriptor_vdisk, buffer, info.st_size);
	
	free(buffer);
	
	//close copied file
	close(src_fd);
	
	//update variables;
	Offset += info.st_size;
	Free_Space -= info.st_size;
	
	printf("%s: operation completed successfully\n", file_name);
}


/*---COPY DISK-SYS---*/

void cp_disk_sys(const char* dst_path, const char* file_name)
{
	
	if(!N_Slots)
	{
		fprintf(stderr, "disk is empty\n");
		
		return;
	}
	
	if(strlen(file_name) >= MAX_FILE_NAME_LENGTH)
	{
		fprintf(stderr, "%s: file name too long\n", file_name);
		
		return;
	}
	
	node* p = Head;
	
	while(p)
	{
		//file found - execute copy operation
		if(!strcmp(file_name, p->data.name))
		{
			//attempt to open destination file
			int dst_fd = open(dst_path, O_WRONLY | O_TRUNC | O_CREAT, FMODE);
			
			if(dst_fd == -1)
			{
				perror("error with open");
				
				return;
			}
			
			char* buffer = (char*)malloc(p->data.size);
			
			//read copied file to buffer
			pread(descriptor_vdisk, buffer, p->data.size, p->data.pos + FTABLE_SIZE);
			
			//write data to dest file
			write(dst_fd, buffer, p->data.size);
			
			free(buffer);
			
			close(dst_fd);
			
			printf("%s: operation completed successfully\n", file_name);
			
			return;
		}
		
		p = p->next;
	}
	
	fprintf(stderr, "%s: file requested for copy does not exist\n", file_name);
}


/*---remove_file---*/

void remove_file(const char* file_name)
{
	
	if(!N_Slots)
	{
		fprintf(stderr, "Disk is empty\n");
		
		return;
	}
	
	if(strlen(file_name) >= MAX_FILE_NAME_LENGTH)
	{
		fprintf(stderr, "%s: file name too long\n", file_name);
		
		return;
	}
	
	node *p = Head;
	node *prev = NULL;
	
	while(p)
	{
		//file found - execute removal
		if(!strcmp(file_name, p->data.name))
		{
			//shiftpos all files after the removed one
			node* sp = p->next;
			off_t shifted = 0;
			off_t shiftbeg;
			
			//set shift begin position
			if(sp)
				shiftbeg = sp->data.pos;
			
			while(sp)
			{
				//update file position
				//for all shifted files
				sp->data.pos -= p->data.size;
				shifted += sp->data.size;
				
				sp = sp->next;
			}
			
			//if any file is shifted
			if(p->next)
			{
				//create a buffer in memory
				//for the shift operation
				char* buffer = (char*)malloc(shifted);
				
				//read shifted files to buffer
				lseek(descriptor_vdisk, shiftbeg + FTABLE_SIZE, SEEK_SET);
				read(descriptor_vdisk, buffer, shifted);
				
				//write shifted files to new position
				lseek(descriptor_vdisk, p->data.pos + FTABLE_SIZE, SEEK_SET);
				write(descriptor_vdisk, buffer, shifted);
				
				//free allocated memory
				free(buffer);
			}
			
			//removed file is on the last position
			else
				Tail = prev;
			
			//update write offset
			if(Tail)
				Offset = Tail->data.pos + Tail->data.size;
			
			else
				Offset = 0;
			
			lseek(descriptor_vdisk, Offset + FTABLE_SIZE, SEEK_SET);
			
			//update amount of free space
			Free_Space += p->data.size;
			
			//update file list
			--N_Slots;
			
			if(prev)
				prev->next = p->next;
			
			else
				Head = p->next;
			
			//erase list node
			free(p);
			
			printf("%s: removed successfully\n", file_name);
			
			return;
		}
		
		prev = p;
		
		p = p->next;
	}
	
	fprintf(stderr, "%s: file requested for removal not found\n", file_name);
}


/*---CLOSE DISK---*/

void close_disk()
{
	//close disk and write file data in the data sector
	//for future operations
	
	node* p = Head;
	node* del;
	
	//set file offset to the beginning
	lseek(descriptor_vdisk, 0, SEEK_SET);
	
	//go through file list and write data to disk
	//also erase the list
	while(p)
	{
		write(descriptor_vdisk, &(p->data), sizeof(file_data));
		
		del = p;
		p = p->next;
		
		free(del);
	}
	
	Tail = Head = NULL;
	
	//write number of slots at the end of data sector
	lseek(descriptor_vdisk, MAX_FILES * sizeof(file_data), SEEK_SET);
	
	write(descriptor_vdisk, &N_Slots, sizeof(size_t));
	
	//finally close the disk
	close(descriptor_vdisk);
	
	Is_Open = FALSE;
}


/*---REMOVE DISK---*/

void remove_disk()
{
	
	close(descriptor_vdisk);
	
	unlink(Disk_Path);
	
	node* p = Head;
	node* del;
	
	while(p)
	{
		del = p;
		p = p->next;
		
		free(del);
	}
	
	Head = Tail = NULL;
	
	Is_Open = FALSE;
	
	printf("disk removed successfully\n");
}


/*---SHOW DIR---*/

void ls_disk()
{
	printf("Files: %lu / %d\nAllocated space: %ld / %d bytes\n", N_Slots, MAX_FILES, Offset, DISK_SIZE);
	
	if(Head)
	{
		char name_string[7] = "[NAME]\0";
		printf("\n%-64s\t[SIZE]\t[OFFSET]\n\n", name_string);
		
		node* p = Head;
		
		while(p)
		{
			printf("%-64s\t%ld\t%ld\n", p->data.name, p->data.size, p->data.pos);
			p = p->next;
		}
	}
}


/*---SHOW INFO---*/

void show_info()
{

	printf("[TYPE]\t[BEGIN]\t\t[END]\t\t[SIZE]\t\t[STATE]\n\n");
	
	printf("T\t0\t\t%lu\t\t%lu\t\tReserved\n", FTABLE_SIZE - 1, FTABLE_SIZE);
	
	if(Offset)
		printf("D\t%lu\t\t%lu\t\t%ld\t\tUsed\n", FTABLE_SIZE, FTABLE_SIZE + Offset - 1, Offset);
	
	if(Free_Space)
		printf("D\t%lu\t\t%lu\t\t%ld\t\tFree\n", FTABLE_SIZE + Offset, FTABLE_SIZE + DISK_SIZE - 1, Free_Space);
}



/*---MAIN---*/

int main(int argc, char** argv)
{
	open_disk(argv[1]);

	if(!strcmp(argv[2], "-ls"))
	{
		ls_disk();
	}
	else if(!strcmp(argv[2], "-i"))
	{
		show_info();
	}
	else if(!strcmp(argv[2], "-csd"))
	{
		cp_sys_disk(argv[3], argv[4]);
	}
	else if(!strcmp(argv[2], "-cds"))
	{
		cp_disk_sys(argv[3], argv[4]);
	}
	else if(!strcmp(argv[2], "-rmf"))
	{
		remove_file(argv[3]);
	}
	else if(!strcmp(argv[2], "-rmd"))
	{
		remove_disk();
	}
	//close disk after program has finished
	if(Is_Open)
		close_disk();
	return 0;
}
