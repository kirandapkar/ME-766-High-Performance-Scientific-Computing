#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <mpi.h>
#include <omp.h>
#include <time.h>

#define TAG_HYP     1
#define TAG_EXIT    2
#define TAG_ASK_JOB 3

#define ROW(i) i/m_size
#define COL(i) i%m_size
#define BOX(row, col) r_size*(row/r_size)+col/r_size

#define BLOCK_LOW(id, p, n) ((id)*(n)/(p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id)+1,p,n)-1)


typedef struct{
    int cell;
    int num;
}Item;

typedef struct ListNode{
    Item this;
    struct ListNode *next;
    struct ListNode *prev;
}ListNode;

typedef struct{
    ListNode *head;
    ListNode *tail;
    int len;
}List;

List* init_list(void){
    List* newList = (List*)malloc(sizeof(List));
    newList -> head = NULL;
    newList -> tail = NULL;
    newList->len = 0;

    return newList;
}

ListNode* newNode(Item this){
    ListNode* new_node = (ListNode*) malloc(sizeof(ListNode));

    new_node -> this = this;
    new_node -> next = NULL;

    return new_node;
}

void insert_head(List* list, Item this){
    ListNode* node = newNode(this);

    if (list->len) {
        node->next = list->head;
        node->prev = NULL;
        list->head->prev = node;
        list->head = node;
    }else {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    }

    ++list->len;
}

Item pop_head(List* list){
    Item item;

    ListNode* node = list->head;

    if (--list->len)
        (list->head = node->next)->prev = NULL;
    else
        list->head = list->tail = NULL;

    node->next = node->prev = NULL;

    item = node -> this;
    free(node);
    return item;
}

Item pop_tail(List *list) {
    Item item;

    ListNode *node = list->tail;
  
    if (--list->len)
        (list->tail = node->prev)->next = NULL;
    else
        list->tail = list->head = NULL;

    node->next = node->prev = NULL;

    item = node-> this;
    free(node);
    return item;
}

int r_size, m_size, v_size, id, p;

void print_sudoku(int *sudoku) {
    int i;
    for (i = 0; i < v_size; i++) {
        if(i%m_size != m_size - 1)
            printf("%2d ", sudoku[i]);
        else
            printf("%2d\n", sudoku[i]);
    }
}


//initialize a mask
int new_mask(int size) {
    return (0 << (size-1));
}


int int_to_mask(int num) {
    return (1 << (num-1));
}


void update_masks(int num, int row, int col, uint64_t* rows_mask, uint64_t* cols_mask, uint64_t* boxes_mask) {
    int new_mask = int_to_mask(num); //convert number found to mask ex: if dim=4x4, 3 = 0010
    rows_mask[row] |= new_mask;      
    cols_mask[col] |= new_mask;     
    boxes_mask[BOX(row, col)] |= new_mask;
}

//initialize the masks from the sudoku read from the file
void init_masks(int* sudoku, uint64_t* rows_mask, uint64_t* cols_mask, uint64_t* boxes_mask) {
    int i;

    #pragma omp parallel for
    for(i = 0; i < m_size; i++){
        rows_mask[i]  = 0;
        cols_mask[i]  = 0;
        boxes_mask[i] = 0;
    }

    for(i = 0; i < v_size; i++)
        if(sudoku[i])
            update_masks(sudoku[i], ROW(i), COL(i), rows_mask, cols_mask, boxes_mask);
}


void delete_from(int* sudoku, int *cp_sudoku, uint64_t* rows_mask, uint64_t* cols_mask, uint64_t* boxes_mask, int cell){
    int i;
    init_masks(sudoku, rows_mask, cols_mask, boxes_mask);
    
    //clear all work done by the sender process
    #pragma omp parallel for
    for(i = v_size; i >= cell; i--)
        if(cp_sudoku[i] > 0)
            cp_sudoku[i] = 0;

    //initialize the masks from the cp_sudoku received
    for(i = 0; i < cell; i++)
        if(cp_sudoku[i] > 0)
            update_masks(cp_sudoku[i], ROW(i), COL(i), rows_mask, cols_mask, boxes_mask);
}

Item invalid_hyp(void){
    Item item;
    item.cell = -1;
    item.num = -1;
    return item;
}

void send_ring(void *msg, int tag, int dest){
    int msg_send[2];
    msg_send[0] =*((int*) msg);
    msg_send[1] = dest;
    
    if(id == p-1)
        MPI_Send(msg_send, 2, MPI_INT, 0, tag, MPI_COMM_WORLD);
    else
        MPI_Send(msg_send, 2, MPI_INT, id+1, tag, MPI_COMM_WORLD);
}

int exists_in(int index, uint64_t* mask, int num) {
    int res, masked_num = int_to_mask(num);

    res = mask[index] | masked_num;  
    if(res != mask[index])         
        return 0;
    return 1;
}

int is_safe_num(uint64_t* rows_mask, uint64_t* cols_mask, uint64_t* boxes_mask, int row, int col, int num) {
    return !exists_in(row, rows_mask, num) && !exists_in(col, cols_mask, num) && !exists_in(BOX(row, col), boxes_mask, num);
}

void rm_num_masks(int num, int row, int col, uint64_t* rows_mask, uint64_t* cols_mask, uint64_t* boxes_mask) {
    int num_mask = int_to_mask(num);
    rows_mask[row] ^= num_mask;
    cols_mask[col] ^= num_mask;
    boxes_mask[BOX(row, col)] ^= num_mask;
}

int solve_from(int* sudoku, int* cp_sudoku, uint64_t* rows_mask, uint64_t* cols_mask, uint64_t* boxes_mask, List* work, int last_pos){
    int i, cell, val, number_amount, flag = 0, no_sol_count, len;
    
    MPI_Request request;
    MPI_Status status;
    Item hyp, no_hyp = invalid_hyp();
    
    //a while loop to get work from other processes when the list gets empty
    while(1){

        //a while loop to get work from the work list
        while(work->head != NULL){

            hyp = pop_head(work);
            len = work->len;
            int start_pos = hyp.cell;

            if(!is_safe_num(rows_mask, cols_mask, boxes_mask, ROW(hyp.cell), COL(hyp.cell), hyp.num))
                continue;

            //a while loop to solve the sudoku
            while(1){

                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
                
                if(flag && status.MPI_TAG != -1){
                    flag = 0;

                    MPI_Get_count(&status, MPI_INT, &number_amount);
                    int* number_buf = (int*)malloc(number_amount * sizeof(int));

                    MPI_Recv(number_buf, number_amount, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    
                    if(status.MPI_TAG == TAG_EXIT){
                        send_ring(&id, TAG_EXIT, -1);
                        return 0;

                    }else if(status.MPI_TAG == TAG_ASK_JOB){

                        if(work->tail != NULL){

                            Item hyp_send = pop_tail(work);

                            int* send_msg = (int*)malloc((v_size+2)*sizeof(int));
                            memcpy(send_msg, &hyp_send, sizeof(Item));
                            memcpy((send_msg+2), cp_sudoku, v_size*sizeof(int));

                            MPI_Send(send_msg, (v_size+2), MPI_INT, status.MPI_SOURCE, TAG_HYP, MPI_COMM_WORLD);
                            free(send_msg);
                        
                        }else
                            MPI_Send(&no_hyp, 2, MPI_INT, status.MPI_SOURCE, TAG_HYP, MPI_COMM_WORLD);
                    }
                    free(number_buf);
                }
            
                update_masks(hyp.num, ROW(hyp.cell), COL(hyp.cell), rows_mask, cols_mask, boxes_mask);
                cp_sudoku[hyp.cell] = hyp.num;
                
                //iterate cells of the sudoku
                for(cell = hyp.cell + 1; cell < v_size; cell++){
                    
                    //if the cell has an unchangeable number skip the cell
                    if(cp_sudoku[cell])
                        continue;
                    
                    //test numbers in the cell
                    for(val = m_size; val >= 1; val--){

                        //if the current number is not valid in this cell skip the number
                        if(is_safe_num(rows_mask, cols_mask, boxes_mask, ROW(cell), COL(cell), val)){
                            
                            //if the cell is the last one and a valid number for it was found the sudoku has been solved
                            if(cell == last_pos){
                                cp_sudoku[cell] = val;
                                send_ring(&id, TAG_EXIT, -1);
                                return 1;
                            }   
                            hyp.cell = cell;
                            hyp.num = val;
                            insert_head(work, hyp);
                        }       
                    }
                    break;
                }
                
                if(work->len == len){
                    for(cell = v_size - 1; cell >= start_pos; cell--)
                        if(cp_sudoku[cell] > 0){
                            rm_num_masks(cp_sudoku[cell],  ROW(cell), COL(cell), rows_mask, cols_mask, boxes_mask);
                            cp_sudoku[cell] = 0;
                        }
                    break;
                }
                
                hyp = pop_head(work);
                
                for(cell--; cell >= hyp.cell; cell--){
                    if(cp_sudoku[cell] > 0) {
                        rm_num_masks(cp_sudoku[cell],  ROW(cell), COL(cell), rows_mask, cols_mask, boxes_mask);
                        cp_sudoku[cell] = 0;
                    }   
                }
            }
        }

        no_sol_count = 0;
        
        if(p == 1)
            return 0;

        //cycle to ask other processes for work
        for(i = id+1;; i++){

            if(i == p) i = 0;
            if(i == id) continue;

            //send a work request message to the ith process
            MPI_Send(&i, 1, MPI_INT, i, TAG_ASK_JOB, MPI_COMM_WORLD);
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_INT, &number_amount);
            int* number_buf = (int*)malloc(number_amount * sizeof(int));
            
            MPI_Recv(number_buf, number_amount, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            
            if(status.MPI_TAG == TAG_HYP && number_amount != 2){
                Item hyp_recv;
                memcpy(&hyp_recv, number_buf, sizeof(Item));
                memcpy(cp_sudoku, (number_buf+2), v_size*sizeof(int));
                
                delete_from(sudoku, cp_sudoku, rows_mask, cols_mask, boxes_mask, hyp_recv.cell);                
                insert_head(work, hyp_recv);
                free(number_buf);
                break;

            }else if(status.MPI_TAG == TAG_HYP && number_amount == 2){
                no_sol_count++;

            }else if(status.MPI_TAG == TAG_EXIT){
                send_ring(&id, TAG_EXIT, -1);
                return 0;

            }else if(status.MPI_TAG == TAG_ASK_JOB){
                MPI_Send(&no_hyp, 2, MPI_INT, status.MPI_SOURCE, TAG_HYP, MPI_COMM_WORLD);
            }
            
            if(no_sol_count == p-1 && id == 0){
                send_ring(&id, TAG_EXIT, -1);
                return 0;
            }

            free(number_buf);
        }
    }
}


int solve(int* sudoku){
    int i, last_pos, flag_start = 0, solved = 0;
    Item hyp;
    
    uint64_t *r_mask_array = (uint64_t*) malloc(m_size * sizeof(uint64_t));
    uint64_t *c_mask_array = (uint64_t*) malloc(m_size * sizeof(uint64_t));
    uint64_t *b_mask_array = (uint64_t*) malloc(m_size * sizeof(uint64_t));
    int *cp_sudoku = (int*) malloc(v_size * sizeof(int));
    List *work = init_list();

    for(i = 0; i < v_size; i++) {
        if(sudoku[i])
            cp_sudoku[i] = -1;
        else{
            cp_sudoku[i] = 0;
            if(!flag_start){
                flag_start = 1;
                hyp.cell = i;
            }
            last_pos = i;
        }
    }

    init_masks(sudoku, r_mask_array, c_mask_array, b_mask_array);
    
    for(i = 1 + BLOCK_HIGH(id, p, m_size); i >= 1 + BLOCK_LOW(id, p, m_size); i--){
        hyp.num = i;
        insert_head(work, hyp);
    }

    // try to solve sudoku
    solved = solve_from(sudoku, cp_sudoku, r_mask_array, c_mask_array, b_mask_array, work, last_pos);

    if(solved){
        #pragma omp parallel for
        for(i = 0; i < v_size; i++)
            if(cp_sudoku[i] != -1)
                sudoku[i] = cp_sudoku[i];
    }
    
    free(work);
    free(r_mask_array);
    free(c_mask_array);
    free(b_mask_array);
    free(cp_sudoku);

    return solved;
}

int* read_matrix(char *argv[]) {
    FILE *fp;
    size_t characters, len = 1;
    char *line = NULL, aux[3];
    int i, j, k, l;

    if((fp = fopen(argv[1], "r+")) == NULL) {
        fprintf(stderr, "unable to open file %s\n", argv[1]);
        exit(1);
    }

    getline(&line, &len, fp);
    r_size = atoi(line);
    m_size = r_size *r_size;
    v_size = m_size * m_size;

    int* sudoku = (int*)malloc(v_size * sizeof(int));

    k = 0, l = 0;
    len = m_size * 2;
    for(i = 0; (characters = getline(&line, &len, fp)) != -1; i++){
        for (j = 0; j < characters; j++) {
            if(isdigit(line[j])){
                aux[l++] = line[j];
            }else if(l > 0){
                aux[l] = '\0';
                l = 0;
                sudoku[k++] = atoi(aux);
                memset(aux, 0, sizeof aux);
            }
        }
    }

    free(line);
    fclose(fp);

    return sudoku;
}


int main(int argc, char *argv[]){
    int* sudoku, result, total;
    clock_t start,time_taken;
    omp_set_num_threads(8);

    if(argc == 2){

        sudoku = read_matrix(argv);

        MPI_Init (&argc, &argv);
        MPI_Comm_rank (MPI_COMM_WORLD, &id);
        MPI_Comm_size (MPI_COMM_WORLD, &p);

        start =  clock();
        result = solve(sudoku);
        
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Allreduce(&result, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        
        if(!total && !id)
            printf("No solution\n");
        else if(total && result)
        {
        	printf("Solution: \n");
            print_sudoku(sudoku);
        }

        fflush(stdout);
        MPI_Finalize();
        
        time_taken = clock() - start;
        double total_time = (((double)time_taken)/CLOCKS_PER_SEC);  
        printf("time taken by %d = %f\n",id, total_time);
        
    }else
        printf("invalid input arguments.\n");
    free(sudoku);    
    return 0;
}