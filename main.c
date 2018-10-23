/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int *tabla_marcos;
int npages,nframes;
struct disk *disk;
char *physmem;
int nframes;
const char *metodo;

struct index{
	int frame;
	int page;
	struct index *sgte;};

struct lista{
	struct index *cabeza;};

struct lista *table_index;

void add_index(int frame, int page, struct index *cabeza,struct index *next){
	next = cabeza;
	if(next!=NULL){
		while(next->sgte != NULL){
			next = next->sgte;}
		}
	next->sgte = malloc(sizeof(struct index));
	next = next->sgte;
	next->frame = frame;
	next->page = page;
	}


void page_fault_handler_propio( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	//exit(1);
}
void page_fault_handler_fifo( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	//exit(1);
}
void page_fault_handler_random( struct page_table *pt, int page )
{
	int nframe = page_table_get_nframes(pt);
	int rand_frame = lrand48() % nframe;
	int page2;
	fprintf(stderr,"entro");
	struct index *aux = table_index->cabeza;//no retorna nada, arreglar, falta inicializar table_index antes de esto.
	
	while(aux->sgte!= NULL){
		if(aux->frame==rand_frame){
			page2 = aux->page;
			
			break;}
		aux = aux->sgte;}
	disk_write(disk,page2,&physmem[rand_frame * PAGE_SIZE]);
	disk_read(disk,page,&physmem[rand_frame *PAGE_SIZE]);
	
	struct index *aux2 = table_index->cabeza;
	while(aux2->sgte!= NULL){
		if(aux2->frame==rand_frame){
			aux2->page = page;;
			break;}
		aux2 = aux2->sgte;}
	page_table_set_entry(pt,page2,rand_frame,0);
	page_table_set_entry(pt,page,rand_frame, PROT_READ|PROT_WRITE|PROT_EXEC);
}

void page_fault_handler(struct page_table *pt, int page){
	//creando tabla de marcos-->
	int auxmetodo = 1;
	for(int i = 0; i<nframes;i++){
		if(tabla_marcos[i]!=0){//marco ocupado, 1 marco desocupado;
			disk_read(disk,page,&physmem[i * PAGE_SIZE]);
			page_table_set_entry(pt,page,i, PROT_READ|PROT_WRITE|PROT_EXEC);
			tabla_marcos[i]= 0;
			struct index *aux1 = table_index->cabeza;
			if(i==0){
				table_index->cabeza->frame = 0;
				table_index->cabeza->page = page;
				auxmetodo=0;
				break;
				}
			add_index(i,page,table_index->cabeza,aux1);
			auxmetodo = 0;
			break;
		}
	}
	if (auxmetodo == 1){
		int aux2=1;
		if(strcmp(metodo,"random")==0){
			printf("entro");
			page_fault_handler_random(pt,page);
			aux2=2;}
		if(strcmp(metodo,"fifo")==0){
			page_fault_handler_fifo(pt,page);
			aux2=2;}
		if(strcmp(metodo,"propio")==0){
			page_fault_handler_propio(pt,page);
			aux2=2;}
		if(aux2==1){
		printf("método %s desconocido",metodo);
		exit(1);}
	}
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <lru|fifo> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	metodo = argv[3];
	const char *program = argv[4];

	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	
	printf("entro 3");
	int m[nframes];
	tabla_marcos = m;

	for(int d=0;d<nframes;d++){
		tabla_marcos[d]=1;}

	printf("entro2");
	struct page_table *pt;
	
	pt = page_table_create(npages,nframes,page_fault_handler);

	char *virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);
	
	for(int j =0; j<nframes;j++){
		disk_write(disk,j,&physmem[j* BLOCK_SIZE]);}
	printf("entro1");

	
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}
	
	page_table_print(pt);

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
