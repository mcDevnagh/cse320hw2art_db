#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "warehouse.h"
#define BOOLEAN char
#define TRUE 1
#define FALSE 0

/*
 * badInt()
 * Checks to see if the ID is valid
 *
 * Params:	id
 * 		the ID to be checked
 *
 * 		userInput
 * 		if true, it gives the user feedback, if not, the feedback is suppressed (for use in nextGoodID())
 *
 * Return:	TRUE if ID is negative or if a warehouse in the data structure already has that ID
 * 		FALSE otherwise
 */
BOOLEAN badID(int id, BOOLEAN userInput){
	if (id<=0){
		if(userInput)
			printf("ERROR: All ID's must be positive. %d is not!\n", id);
		return TRUE;
	}
	if (!sf_head)
		return FALSE;
	else{
		struct warehouse_sf_list* sf_cursor = sf_head;
		struct warehouse_list* wl_cursor;
		while (sf_cursor){
			wl_cursor = sf_cursor->warehouse_list_head;
			while (wl_cursor){
				if (id == wl_cursor->warehouse->id){
					if (userInput)
						printf("ERROR: All ID's must be unique. %d is not!", id);
					return TRUE;
				}
				wl_cursor = wl_cursor->next_warehouse;
			}
			sf_cursor = sf_cursor->sf_next_warehouse;
		}
		return FALSE;
	}
}

/*
 * nextGoodID()
 * finds the next valid ID to be used in cases of splitting a warehouse into 2
 *
 * Params: void
 *
 * Return:
 * 	integer ID that can be used by a subsequent warehouse
 */
int nextGoodID(){
	int output = 5000;
	while (badID(++output, FALSE));
	return output;
}	

/*
 * createWarehouse()
 * allocates space for a Warehouse struct, initialized with a unique ID, a specified size, and a NULL art collection
 *
 * Params:
 * 	id
 * 	integer used to identify the warehouse
 * 	
 * 	size
 * 	the storage capacity of the warehouse
 *
 * Return:
 * 	pointer to a newly malloc'd warehouse struct
 */
struct warehouse* createWarehouse(int id, int size){
	if (badID(id, TRUE)){
		return NULL;
	}
	if ((size & 1) || (size<4)){
		printf("ERROR: warehouse size must be a multiple of 2 and greated than 4, %d has size of %d\n", id, size);
		return NULL;
	}
	struct warehouse* output = malloc(sizeof(struct warehouse));
	output->id = id;
	output->size = size;
	output->art_collection = NULL;
	return output;
}

/*
 * createWarehouseList()
 * malloc's space for a member of a warehouse list and initializes its values
 *
 * Params: 
 * 	warehouse
 * 	the payload of the warehouse list member
 *
 * 	private
 * 	BOOLEAN used to initialized the single bit of meta_info to indicate if the warehouse is private or public
 *
 * Return:	
 * 	pointer to newly malloc'd warehouse struct
 */
struct warehouse_list* createWarehouseList(struct warehouse* warehouse, BOOLEAN private){
	struct warehouse_list* output = malloc(sizeof(struct warehouse_list));
	output->warehouse = warehouse;
	output->meta_info = ((warehouse->size)<<1) | (private & 1);
	output->next_warehouse = NULL;
	return output;
}

/*
 * createWarehouseSFList()
 * allocates memory for a new class size of warehouses for the segregated free list
 * 
 * Params:
 * 	class_size
 *	size all members of its warehouse list will be
 *	
 *	warehouse_list_head
 *	Since this function is only called if a warehouse is created, but an entry in the Segregated List is yet to exist, that newly created warehouse will be the beginning of the list for its class size
 * 
 * Return:
 * 	pointer to the newly malloc'd warehouse_sf_list [member]
 */
struct warehouse_sf_list* createWarehouseSFList(int class_size, struct warehouse_list* warehouse_list_head){
	struct warehouse_sf_list* output = malloc(sizeof(struct warehouse_sf_list));
	output->class_size = class_size;
	output->warehouse_list_head = warehouse_list_head;
	return output;
}

/*
 * insertWarehouseSFList()
 * inserts a the new member of the segrated free list in its correct position relative to the already inserted members
 *
 * Params:	
 * 	toBeInserted
 * 	the member of the segregated free list that this function will insert into the free list
 *
 * Return:
 * 	void	
 */
void insertWarehouseSFList(struct warehouse_sf_list* toBeInserted){
	if (!toBeInserted)
		return;
	if (!sf_head){
		sf_head = toBeInserted;
	}
	else{
		if (sf_head->class_size > toBeInserted->class_size){
			toBeInserted->sf_next_warehouse = sf_head;
			sf_head = toBeInserted;
			return;
		}
		struct warehouse_sf_list* cursor = sf_head->sf_next_warehouse;
		struct warehouse_sf_list* prev = cursor;
		while (cursor){
			if (cursor->class_size > toBeInserted->class_size){
				toBeInserted->sf_next_warehouse = cursor;
				prev->sf_next_warehouse = toBeInserted;
				return;
			}
			prev = cursor;
			cursor = cursor->sf_next_warehouse;
		}
		prev->sf_next_warehouse = toBeInserted;
		toBeInserted->sf_next_warehouse = NULL;
	}
}

/*
 * insertWarehouseList
 * inserts a warehouse list into a new SF List member, since insertWarehouse() appends warehouse lists to already made SF List members
 * 
 * Params: 
 * 	warehouse_list
 * 	list to be the head of a new SF List member
 *
 * Return:
 * 	void
 */
void insertNewWarehouseList(struct warehouse_list* warehouse_list){
	if (!warehouse_list)
		return;
	int class_size = (warehouse_list->meta_info >> 1) & -2;
	
	struct warehouse_sf_list* new_sf = createWarehouseSFList( class_size, warehouse_list );
	insertWarehouseSFList(new_sf);
	new_sf->warehouse_list_head = warehouse_list;
}

/*
 * insertWarehouse()
 * Creates a new warehouse list member (as a wrapper) either appends it to the SF List of its class size, or creates a new SF List of its class size
 *
 * Params:
 * 	warehouse
 * 	warehouse to be wrapped and inserted
 *
 * 	private
 * 	BOOLEAN to indicate whether the warehouse is private or public
 *
 * Return:
 * 	void
 */
void insertWarehouse(struct warehouse* warehouse, BOOLEAN private){
	if (!warehouse)
		return;
	if (!sf_head){
		insertNewWarehouseList( createWarehouseList( warehouse, private ));
		return;
	}
	struct warehouse_sf_list* sf_cursor = sf_head;
	while (sf_cursor){
		if (sf_cursor->class_size == warehouse->size){
			struct warehouse_list* wl_cursor = sf_cursor->warehouse_list_head;
			if (!wl_cursor){
				sf_cursor->warehouse_list_head = createWarehouseList(warehouse, private);
				return;
			}
			while (wl_cursor->next_warehouse){
				wl_cursor = wl_cursor->next_warehouse;
			}
			wl_cursor->next_warehouse = createWarehouseList(warehouse, private);
			return;
		}
		sf_cursor = sf_cursor->sf_next_warehouse;
	}
	insertNewWarehouseList( createWarehouseList( warehouse, private ));
}

/*
 * loadWarehouseFile()
 * creates and inserts warehouses from file each specified by ID SIZE TYPE\n
 *
 * Params:
 * 	warehouseFile
 * 	pointer to the opened file to be read
 *
 * Return:
 * 	void
 */

void loadWarehouseFile(FILE* warehouseFile){
	char* commandLine = malloc(256 * sizeof(char*));
	char** args;
	int id;
	int size;
	BOOLEAN private;
	while (fgets(commandLine, 255, warehouseFile) != NULL){
		args = commandSplitter(commandLine, 3);
		if (args && (args+1) && (args+2)){
			id = atoi(*args);
			size = atoi(*(args+1));
			private = atoi(*(args+2));
			insertWarehouse( createWarehouse(id, size), private);
		}
		if (args)
			free(args);
	}
	free(commandLine);
}

/***********************************************************************************************/

/*
 * freeWarehouse()
 * Frees the memory allocated to the specified warehouse and its dependencies if any
 *
 * Params:
 * 	warehouse
 * 	pointer to the warehouse to be free()d
 *
 * Return:
 * 	void
 */
void freeWarehouse(struct warehouse* warehouse){
	if (warehouse->art_collection){
		if (warehouse->art_collection->name)
			free(warehouse->art_collection->name);
		free(warehouse->art_collection);
	}
	free(warehouse);
}

/*
 * freeWarehouseList()
 * frees the memory allocated to a warehouse list member and calls freeWarehouse() on its encompassed warehouse
 *
 * Params:
 * 	wl
 * 	pointer to the warehouse list to be free()d
 *
 * Return:
 * 	void
 */
void freeWarehouseList(struct warehouse_list* wl){
	if (wl->warehouse)
		freeWarehouse(wl->warehouse);
	free(wl);
}

/*
 * freeAllWarehouseList()
 * frees all the warehouse of the same class size
 *
 * Params:
 * 	warehouse_list_head 
 * 	head of the list of the class size to be freed
 *
 * Return:
 * 	void
 */
void freeAllWarehouseList(struct warehouse_list* warehouse_list_head){
	struct warehouse_list* cursor = warehouse_list_head;
	struct warehouse_list* temp;
	while (cursor){
		temp = cursor;
		cursor = cursor->next_warehouse;
		freeWarehouseList(temp);
	}
}

/*
 * freeAllWarehouseSFList()
 * frees the entirety of the Segregated List including all dependacies (warehouse_lists, warehouses, art_collections)
 *
 * Params:
 * 	void
 * 
 * Return:
 * 	void
 */
void freeAllWarehouseSFList(){
	if (!sf_head)
		return;
	struct warehouse_sf_list* cursor = sf_head;
	struct warehouse_sf_list* temp;
	while (cursor){
		freeAllWarehouseList(cursor->warehouse_list_head);
		temp = cursor;
		cursor = cursor->sf_next_warehouse;
		free(temp);
	}
}


/***********************************************************************************************/

/* coalesce()
 * When emptying a warehouse, this checks if the surrounding warehouses are also empty and of the same type (private/public)
 * If so, it is coalesced with the ones which match that criteria
 *
 * Params:
 * 	sf
 * 	the member of the sf list of which the warehouses are apart (so we need not iterate through the list again)
 *
 * 	wl_prev_prev
 * 	the member (if any) of the sf list 2 back from the emptying warehouse (since the database is singly linked)
 *	provided so if wl_prev is coalesced, it's next_warehouse can update accordingly
 *
 * 	wl_prev
 * 	the member (if any) of the sf list behind the emptying warehouse (since the database is singly linked)
 *
 * 	wl
 * 	the emptying warehouse
 *
 * Return:
 * 	void
 */
void coalesce(struct warehouse_sf_list* sf, struct warehouse_list* wl_prev_prev, struct warehouse_list* wl_prev, struct warehouse_list* wl){

	if ((wl_prev) && !(wl_prev->meta_info & 2) && !((wl->meta_info & 1) ^ (wl_prev->meta_info & 1))){
		
		if ((wl->next_warehouse) && !(wl->next_warehouse->meta_info & 2) && !((wl->meta_info & 1) ^ (wl->next_warehouse->meta_info & 1))){
			if (wl_prev_prev){
				wl_prev_prev->next_warehouse = wl->next_warehouse->next_warehouse;
			}
			else{
				sf->warehouse_list_head = wl->next_warehouse->next_warehouse;
			}
			insertWarehouse( createWarehouse( wl->warehouse->id, wl->warehouse->size * 3), wl->meta_info & 1);
			freeWarehouseList(wl->next_warehouse);
			freeWarehouseList(wl);
			freeWarehouseList(wl_prev);
		}
		else {
			if (wl_prev_prev){
				wl_prev_prev->next_warehouse = wl->next_warehouse;
			}
			else{
				sf->warehouse_list_head = wl->next_warehouse;
			}
			insertWarehouse( createWarehouse( wl->warehouse->id, wl->warehouse->size * 2), wl->meta_info & 1);
			freeWarehouseList(wl);
			freeWarehouseList(wl_prev);
		}
	}
	else{
		if ((wl->next_warehouse) && !(wl->next_warehouse->meta_info & 2) && !((wl->meta_info & 1) ^ (wl->next_warehouse->meta_info & 1))){
			if (wl_prev){
				wl_prev->next_warehouse = wl->next_warehouse->next_warehouse;
			}
			else{
				sf->warehouse_list_head = wl->next_warehouse->next_warehouse;
			}
			insertWarehouse( createWarehouse( wl->warehouse->id, wl->warehouse->size * 2), wl->meta_info & 1);
			freeWarehouseList(wl->next_warehouse);
			freeWarehouseList(wl);
		}
	}
}

/*
 * emptyWarehouse()
 * changes the emptying warehouse's allocated bit to 0 and calls coalesce()
 *
 * Params:
 * 	sf, wl_prev_prev, wl_prev
 * 	for calling coalesce(), since they are found in the calling function
 *
 * 	wl
 * 	the warehouse to be emptied
 *
 * Return:
 * 	void
 */
void emptyWarehouse(struct warehouse_sf_list* sf, struct warehouse_list* wl_prev_prev, struct warehouse_list* wl_prev, struct warehouse_list* wl){
	wl->meta_info = wl->meta_info & -3;
	coalesce(sf, wl_prev_prev, wl_prev, wl);
}

/***********************************************************************************************/

/*
 * printUtilization()
 * prints two ratios
 * 	the ratio of occupied warehouses to the total number of warehouses 
 * 	the ratio of the total size of all art collections and the total capacity of all warehouses
 *
 * Params:
 * 	void
 *
 * Return:
 * 	void
 */
void printUtilization(){
	struct warehouse_sf_list* sf_cursor = sf_head;
	struct warehouse_list* wl_cursor;
	double numOccupied = 0;
	double totalWarehouse = 0;
	double totalArtSize = 0;
	double warehouseCapacity = 0;
	while(sf_cursor){
		wl_cursor = sf_cursor->warehouse_list_head;
		while(wl_cursor){
			totalWarehouse++;
			warehouseCapacity += wl_cursor->warehouse->size;
			if (wl_cursor->meta_info & 2){
				numOccupied++;
				totalArtSize += wl_cursor->warehouse->art_collection->size;
			}
			wl_cursor = wl_cursor->next_warehouse;
		}
		sf_cursor = sf_cursor->sf_next_warehouse;
	}
	printf("%f\n", numOccupied/totalWarehouse);
	printf("%f\n", totalArtSize/warehouseCapacity);
}
