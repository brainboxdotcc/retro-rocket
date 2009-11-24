#ifndef __PAGING_H__
#define __PAGING_H__

#define USTACK 		0x80000000
#define USTACK_SIZE	0x2000

typedef struct page {
	u32int present	:1;	/*Flag an yparxei stin mnimi */
	u32int rw	:1;	/*Read Write  == 1 */
	u32int user	:1;	/*SU == 0 */
	u32int accessed	:1;
	u32int dirty	:1;
	u32int unused	:7;
	u32int frame	:20;	/*Memory Frame Address */
}page_t;

typedef struct page_table {
	page_t pages[1024];	/*Apla periexei 1024 page entries */
}page_table_t;

typedef struct page_directory {
	page_table_t *tables[1024];	/*pointers gia tables */
	u32int tablesPhysical[1024];	/*Tha krataei tis fisikes addresses twn tables
					  Ta entries einai episis se morfi page_t
					  opote kai ta teleftea 12 bits xrisimopoiountai
					  san attributes. Gia na paroume tin diefthinsi tou 
					  frame, apla kanoume Page Align (& 0xFFFFF000) */
	u32int phys;			/*Fusiki diefthinsi tou dir */
}page_directory_t;

u32int init_paging(void* mbd);
void alloc_frame(page_t *page, u8int f_usr, u8int f_rw);
void free_frame(page_t *page);
void switch_page_directory(page_directory_t*);
void sign_sect(u32int start, u32int end, u8int usr, u8int rw, page_directory_t *dir1);
void release_sect(u32int start, u32int end, page_directory_t *dir);
page_t *get_page(u32int addr, u8int make, page_directory_t *dir);
page_directory_t *clone_directory(page_directory_t *src);
void kill_directory(page_directory_t *src);
page_directory_t *init_procdir(void);

#endif

