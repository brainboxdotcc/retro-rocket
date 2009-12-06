#ifndef PAGING_H
#define PAGING_H

typedef struct page
{
	u64 present    : 1;   // Page present in memory
	u64 rw         : 1;   // Read-only if clear, readwrite if set
	u64 user       : 1;   // Supervisor level only if clear
	u64 accessed   : 1;   // Has the page been accessed since last refresh?
	u64 dirty      : 1;   // Has the page been written to since last refresh?
	u64 unused     : 7;   // Amalgamation of unused and reserved bits
	u64 frame      : 52;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table
{
	page_t pages[1024];
} page_table_t;

typedef struct page_directory
{
	/**
	 * Array of pointers to pagetables.
	 **/
	page_table_t *tables[1024];
	/**
	 * Array of pointers to the pagetables above, but gives their *physical*
	 * location, for loading into the CR3 register.
	 **/
	u64 tablesPhysical[1024];

	 /**
	  * The physical address of tablesPhysical. This comes into play
	  * when we get our kernel heap allocated and the directory
	  * may be in a different location in virtual memory.
	  **/
	u64 hysicalAddr;
} page_directory_t;

/**
 * Sets up the environment, page directories etc and
 * enables paging.
 **/
void initialise_paging();

/**
 * Causes the specified page directory to be loaded into the
 * CR3 register.
 **/
void switch_page_directory(page_directory_t *new);

/**
 * Retrieves a pointer to the page required.
 * If make == 1, if the page-table in which this page should
 * reside isn't created, create it!
 **/
page_t *get_page(u64 address, int make, page_directory_t *dir);

#endif
