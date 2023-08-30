/*
 * KnfcList.c
 *
 *  Created on: Jun 26, 2013
 *      Author: youngsun
 */

#include "KnfcList.h"

KnfcNode_t *
KnfcNewList(void) {

	KnfcNode_t *headNode;

	headNode = (KnfcNode_t*)CMemoryAlloc(sizeof(KnfcNode_t));

	if(headNode != NULL) {
		headNode->pNext = NULL;
		headNode->pPrev = NULL;
		headNode->pItem = NULL;
	}

	return headNode;
}

KnfcNode_t *
KnfcInsertNode(KnfcNode_t *pHeadNode, void *pItem) {

	KnfcNode_t *newNode;

	if(pHeadNode == NULL) {
		return NULL;
	}

	newNode = (KnfcNode_t*)CMemoryAlloc(sizeof(KnfcNode_t));

	if(newNode != NULL) {
		newNode->pNext = pHeadNode->pNext;
		newNode->pPrev = pHeadNode;
		newNode->pItem = pItem;

		if(newNode->pNext != NULL) {
			newNode->pNext->pPrev = newNode;
		}
	}

	pHeadNode->pNext = newNode;
	return newNode;
}

KnfcNode_t *
KnfcDeleteNode(KnfcNode_t *pHeadNode, void *pItem) {

	KnfcNode_t *currNode;
	KnfcNode_t *prevNode;
	KnfcNode_t *nextNode;

	if(pHeadNode == NULL || pItem == NULL) {
		return NULL;
	}

	currNode = pHeadNode->pNext;

	for(; currNode != NULL; currNode = currNode->pNext) {

		if(currNode->pItem == pItem) {

			prevNode = currNode->pPrev;
			nextNode = currNode->pNext;

			prevNode->pNext = nextNode;

			if(nextNode != NULL) {
				nextNode->pPrev = prevNode;
			}

			return currNode;
		}
	}

	return NULL;
}

void
KnfcDeleteList(KnfcNode_t *pHeadNode) {

	KnfcNode_t *currNode;
	KnfcNode_t *prevNode;

	if(pHeadNode == NULL) {
		return;
	}

	prevNode = pHeadNode;
	currNode = pHeadNode->pNext;

	for(; currNode != NULL; currNode = currNode->pNext) {

		CMemoryFree(prevNode->pItem);
		CMemoryFree(prevNode);
		prevNode = currNode;
	}
}

int32_t
KnfcGetListSize(KnfcNode_t *pHeadNode) {

	int32_t listSize;
	KnfcNode_t *currNode;

	if(pHeadNode == NULL) {
		return -1;
	}

	listSize = 0;
	currNode = pHeadNode->pNext;

	for(; currNode != NULL; currNode = currNode->pNext) {
		listSize++;
	}

	return listSize;
}

KnfcNode_t *
KnfcGetNextItem(KnfcNode_t *pNode) {

	if(pNode == NULL) {
		return NULL;
	}

	if(pNode->pNext == NULL) {
		return NULL;
	}

	return pNode->pNext->pItem;
}
