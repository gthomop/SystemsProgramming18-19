/********* mergesort.h **********/
#ifndef _MERGESORT_H
#define _MERGESORT_H

#include <stdio.h>
#include "wallet.h"
#include <string.h>

//Mergesort is needed in order to sort the wallets by ID, so as to find certain wallets with Binary Search, before the HashTables are created

//Algorithm found on hackr.io, needs fewer mallocs than the "traditional" algorithm, which creates 2 arrays on every merge call

void merge(Wallet **wallets, size_t start, size_t middle, size_t end, Wallet **scndWallets);
void mergeSort(Wallet **wallets, size_t start, size_t end, Wallet **scndWallets);

#endif
