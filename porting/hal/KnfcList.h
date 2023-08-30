/*
 * KnfcList.h
 *
 *  Created on: Jun 26, 2013
 *      Author: youngsun
 */

#ifndef _KNFC_LIST_H_
#define _KNFC_LIST_H_

#include "wme_context.h"
#include "KnfcDebug.h"

//////////////////////////////////////////////////
// Data Structure Definition
//////////////////////////////////////////////////

typedef struct KnfcNode_s {
	void *pItem;
	struct KnfcNode_s *pPrev;
	struct KnfcNode_s *pNext;
} KnfcNode_t;

//////////////////////////////////////////////////
// Global Function Declaration
//////////////////////////////////////////////////

KnfcNode_t *KnfcNewList(void);
KnfcNode_t *KnfcInsertNode(KnfcNode_t *pHeadNode, void *pItem);
KnfcNode_t *KnfcDeleteNode(KnfcNode_t *pHeadNode, void *pItem);
void KnfcDeleteList(KnfcNode_t *pHeadNode);
int32_t KnfcGetListSize(KnfcNode_t *pHeadNode);
KnfcNode_t *KnfcGetNextItem(KnfcNode_t *pNode);
void KnfcPrintAllItems(KnfcNode_t *pHeadNode);

#endif /* _KNFC_LIST_H_ */
