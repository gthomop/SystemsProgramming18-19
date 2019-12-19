/******* mergesort.c ********/
#include "mergesort.h"


void merge(Wallet **wallets, size_t start, size_t middle, size_t end, Wallet **scndWallets){
        size_t i = start , j = middle + 1 , k = start;
        while (k <= end){
                if (i > middle){
                        scndWallets[k] = wallets[j];
                        j++;
                }
                else if (j > end){
                        scndWallets[k] = wallets[i];
                        i++;
                }
                else if (strcmp(wallets[i]->walletID, wallets[j]->walletID) < 0){
                        scndWallets[k] = wallets[i];
                        i++;
                }
                else if (strcmp(wallets[i]->walletID, wallets[j]->walletID) >= 0){
                        scndWallets[k] = wallets[j];
                        j++;
                }
                k++;
        }

        for (k = start; k <= end; k++)
                wallets[k] = scndWallets[k];
}
 
void mergeSort(Wallet **wallets, size_t start, size_t end, Wallet **scndWallets){
        if (start >= end)       return;
 
        size_t middle = (start + end) / 2;
        mergeSort(wallets, start, middle, scndWallets);
        mergeSort(wallets, middle + 1, end, scndWallets);

        merge(wallets, start, middle, end, scndWallets);
}
