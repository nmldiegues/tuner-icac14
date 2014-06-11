/* =============================================================================
 *
 * reservation.c
 * -- Representation of car, flight, and hotel relations
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


#include <stdlib.h>
#include "memory.h"
#include "reservation.h"
#include "tm.h"
#include "types.h"

/* =============================================================================
 * DECLARATION OF  FUNCTIONS
 * =============================================================================
 */


/* =============================================================================
 * reservation_info_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
__attribute__((transaction_safe)) reservation_info_t*
reservation_info_alloc (reservation_type_t type, long id, long price)
{
    reservation_info_t* reservationInfoPtr;

    reservationInfoPtr = (reservation_info_t*)malloc(sizeof(reservation_info_t));
    if (reservationInfoPtr != NULL) {
        reservationInfoPtr->type = type;
        reservationInfoPtr->id = id;
        reservationInfoPtr->price = price;
    }

    return reservationInfoPtr;
}


/* =============================================================================
 * reservation_info_free
 * =============================================================================
 */
__attribute__((transaction_safe)) void
reservation_info_free (reservation_info_t* reservationInfoPtr)
{
    free(reservationInfoPtr);
}


/* =============================================================================
 * reservation_info_compare
 * -- Returns -1 if A < B, 0 if A = B, 1 if A > B
 * =============================================================================
 */
__attribute__((transaction_safe)) long
reservation_info_compare (reservation_info_t* aPtr, reservation_info_t* bPtr)
{
    long typeDiff;

    typeDiff = aPtr->type - bPtr->type;

    return ((typeDiff != 0) ? (typeDiff) : (aPtr->id - bPtr->id));
}


/* =============================================================================
 * reservation_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
__attribute__((transaction_safe)) reservation_t*
reservation_alloc (long id, long numTotal, long price)
{
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)malloc(sizeof(reservation_t));
    if (reservationPtr != NULL) {
        reservationPtr->id = id;
        reservationPtr->numUsed = 0;
        reservationPtr->numFree = numTotal;
        reservationPtr->numTotal = numTotal;
        reservationPtr->price = price;
    }

    return reservationPtr;
}


reservation_t*
reservation_alloc_seq (long id, long numTotal, long price)
{
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)malloc(sizeof(reservation_t));
    if (reservationPtr != NULL) {
        reservationPtr->id = id;
        reservationPtr->numUsed = 0;
        reservationPtr->numFree = numTotal;
        reservationPtr->numTotal = numTotal;
        reservationPtr->price = price;
    }

    return reservationPtr;
}


/* =============================================================================
 * reservation_addToTotal
 * -- Adds if 'num' > 0, removes if 'num' < 0;
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
__attribute__((transaction_safe)) bool_t
reservation_addToTotal (reservation_t* reservationPtr, long num)
{
    long numFree = reservationPtr->numFree;

    if (numFree + num < 0) {
        return FALSE;
    }

    reservationPtr->numFree = numFree + num;
    reservationPtr->numTotal = reservationPtr->numTotal + num;

    return TRUE;
}


bool_t
reservation_addToTotal_seq (reservation_t* reservationPtr, long num)
{
    if (reservationPtr->numFree + num < 0) {
        return FALSE;
    }

    reservationPtr->numFree += num;
    reservationPtr->numTotal += num;

    return TRUE;
}


/* =============================================================================
 * reservation_make
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
__attribute__((transaction_safe)) bool_t
reservation_make (reservation_t* reservationPtr)
{
    long numFree = reservationPtr->numFree;

    if (numFree < 1) {
        return FALSE;
    }
    reservationPtr->numUsed = reservationPtr->numUsed + 1;
    reservationPtr->numFree = numFree - 1;

    return TRUE;
}


bool_t
reservation_make_seq (reservation_t* reservationPtr)
{
    if (reservationPtr->numFree < 1) {
        return FALSE;
    }

    reservationPtr->numUsed++;
    reservationPtr->numFree--;

    return TRUE;
}


/* =============================================================================
 * reservation_cancel
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
__attribute__((transaction_safe)) bool_t
reservation_cancel (reservation_t* reservationPtr)
{
    long numUsed = reservationPtr->numUsed;

    if (numUsed < 1) {
        return FALSE;
    }

    reservationPtr->numUsed = numUsed - 1;
    reservationPtr->numFree = reservationPtr->numFree + 1;

    return TRUE;
}


bool_t
reservation_cancel_seq (reservation_t* reservationPtr)
{
    if (reservationPtr->numUsed < 1) {
        return FALSE;
    }

    reservationPtr->numUsed--;
    reservationPtr->numFree++;

    return TRUE;
}


/* =============================================================================
 * reservation_updatePrice
 * -- Failure if 'price' < 0
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
__attribute__((transaction_safe)) bool_t
reservation_updatePrice (reservation_t* reservationPtr, long newPrice)
{
    if (newPrice < 0) {
        return FALSE;
    }

    reservationPtr->price = newPrice;

    return TRUE;
}


bool_t
reservation_updatePrice_seq (reservation_t* reservationPtr, long newPrice)
{
    if (newPrice < 0) {
        return FALSE;
    }

    reservationPtr->price = newPrice;

    return TRUE;
}


/* =============================================================================
 * reservation_compare
 * -- Returns -1 if A < B, 0 if A = B, 1 if A > B
 * =============================================================================
 */
__attribute__((transaction_safe)) long
reservation_compare (reservation_t* aPtr, reservation_t* bPtr)
{
    return (aPtr->id - bPtr->id);
}


/* =============================================================================
 * reservation_hash
 * =============================================================================
 */
__attribute__((transaction_safe)) ulong_t
reservation_hash (reservation_t* reservationPtr)
{
    /* Separate tables for cars, flights, etc, so no need to use 'type' */
    return (ulong_t)reservationPtr->id;
}


/* =============================================================================
 * reservation_free
 * =============================================================================
 */
__attribute__((transaction_safe)) void
reservation_free (reservation_t* reservationPtr)
{
    free(reservationPtr);
}




/* =============================================================================
 *
 * End of reservation.c
 *
 * =============================================================================
 */
