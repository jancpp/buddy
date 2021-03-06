/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1 << MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx * PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1 << o)) + (unsigned long)g_memory)

#if USE_DEBUG == 1
#define PDEBUG(fmt, ...)                 \
	fprintf(stderr, "%s(), %s:%d: " fmt, \
			__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#define IFDEBUG(x) x
#else
#define PDEBUG(fmt, ...)
#define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct
{
	struct list_head list;
	/* TODO: DECLARE NECESSARY free VARIABLES */
	char *address;
	int order;
	int is_free;
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER + 1];

/* memory area */
char g_memory[1 << MAX_ORDER];

/* page structures */
page_t g_pages[(1 << MAX_ORDER) / PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/


 page_t* merge(page_t *block, page_t *buddy_block)
{
	list_del(&buddy_block->list);
	if(block->address > buddy_block->address)
	{
		block = buddy_block;
		buddy_block = NULL;
	}
	else
	{
		buddy_block = NULL;
	}
		block->order++;
		return block;
}

// Split the block, add new block to list of free blocks
// and return pointer to one we'll use
page_t split(page_t *block_to_alloc, page_t *block_buddy)
{
	int temp_order = block_to_alloc->order;
	temp_order--;

	char *buddy_address = BUDDY_ADDR(block_to_alloc->address, (temp_order));
	block_buddy = &g_pages[ADDR_TO_PAGE(buddy_address)];

	block_buddy->address = buddy_address;
	block_buddy->order = temp_order;
	block_buddy->is_free = 1;
	list_add(&block_buddy->list, &free_area[temp_order]);

	block_to_alloc->order = temp_order;
	block_to_alloc->is_free = 0;
	return *block_to_alloc;
}

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1 << MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++)
	{
		/* TODO: INITIALIZE PAGE STRUCTUreRES */
		g_pages[i].address = (char *)PAGE_TO_ADDR(i);
		g_pages[i].order = MAX_ORDER;
		g_pages[i].is_free = 1;
		INIT_LIST_HEAD(&g_pages[i].list);
	}

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++)
	{
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{

	// Size of a block we need
	int block_order = ceil(log2(size));

	// Find block
	page_t *block_to_alloc = NULL;
	page_t *block_buddy = NULL;
	struct list_head *head = NULL;

	if (block_order < MIN_ORDER) {
		block_order = MIN_ORDER;
	}
	for (int i = block_order; i <= MAX_ORDER; i++)
	{
		if (!list_empty(&free_area[i]))
		{
			head = free_area[i].next;
			block_to_alloc = list_entry(head, page_t, list);
			list_del(head);
			break;
		}
	}

	// Keep spliting block until we have a right size
	if(block_to_alloc == NULL)
	{
		return NULL;
	}
	if (block_to_alloc->is_free == 1)
	{
		while (block_to_alloc->order != block_order)
		{
			*block_to_alloc = split(block_to_alloc, block_buddy);
		}
	}
	block_to_alloc->order = block_order;
	block_to_alloc->is_free = 0;
	return block_to_alloc->address;
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	page_t *block_to_free = &g_pages[ADDR_TO_PAGE(addr)];
	block_to_free->is_free = 1;
	if(block_to_free->order >= MAX_ORDER)
	{
		list_add(&block_to_free->list, &free_area[block_to_free->order]);
		return;
	}
	char *buddy_address = (char *)BUDDY_ADDR(addr, block_to_free->order);
	page_t *buddy_block = &g_pages[ADDR_TO_PAGE(buddy_address)];
	if(buddy_block->is_free == 1 && buddy_block->order == block_to_free->order)
	{
		block_to_free = merge(block_to_free, buddy_block);
		buddy_free(block_to_free->address);
	}
	else
	{
		list_add(&block_to_free->list, &free_area[block_to_free->order]);
	}
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++)
	{
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o])
		{
			cnt++;
		}
		printf("%d:%dK ", cnt, (1 << o) / 1024);
	}
	printf("\n");
}
