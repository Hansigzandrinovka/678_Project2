/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
	
	q->q_front = NULL;//front end reference pointer to the entire q

	q->q_size = 0;//the size of the q
	
	q->comparer = comparer;//the q's coomparer
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	//create new node
	node *newNode = malloc(sizeof(node));
	node *temp = q->q_front;//used for traversal
	//set teh new node's values for val and nxt	
	newNode -> val = ptr; 
	newNode -> nxt = NULL;
	int thisIndex = 1;//the index to be retuned at end if not 0	
	
	
	//if q is empty
	if(q->q_size == 0){
		q->q_front = newNode;	
	}
	//if we are replacing new node as the new front
	else if(q->comparer(ptr, q->q_front->val)<0){
			
		newNode -> nxt = q->q_front;
		q->q_front = newNode;
	}
	//placing it somewhere else			
	else{
		while(temp -> nxt != NULL){//traverse till the last node				
			//traverse further and icrement thisIndex if we... 
			//havent foudn newNodes proper place yet
			if(q->comparer(ptr, temp ->nxt->val) < 0 ){
				break;//break when we find the index to place our new node
			}
			else{	
				temp = temp -> nxt;
				thisIndex++;
			}
		}
		//update new nodes nxt and temps nxt to avoid seg faults
		newNode -> nxt = temp -> nxt;
		temp -> nxt = newNode;
		q->q_size++;
		return(thisIndex);//returning the index new node is placed at
	}
	q->q_size++;
	return 0;//because newNode was placed at front
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	if(q->q_size !=0){//if our q is not empty...
		return q-> q_front->val;//...return the front value
	}
	else{//if our q is empty...
		return NULL;//...return null
	}
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	if(q->q_size !=0){//if our q is not empty....
		void* tempvalue = q->q_front->val;
		node* temp = q->q_front;

		if(q->q_size == 1){
			q->q_front = NULL;//if there was only one item in q, new front is Null	
		}
		else{
			q->q_front = q->q_front->nxt;//update q_front and q_size
		}
		q->q_size--;		
		free(temp);//free the old q_front		
		
		return tempvalue;//return the old q_front's value
	}
	else{//if our q is empty...
		return NULL;//...return null
	}
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	
	if(index > q->q_size-1 || index < 0){
		return NULL;//this index does not exist
	}
	else{
		int i = 0;
		node* temp = q->q_front;
		while(i != index){//traverse until i == index, which means temp will be at wanted index
			temp = temp->nxt;
			i++;
		}
		return temp-> val;//returning value of the wanted index'th element	
	}
		
}


/**
  Removes all instances of ptr from the queue. 
  
  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{	
	int removed = 0;//how many nodes have been removed	
	if(q->q_front != NULL){
		node *temp = q->q_front;//the traversing node pointer
		while(temp ->nxt != NULL){
			if(q->q_front->val ==ptr){
				node *temp2 = q->q_front;
				q->q_front = q->q_front->nxt;//update q_front
				
				temp = q->q_front;//keep temp at front
				
				removed ++;//update number of removed
				free (temp2);//delete temp2's node
			}
			else if(temp->nxt->val == ptr){
				node *temp2 = temp->nxt;
				temp->nxt = temp->nxt->nxt;//update the nxt pointer	
				
				removed++;
				free(temp2);
			}
			else{
				temp = temp->nxt;//traverse
			}
		}
		q->q_size = q->q_size - removed;//q size decremented by number of removed	
		return removed;
	}
	else{
		return removed;//which is 0
	}
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{

	int i = 0;
	void *tempValue = q->q_front->val;
	node *temp = q->q_front;
	node *temp2 = q->q_front->nxt;	

	if(index > q->q_size-1 || index<0 || q->q_front == NULL){
		return NULL;//this index does not exist, or the q does not exist
	}
	else if(index == 0){			
		q->q_front = q->q_front->nxt;//shift front back 1 node
		free(temp);
	}
	else{
		while(i != (index-1)){//used to traverse the queue until we get to the intended index's node		
			temp = temp2;
			temp2 = temp2->nxt;
			i++;			
		}
		
		temp -> nxt = temp2 -> nxt;//make sure the queue doesnt break when we remove temp2 element
		tempValue = temp2->val;
		free(temp2);
	}
	q->q_size--;
	return tempValue;
}


/**
  Returns the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	
	return q->q_size;//returning the size of q
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	while(q->q_size != 0){
		node *temp = q->q_front;
		q->q_front = q->q_front->nxt;
		free(temp);
		q->q_size --;
	}
	
}








