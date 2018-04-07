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

// Split the block, add new block to list of free blocks
// and return pointer to one we'll use
page_t split(page_t *block_alloc, page_t *block_buddy)
{
	int temp_order = block_alloc->order;

	char *buddy_address = BUDDY_ADDR(block_alloc->address, (temp_order - 1));
	block_buddy = &g_pages[ADDR_TO_PAGE(buddy_address)];

	block_buddy->address = buddy_address;
	block_buddy->order = temp_order - 1;
	block_buddy->is_free = 1;
	list_add(&block_buddy->list, &free_area[temp_order - 1]);
	
	block_alloc->order--;
	block_alloc->is_free = 1;
	return *block_alloc;
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
	page_t *block_alloc = NULL;
	page_t *block_buddy = NULL;
	struct list_head *head;

	// Find smallest free block
	for (int i = block_order; i <= MAX_ORDER; i++)
	{
		list_for_each(head, &free_area[i])
		{
			block_alloc = list_entry(head, page_t, list);
			if (block_alloc->is_free == 1)
			{
				// Found the block, remove from list of free blocks and use it
				list_del(head);
				break;
			}
		}
	}

	// Keep spliting block until we have a right size
	if (block_alloc->is_free == 1)
	{
		while (block_alloc->order != block_order)
		{
			*block_alloc = split(block_alloc, block_buddy);
		}
	}

	if (block_alloc == NULL)
	{
		return NULL;
	}
	else
	{
		return block_alloc->address;
	}
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
	/* TODO: IMPLEMENT THIS FUNCTION */
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
