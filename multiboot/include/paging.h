#ifndef __PAGING_H__
#define __PAGING_H__

#define TABLESIZE	1024
#define USTACK 		0x80000000
#define USTACK_SIZE	0x2000

typedef struct page
{
	u32int present	: 1;	/* Page is present */
	u32int rw	: 1;	/* Writeable page */
	u32int user	: 1;	/* Set to 1 if this is a user accessible page */
	u32int accessed	: 1;	/* Page has been accessed */
	u32int dirty	: 1;	/* Page has been written to */
	u32int unused	: 7;	/* Various unused and reserved fields*/
	u32int frame	: 20;	/* Memory Frame Address */
}page_t;

typedef struct page_table
{
	page_t pages[TABLESIZE];	/*Apla periexei 1024 page entries */
} page_table_t;

typedef struct page_directory
{
	/* Array of pointers to pagetables. */
	page_table_t *tables[TABLESIZE];
	/* Array of pointers to the pagetables above, but gives their *physical*
	 * location, for loading into the CR3 register.
	 */
	u32int tablesPhysical[TABLESIZE];
	/* The physical address of tablesPhysical. This comes into play
	 * when we get our kernel heap allocated and the directory
	 * may be in a different location in virtual memory.
	 */
	u32int phys;
} page_directory_t;

u32int init_paging(void* mbd);
void alloc_frame(page_t *page, u8int f_usr, u8int f_rw);
void free_frame(page_t *page);
void switch_page_directory(page_directory_t*);
void sign_sect(u32int start, u32int end, u8int usr, u8int rw, page_directory_t *dir1);
void release_sect(u32int start, u32int end, page_directory_t *dir);
page_t *get_page(u32int addr, u8int make, page_directory_t *dir);
page_directory_t *clone_directory(page_directory_t *src);
void kill_directory(page_directory_t *src);
int invalid_frame(u32int physaddr);
page_directory_t *init_procdir(void);
void print_heapinfo();

#endif

