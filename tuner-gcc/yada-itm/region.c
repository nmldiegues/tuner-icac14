/* =============================================================================
 *
 * region.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#include <assert.h>
#include <stdlib.h>
#include "region.h"
#include "coordinate.h"
#include "element.h"
#include "list.h"
#include "map.h"
#include "queue.h"
#include "mesh.h"
#include "tm.h"


struct region {
    coordinate_t centerCoordinate;
    queue_t* expandQueuePtr;
    list_t* beforeListPtr; /* before retriangulation; list to avoid duplicates */
    list_t* borderListPtr; /* edges adjacent to region; list to avoid duplicates */
    vector_t* badVectorPtr;
};


/* =============================================================================
 * Pregion_alloc
 * =============================================================================
 */
region_t*
Pregion_alloc ()
{
    region_t* regionPtr;

    regionPtr = (region_t*)P_MALLOC(sizeof(region_t));
    if (regionPtr) {
        regionPtr->expandQueuePtr = PQUEUE_ALLOC(-1);
        assert(regionPtr->expandQueuePtr);
        regionPtr->beforeListPtr = PLIST_ALLOC(&element_listCompare);
        assert(regionPtr->beforeListPtr);
        regionPtr->borderListPtr = PLIST_ALLOC(&element_listCompareEdge);
        assert(regionPtr->borderListPtr);
        regionPtr->badVectorPtr = PVECTOR_ALLOC(1);
        assert(regionPtr->badVectorPtr);
    }

    return regionPtr;
}


/* =============================================================================
 * Pregion_free
 * =============================================================================
 */
void
Pregion_free (region_t* regionPtr)
{
    PVECTOR_FREE(regionPtr->badVectorPtr);
    PLIST_FREE(regionPtr->borderListPtr);
    PLIST_FREE(regionPtr->beforeListPtr);
    PQUEUE_FREE(regionPtr->expandQueuePtr);
    P_FREE(regionPtr);
}


/* =============================================================================
 * TMaddToBadVector
 * =============================================================================
 */
__attribute__((transaction_safe)) void
TMaddToBadVector (  vector_t* badVectorPtr, element_t* badElementPtr)
{
    bool_t status = PVECTOR_PUSHBACK(badVectorPtr, (void*)badElementPtr);
    TMELEMENT_SETISREFERENCED(badElementPtr, TRUE);
}


/* =============================================================================
 * TMretriangulate
 * -- Returns net amount of elements added to mesh
 * =============================================================================
 */
__attribute__((transaction_safe)) long
TMretriangulate (
                 element_t* elementPtr,
                 region_t* regionPtr,
                 mesh_t* meshPtr,
                 MAP_T* edgeMapPtr)
{
    vector_t* badVectorPtr = regionPtr->badVectorPtr; /* private */
    list_t* beforeListPtr = regionPtr->beforeListPtr; /* private */
    list_t* borderListPtr = regionPtr->borderListPtr; /* private */
    list_iter_t it;
    long numDelta = 0L;

    coordinate_t centerCoordinate = element_getNewPoint(elementPtr);

    /*
     * Remove the old triangles
     */

    list_iter_reset(&it, beforeListPtr);
    while (list_iter_hasNext(&it, beforeListPtr)) {
        element_t* beforeElementPtr =
            (element_t*)list_iter_next(&it, beforeListPtr);
        TMMESH_REMOVE(meshPtr, beforeElementPtr);
    }

    numDelta -= PLIST_GETSIZE(beforeListPtr);

    /*
     * If segment is encroached, split it in half
     */

    if (element_getNumEdge(elementPtr) == 1) {

        coordinate_t coordinates[2];

        edge_t* edgePtr = element_getEdge(elementPtr, 0);
        coordinates[0] = centerCoordinate;

        coordinates[1] = *(coordinate_t*)(edgePtr->firstPtr);
        element_t* aElementPtr = TMELEMENT_ALLOC(coordinates, 2);
        TMMESH_INSERT(meshPtr, aElementPtr, edgeMapPtr);

        coordinates[1] = *(coordinate_t*)(edgePtr->secondPtr);
        element_t* bElementPtr = TMELEMENT_ALLOC(coordinates, 2);
        TMMESH_INSERT(meshPtr, bElementPtr, edgeMapPtr);

        bool_t status;
        status = TMMESH_REMOVEBOUNDARY(meshPtr, element_getEdge(elementPtr, 0));
        status = TMMESH_INSERTBOUNDARY(meshPtr, element_getEdge(aElementPtr, 0));
        status = TMMESH_INSERTBOUNDARY(meshPtr, element_getEdge(bElementPtr, 0));

        numDelta += 2;
    }

    /*
     * Insert the new triangles. These are contructed using the new
     * point and the two points from the border segment.
     */

    list_iter_reset(&it, borderListPtr);
    while (list_iter_hasNext(&it, borderListPtr)) {
        element_t* afterElementPtr;
        coordinate_t coordinates[3];
        edge_t* borderEdgePtr = (edge_t*)list_iter_next(&it, borderListPtr);
        coordinates[0] = centerCoordinate;
        coordinates[1] = *(coordinate_t*)(borderEdgePtr->firstPtr);
        coordinates[2] = *(coordinate_t*)(borderEdgePtr->secondPtr);
        afterElementPtr = TMELEMENT_ALLOC(coordinates, 3);
        TMMESH_INSERT(meshPtr, afterElementPtr, edgeMapPtr);
        if (element_isBad(afterElementPtr)) {
            TMaddToBadVector(  badVectorPtr, afterElementPtr);
        }
    }

    numDelta += PLIST_GETSIZE(borderListPtr);

    return numDelta;
}


/* =============================================================================
 * TMgrowRegion
 * -- Return NULL if success, else pointer to encroached boundary
 * =============================================================================
 */
__attribute__((transaction_safe)) element_t*
TMgrowRegion (
              element_t* centerElementPtr,
              region_t* regionPtr,
              mesh_t* meshPtr,
              MAP_T* edgeMapPtr)
{
    bool_t isBoundary = FALSE;

    if (element_getNumEdge(centerElementPtr) == 1) {
        isBoundary = TRUE;
    }

    list_t* beforeListPtr = regionPtr->beforeListPtr;
    list_t* borderListPtr = regionPtr->borderListPtr;
    queue_t* expandQueuePtr = regionPtr->expandQueuePtr;

    PLIST_CLEAR(beforeListPtr);
    PLIST_CLEAR(borderListPtr);
    PQUEUE_CLEAR(expandQueuePtr);

    coordinate_t centerCoordinate = element_getNewPoint(centerElementPtr);
    coordinate_t* centerCoordinatePtr = &centerCoordinate;

    PQUEUE_PUSH(expandQueuePtr, (void*)centerElementPtr);
    while (!PQUEUE_ISEMPTY(expandQueuePtr)) {

        element_t* currentElementPtr = (element_t*)PQUEUE_POP(expandQueuePtr);

        PLIST_INSERT(beforeListPtr, (void*)currentElementPtr); /* no duplicates */
        list_t* neighborListPtr = element_getNeighborListPtr(currentElementPtr);

        list_iter_t it;
        TMLIST_ITER_RESET(&it, neighborListPtr);
        while (TMLIST_ITER_HASNEXT(&it, neighborListPtr)) {
            element_t* neighborElementPtr =
                (element_t*)TMLIST_ITER_NEXT(&it, neighborListPtr);
            TMELEMENT_ISGARBAGE(neighborElementPtr); /* so we can detect conflicts */
            if (!list_find(beforeListPtr, (void*)neighborElementPtr)) {
                if (element_isInCircumCircle(neighborElementPtr, centerCoordinatePtr)) {
                    /* This is part of the region */
                    if (!isBoundary && (element_getNumEdge(neighborElementPtr) == 1)) {
                        /* Encroached on mesh boundary so split it and restart */
                        return neighborElementPtr;
                    } else {
                        /* Continue breadth-first search */
                        PQUEUE_PUSH(expandQueuePtr,
                                                (void*)neighborElementPtr);
                    }
                } else {
                    /* This element borders region; save info for retriangulation */
                    edge_t* borderEdgePtr = element_getCommonEdge(neighborElementPtr, currentElementPtr);
                    PLIST_INSERT(borderListPtr,
                                 (void*)borderEdgePtr); /* no duplicates */
                    if (!MAP_CONTAINS(edgeMapPtr, borderEdgePtr)) {
                        PMAP_INSERT(edgeMapPtr, borderEdgePtr, neighborElementPtr);
                    }
                }
            } /* not visited before */
        } /* for each neighbor */

    } /* breadth-first search */

    return NULL;
}


/* =============================================================================
 * TMregion_refine
 * -- Returns net number of elements added to mesh
 * =============================================================================
 */
__attribute__((transaction_safe)) long
TMregion_refine (
                 region_t* regionPtr, element_t* elementPtr, mesh_t* meshPtr)
{

    long numDelta = 0L;
    MAP_T* edgeMapPtr = NULL;
    element_t* encroachElementPtr = NULL;

    TMELEMENT_ISGARBAGE(elementPtr); /* so we can detect conflicts */

    while (1) {
        edgeMapPtr = PMAP_ALLOC(NULL, &element_mapCompareEdge);
        encroachElementPtr = TMgrowRegion(
                                          elementPtr,
                                          regionPtr,
                                          meshPtr,
                                          edgeMapPtr);

        if (encroachElementPtr) {
            TMELEMENT_SETISREFERENCED(encroachElementPtr, TRUE);
            numDelta += TMregion_refine(
                                        regionPtr,
                                        encroachElementPtr,
                                        meshPtr);
            if (TMELEMENT_ISGARBAGE(elementPtr)) {
                break;
            }
        } else {
            break;
        }
        PMAP_FREE(edgeMapPtr);
    }

    /*
     * Perform retriangulation.
     */

    if (!TMELEMENT_ISGARBAGE(elementPtr)) {
        numDelta += TMretriangulate(
                                    elementPtr,
                                    regionPtr,
                                    meshPtr,
                                    edgeMapPtr);
    }

    PMAP_FREE(edgeMapPtr); /* no need to free elements */

    return numDelta;
}


/* =============================================================================
 * Pregion_clearBad
 * =============================================================================
 */
__attribute__((transaction_safe)) void
Pregion_clearBad (region_t* regionPtr)
{
    PVECTOR_CLEAR(regionPtr->badVectorPtr);
}


/* =============================================================================
 * TMregion_transferBad
 * =============================================================================
 */
__attribute__((transaction_safe)) void
TMregion_transferBad (  region_t* regionPtr, heap_t* workHeapPtr)
{
    vector_t* badVectorPtr = regionPtr->badVectorPtr;
    long numBad = PVECTOR_GETSIZE(badVectorPtr);
    long i;

    for (i = 0; i < numBad; i++) {
        element_t* badElementPtr = (element_t*)vector_at(badVectorPtr, i);
        if (TMELEMENT_ISGARBAGE(badElementPtr)) {
            TMELEMENT_FREE(badElementPtr);
        } else {
            TMHEAP_INSERT(workHeapPtr, (void*)badElementPtr);
        }
    }
}


/* =============================================================================
 *
 * End of region.c
 *
 * =============================================================================
 */
