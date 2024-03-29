#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "warehouse.h"

/*
 * createArtCollection()
 * allocates memory for an art collection with a specified name, size, price
 *
 * Params:
 * 	name
 * 	string for the name of the art collection which will be copied to a new location and pointed to by the new art collection
 *
 * 	size
 * 	value for the smallest size warehouse that can fit this art collection
 *
 * 	price
 * 	value to measure worth of art collection
 *
 * Return:
 * 	pointer to newly malloc'd art collection struct
 */
struct art_collection* createArtCollection(char* name, int size, int price){
	struct art_collection* output = malloc(sizeof(struct art_collection));
	output->name = malloc(strlen(name)+1);
	strcpy(output->name, name);
	int i;
	for (i=0; i<strlen(output->name); i++)
		(output->name)[i] = tolower((output->name)[i]);
	output->size = size;
	output->price = price;
	return output;
}

/*
 * insertArtCollection
 * finds an empty, sizable warehouse to store the specified art collection, or reports the failure to the user
 *
 * Params:
 * 	art_collection
 * 	art collection to be stored
 *
 * Return:
 * 	void
 */
void insertArtCollection(struct art_collection* art_collection){
	if (!sf_head){
		printf("ERROR: There exist no warehouse in the database!\n");
		if (art_collection->name)
			free(art_collection->name);
		free(art_collection);
		return;
	}
	struct warehouse_sf_list* sf_cursor = sf_head;
	while (sf_cursor->class_size < art_collection->size){
		sf_cursor = sf_cursor->sf_next_warehouse;
		if (!sf_cursor){
			printf("ERROR: There exists no unoccupied warehouse large enough to fit Art Collection \"%s\".\n", art_collection->name);
			if (art_collection->name)
				free(art_collection->name);
			free(art_collection);
			return;
		}
	}
	struct warehouse_list* wl_cursor = sf_cursor->warehouse_list_head;
	struct warehouse_list* wl_prev = NULL;
	while (1){
		while (!wl_cursor){
			sf_cursor = sf_cursor->sf_next_warehouse;
			if (!sf_cursor){
				printf("ERROR: There exists no Warehouse large enough to fit Art Collection \"%s\".\n", art_collection->name);
				if (art_collection->name)
					free(art_collection->name);
				free(art_collection);
				return;
			}
			wl_cursor = sf_cursor->warehouse_list_head;
			wl_prev = NULL;
		}
		while(wl_cursor){
			if ((wl_cursor->meta_info) & 2){
				wl_prev = wl_cursor;
				wl_cursor = wl_cursor->next_warehouse;
			}
			else{
				int artSize = art_collection->size;
				if (art_collection->size % 2)
					artSize++;
				if (artSize < 4)
					artSize = 4;
				int newSize = sf_cursor->class_size - artSize;
				if (newSize < 4){
					wl_cursor->warehouse->art_collection = art_collection;
					wl_cursor->meta_info = wl_cursor->meta_info | 2;
					return;
				}
				else{
					if (wl_prev)
						wl_prev->next_warehouse = wl_cursor->next_warehouse;
					else 
						sf_cursor->warehouse_list_head = wl_cursor->next_warehouse;
					insertWarehouse(createWarehouse(	wl_cursor->warehouse->id, 	artSize),	wl_cursor->meta_info&1);
					insertWarehouse(createWarehouse(	nextGoodID(),			newSize),	wl_cursor->meta_info&1);
					freeWarehouseList(wl_cursor);
					insertArtCollection(art_collection);
					return;
				}
			}
		}
	}
}

extern BOOLEAN equals(char* s1, char* s2); //used in removeArtCollection defined in main.c

/*
 * removeArtCollection()
 * removes all instances of an art collection from the database
 *
 * Params:
 * 	name
 * 	name to be removed if matching
 *
 * 	count
 * 	variable to keep track of total matching names found, used in recursion, should always be 0 otherwise
 *
 * Return: 
 * 	void
 */
void removeArtCollection(char* name, int count){
	struct warehouse_sf_list* sf_cursor = sf_head;
	struct warehouse_list* wl_cursor;
	struct warehouse_list* wl_prev;
	struct warehouse_list* wl_prev_prev;
	while (sf_cursor){
		wl_prev_prev = NULL;
		wl_prev = NULL;
		wl_cursor = sf_cursor->warehouse_list_head;
		while (wl_cursor){
			if ((wl_cursor->meta_info & 2) && (equals(wl_cursor->warehouse->art_collection->name, name))){
				emptyWarehouse(sf_cursor, wl_prev_prev, wl_prev, wl_cursor);
				removeArtCollection(name, ++count);
				return;
			}
			wl_prev_prev = wl_prev;
			wl_prev = wl_cursor;
			//long int size = (wl_cursor->meta_info & -4) >> 1;
			//printf("\twarehouse %d\n\t\tof size %ld\n\t\t%s with %s in it\n", wl_cursor->warehouse->id, size, (wl_cursor->meta_info & 1)?"Private":"Public", (wl_cursor->meta_info & 2)? wl_cursor->warehouse->art_collection->name : "nothing");
			wl_cursor = wl_cursor->next_warehouse;
		}
		sf_cursor = sf_cursor->sf_next_warehouse;
	}
	if (count)
		printf("%d instance%s of %s found and deleted.\n", count, (count==1) ? "" : "s", name);
	else
		printf("No instances of %s found, nothing deleted.\n", name);
}

/*
 * loadArtFile()
 * creates and inserts art collections from file each specified by NAME SIZE PRICE\n
 *
 * Params:
 * 	artFile
 * 	pointer to the opened file to be read
 *
 * Return:
 * 	void
 */
void loadArtFile(FILE* artFile){
	char* commandLine = malloc(256 * sizeof(char*));
	char** args;
	char* name;
	int size;
	int price;
	while (fgets(commandLine, 255, artFile) != NULL){
		args = commandSplitter(commandLine, 3);
		if (args && (args+1) && (args+2)){
			name = *args;
			size = atoi(*(args+1));
			price = atoi(*(args+2));
			insertArtCollection( createArtCollection(name, size, price));
		}
		if (args)
			free(args);
	}
	free(commandLine);
}

/*
 * printArtCollection()
 * prints the info of the specified art collection to stdout
 *
 * Params:
 * 	artC
 * 	the art collection to be printed
 *
 * Return:
 * 	void
 */
void printArtCollection(struct art_collection* artC){
	printf("%s %d %d\n", artC->name, artC->size,  artC->price);
}

/*
 * printUnsorted()
 * prints the info of all art collections of the database to stdout
 *
 * Params:
 * 	all
 * 	TRUE if all art collections are to be printed,  FALSE otherwise
 *
 * 	private
 * 	TRUE if art collections in private warehouses are to be printed
 * 	FALSE if art collections in public warehouses are to be printed
 * 	overridden by all param
 *
 * Return:
 * 	void
 */
void printUnsorted(BOOLEAN all, BOOLEAN private){
	struct warehouse_sf_list* sf_cursor = sf_head;
	struct warehouse_list* wl_cursor;
	int total = 0;
	while (sf_cursor){
		wl_cursor = sf_cursor->warehouse_list_head;
		while(wl_cursor){
			if (((wl_cursor->meta_info) & 2) && (all  || !((wl_cursor->meta_info & 1) ^ private))){
				printArtCollection( wl_cursor->warehouse->art_collection);
				total += wl_cursor->warehouse->art_collection->price;
			}
			wl_cursor = wl_cursor->next_warehouse;
		}
		sf_cursor = sf_cursor->sf_next_warehouse;
	}
	printf("%d\n", total);
}

/*
 * art_collection_list
 * Data Structure to help sort art collections for printing
 */
struct art_collection_list{
	struct art_collection* art_collection;
	struct art_collection_list* next;
};

/*
 * printAndFreeACL()
 * once the art collection list is made and sorted, this function prints all of them and frees the art collection list wrapper (not the art collection itself)
 *
 * Params:
 * 	acl_cursor
 * 	head of the art collection list
 *
 * Return:
 * 	void
 */
void printAndFreeACL(struct art_collection_list* acl_cursor){
	struct art_collection_list* acl_prev;
	int total = 0;
	while (acl_cursor){
		printArtCollection(acl_cursor->art_collection);
		acl_prev = acl_cursor;
		free(acl_prev);
		acl_cursor = acl_cursor->next;
	}
	printf("%d\n", total);
}

/*
 * printBySize()
 * creates the acl data structure sorted by size and calls printAndFreeACL()
 *
 * Params:
 * 	all
 * 	TRUE if all art collections are to be printed,  FALSE otherwise
 *
 * 	private
 * 	TRUE if art collections in private warehouses are to be printed
 * 	FALSE if art collections in public warehouses are to be printed
 * 	overridden by all param
 *
 * Return:
 * 	void
 */
void printBySize(BOOLEAN all, BOOLEAN private){
	struct warehouse_sf_list* sf_cursor = sf_head;
	struct warehouse_list* wl_cursor;
	struct art_collection_list* acl_head = NULL;
	struct art_collection_list* acl_cursor = NULL;
	struct art_collection_list* acl_prev;
	struct art_collection_list* acl_new;
	while (sf_cursor){
		wl_cursor = sf_cursor->warehouse_list_head;
		while(wl_cursor){
			if (((wl_cursor->meta_info) & 2) && (all || !((wl_cursor->meta_info & 1) ^ private))){
				if (acl_head){
					if (acl_head->art_collection->size > wl_cursor->warehouse->art_collection->size){
						acl_new = malloc(sizeof(struct art_collection_list));
						acl_new->art_collection = wl_cursor->warehouse->art_collection;
						acl_new->next = acl_head;
						acl_head = acl_new;
					}
					else{
						acl_cursor = acl_head->next;
						acl_prev = acl_head;
						while (acl_cursor){
							if (acl_cursor->art_collection->size > wl_cursor->warehouse->art_collection->size){
								acl_new = malloc(sizeof(struct art_collection_list));
								acl_new->art_collection = wl_cursor->warehouse->art_collection;
								acl_new->next = acl_cursor;
								acl_prev->next = acl_new;
								break;
							}
							acl_prev = acl_cursor;
							acl_cursor = acl_cursor->next;
						}
						if (!acl_cursor){ 
							acl_new = malloc(sizeof(struct art_collection_list));
							acl_new->art_collection = wl_cursor->warehouse->art_collection;
							acl_new->next = NULL;
							acl_prev->next = acl_new;
						}
					}
				}
				else{
					acl_head = malloc(sizeof(struct art_collection_list));
					acl_head->art_collection = wl_cursor->warehouse->art_collection;
					acl_head->next = NULL;
				}
			}
		wl_cursor = wl_cursor->next_warehouse;
		}
	sf_cursor = sf_cursor->sf_next_warehouse;
	}
	printAndFreeACL(acl_head);
}

/*
 * printByPrice()
 * creates the acl data structure sorted by price and calls printAndFreeACL()
 *
 * Params:
 * 	all
 * 	TRUE if all art collections are to be printed,  FALSE otherwise
 *
 * 	private
 * 	TRUE if art collections in private warehouses are to be printed
 * 	FALSE if art collections in public warehouses are to be printed
 * 	overridden by all param
 *
 * Return:
 * 	void
 */
void printByPrice(BOOLEAN all, BOOLEAN private){	
	struct warehouse_sf_list* sf_cursor = sf_head;
	struct warehouse_list* wl_cursor;
	struct art_collection_list* acl_head = NULL;
	struct art_collection_list* acl_cursor = NULL;
	struct art_collection_list* acl_prev;
	struct art_collection_list* acl_new;
	while (sf_cursor){
		wl_cursor = sf_cursor->warehouse_list_head;
		while(wl_cursor){
			if (((wl_cursor->meta_info) & 2) && (all || !((wl_cursor->meta_info) ^ private))){
				if (acl_head){
					if (acl_head->art_collection->price > wl_cursor->warehouse->art_collection->price){
						acl_new = malloc(sizeof(struct art_collection_list));
						acl_new->art_collection = wl_cursor->warehouse->art_collection;
						acl_new->next = acl_head;
						acl_head = acl_new;
					}
					else{
						acl_cursor = acl_head->next;
						acl_prev = acl_head;
						while (acl_cursor){
							if (acl_cursor->art_collection->price > wl_cursor->warehouse->art_collection->price){
								acl_new = malloc(sizeof(struct art_collection_list));
								acl_new->art_collection = wl_cursor->warehouse->art_collection;
								acl_new->next = acl_cursor;
								acl_prev->next = acl_new;
								break;
							}
							acl_prev = acl_cursor;
							acl_cursor = acl_cursor->next;
						}
						if (!acl_cursor){ 
							acl_new = malloc(sizeof(struct art_collection_list));
							acl_new->art_collection = wl_cursor->warehouse->art_collection;
							acl_new->next = NULL;
							acl_prev->next = acl_new;
						}
					}
				}
				else{
					acl_head = malloc(sizeof(struct art_collection_list));
					acl_head->art_collection = wl_cursor->warehouse->art_collection;
					acl_head->next = NULL;
				}
			}
		wl_cursor = wl_cursor->next_warehouse;
		}
	sf_cursor = sf_cursor->sf_next_warehouse;
	}
	printAndFreeACL(acl_head);
}
