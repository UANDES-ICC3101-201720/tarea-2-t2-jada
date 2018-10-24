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

int *tabla_marcos//guardará bits de ocupación;
int nframes;//numero de marcos
int pages_left =0;
int diskwrite =0;
int diskread = 0;
int npages;//numero de paginas
struct disk *disk;
char *physmem;
const char *metodo;//guardara el metodo ingresado por instrucciones de ejecución

struct index{
	int frame;
	int page;
	struct index *sgte;};//paginas indexadas

struct lista{
	struct index *cabeza;};//lista, unico atributo, nuestra primera entrada.Elegí una lista ligada para hacer nuestra organizacion 
				//de los marcos, ya que así, es más fácil implementar el método FIFO, ya que la cabeza , el unico atributo
				//de esta lista, nos hará simplificar el código mucho más(ahorrandonos contadores de tiempo por ejemplo.[para 					//elegir el más antiguo])

struct lista *table_index;

void add_index(int frame, int page, struct index *inicio){// para añadir entradas a  nuestra tabla
	
	struct index *next = inicio;
	if(next!=0){
		while(next->sgte != 0){
			next = next->sgte;}
		}
	next->sgte = malloc(sizeof(struct index));
	next = next->sgte;
	next->frame = frame;
	next->page= page;
	
	}



void page_fault_handler_propio( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	//exit(1);
}
void page_fault_handler_fifo( struct page_table *pt, int page )
{
	int frame = table_index->cabeza->frame;//obtiene los datos del FI(fIRST IN)
	int page2 = table_index->cabeza->page;//obtiene los datos del FI(First in)
	
	table_index->cabeza = table_index->cabeza->sgte;//setea un nuevo FI a la tabla.

	add_index(frame,page,table_index->cabeza);//agrega esta cabeza formalmente a la lista.

	disk_write(disk,page2, &physmem[frame * PAGE_SIZE]);//escribe nueva pagina en disco
	diskwrite++;
	disk_read(disk,page, &physmem[frame * PAGE_SIZE]);//lee pagina antigua para hacer swapping
	diskread++;
	page_table_set_entry(pt,page2, frame, 0);//realiza swap
	page_table_set_entry(pt,page, frame, PROT_READ | PROT_WRITE | PROT_EXEC );//swap.
	
}
void page_fault_handler_random( struct page_table *pt, int page )
{
	int rand_frame = lrand48() % nframes;//elige un marco random
	int page2 ;//pagina que corresponde al marco random
	struct index *aux =table_index->cabeza;
	while(aux!=NULL){
		if(aux->frame == rand_frame){
			page2 = aux->page;}
		aux = aux->sgte;}
	
	disk_write(disk,page2,&physmem[rand_frame * PAGE_SIZE]);//escribe nueva pagina en disco
	diskwrite++;
	disk_read(disk,page,&physmem[rand_frame *PAGE_SIZE]);//lee antigua
	diskread++;
	struct index *aux2 = table_index->cabeza;
	while(aux2!= NULL){
		if(aux2->frame== rand_frame){
			aux2->page = page;
			break;}
		aux2 = aux2->sgte;}//seteamos nueva pagina en el marco elegido anteriormente.

	page_table_set_entry(pt,page2,rand_frame,0);//swap
	page_table_set_entry(pt,page,rand_frame, PROT_READ|PROT_WRITE|PROT_EXEC);//swap
}

void page_fault_handler(struct page_table *pt, int page){
	//creando tabla -->
	int auxmetodo = 1;
	pages_left++;//veces que ocurrio falta de paginas
	for(int i = 0; i<nframes;i++){
		if(tabla_marcos[i]!=0){//marco ocupado, 1 marco desocupado;--> busca marcos desocupados(bit!=0)
			//cuando lo encuentra, se ejecuta esto de abajo-->
			disk_read(disk,page,&physmem[i* PAGE_SIZE]);
			diskread++;
			page_table_set_entry(pt,page,i, PROT_READ|PROT_WRITE|PROT_EXEC);
			tabla_marcos[i]= 0;//y deja el marco como ocupado(esta lista sólo es de referencia)la verdadera tabla de marcos es *table_index;
			if(i==0){//--> por si tenemos nuestra table_index vacia.
				table_index->cabeza->frame = 0;
				table_index->cabeza->page = page;
				auxmetodo = 0 ;
				break;}
			add_index(i,page,table_index->cabeza);
			auxmetodo = 0;
			break;
		}
	}
	if (auxmetodo == 1){//-> acá según el método ingresado en instrucciones de ejecución, se ejecuta el método de paginación requerido.
		int aux2=1;
		if(strcmp(metodo,"rand")==0){
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
	table_index = malloc(sizeof(struct lista));//iniciamos tabla de marcos
	table_index->cabeza = malloc(sizeof(struct index));
	
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <rand|fifo> <sort|scan|focus>\n");
		return 1;
	}

	npages = atoi(argv[1]);//guardamos variables de instrucciones de ejecución.
	nframes = atoi(argv[2]);
	metodo = argv[3];
	const char *program = argv[4];

	int auxiliar[nframes];
	tabla_marcos = auxiliar;
	for(int d=0;d<nframes;d++){
		tabla_marcos[d]=d;}//le damos valores random a la tabla de marcos para mostrar que están disponibles

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	
	
	struct page_table *pt;
	
	pt = page_table_create(npages,nframes,page_fault_handler);

	char *virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);
	
	for(int j =0; j<nframes;j++){
		disk_write(disk,j,&physmem[j* BLOCK_SIZE]);}
	
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}
	
	
	printf("%i diskread \n",diskread);
	printf("%i diskwrite \n",diskwrite);
	printf("%i pages left \n",pages_left);
	
	free(table_index);
	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
