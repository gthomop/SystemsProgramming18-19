/******** bitCoin.c *******/
#include "bitCoin.h"

/* struct BitCoin{
	size_t ID;			
	size_t numOfTransactions;		
	TransTree *root;				
	BitCoin *prevBC;				
	BitCoin *nextBC;				
};

struct TransTree{
	Wallet *walletID;		
	size_t amount;
	TransList *transaction;	
	TransTree *parent;		
	TransTree *left;		
	TransTree *right;		
}; */

//returns a pointer to the root of the transtree created for a BitCoin
TransTree *treeNodeInitialization(Wallet *wallet, size_t amnt, TransList *trans, TransTree *par){
	TransTree *node = malloc(sizeof(TransTree));
	node->walletID = wallet;
	node->amount = amnt;
	node->transaction = trans;
	node->parent = par;
	node->left = NULL;
	node->right = NULL;

	return node;
}

BitCoin *bitCoinInitialization(int id, Wallet *wallet, size_t value, BitCoin *previousBC){
	BitCoin *bC = malloc(sizeof(BitCoin));
	bC->ID = id;
	bC->numOfTransactions = 0;
	bC->root = treeNodeInitialization(wallet, value, NULL, NULL);
	bC->prevBC = previousBC;
	if (previousBC != NULL)
		previousBC->nextBC = bC;

	bC->nextBC = NULL;

	return bC;
}

size_t makeBCTransaction(BitCoin *BC, size_t amount, Wallet *sender, Wallet *receiver, TransList *transaction){

	size_t amountTransferred = 0;
	TransTree *node = BC->root;

	if (BC->root->right == NULL){	//if is is the first transaction using this bitCoin
		BC->root->right = treeNodeInitialization(receiver, amount, transaction, BC->root);
		amountTransferred += amount;
		if (BC->root->amount > amount)
			BC->root->left = treeNodeInitialization(BC->root->walletID, BC->root->amount - amount, transaction, BC->root);

		return amountTransferred;
	}

	if (BC->root->left != NULL)
		node = BC->root->left;
	else
		node = BC->root->right;


	while (amountTransferred < amount){

		if (node->left != NULL)		//not leaf node
			node = node->left;
		else if (node->right != NULL)		//only has right child
			node = node->right;
		else{					//leaf node
			if (!(strcmp(node->walletID->walletID, sender->walletID))){	//if this node is sender's
				if (amount - amountTransferred > node->amount){		//if the transaction amount is greater than the amount owned by sender

					node->right = treeNodeInitialization(receiver, node->amount, transaction, node);
					amountTransferred += node->amount;
					
				}
				else{
					node->right = treeNodeInitialization(receiver, amount - amountTransferred, transaction, node);
					if (node->amount > amount - amountTransferred){
						node->left = treeNodeInitialization(node->walletID, node->amount - amount + amountTransferred, transaction, node);
					}

					amountTransferred += amount - amountTransferred;
				}
			}

			//if we have reached a leaf node and it is not the right child of its parent
			//then we have not checked the right "brother" of node

			//if we have reached and searched a right child node
			//then we need to move as many levels up as we need to begin checking the
			//nearest subtree.
			//In other words, we look for a parent node, whose brother is a right child.

			while (node->parent->right == node){	//if node is its parent's right child
				node = node->parent;								//move to node's parent
				if (node == BC->root)
					break;
			}
			//when we reach this command under, we have reached a leaf node,
			//which means we either move to nearest subtree
			//or to the node's rightmost brother
			//(if the while loop above has run at least once / not at all, respectively)
			if (node != BC->root)
				node = node->parent->right;
			else
				break;
		}
	}
	BC->numOfTransactions++;
	return amountTransferred;

}

//free memory allocated for Transaction Tree
void freeTree(TransTree *root){
	TransTree *node = root;
	TransTree *tmpParent = NULL;

	while (node != NULL){
		if (node->left != NULL){
			node = node->left;
			tmpParent = node->parent;
		}
		else if (node->right != NULL){
			node = node->right;
			tmpParent = node->parent;
		}
		else{
			if (tmpParent == NULL){		//if node is root
				free(node);
				return;			//free root and end the freeing function
			}
			else if (tmpParent->right == node)
				tmpParent->right = NULL;
			else
				tmpParent->left = NULL;
			free(node);
			node = tmpParent;
			tmpParent = node->parent;
		}
	}
}

//frees the BitCoin
void freeBitCoin(BitCoin *BC){
	freeTree(BC->root);
	free(BC);
	return;
}

//free all BitCoins
void freeBitCoins(BitCoin *BC){
	BitCoin *currBC = BC;
	while (currBC->nextBC != NULL){
		currBC = currBC->nextBC;
		freeBitCoin(currBC->prevBC);
	}

	freeBitCoin(currBC);

	return;
}

void bitCoinStatus(size_t BCID, BitCoin *allBitCoins){
	BitCoin *currBC = allBitCoins;

	do{
		if (BCID == currBC->ID){		//assuming we found the BitCoin we are looking for
			//print the amount of root, which is its initial value
			//and then print the number of transactions this BitCoin has been involved into
			printf("%lu %lu ", currBC->root->amount, currBC->numOfTransactions);
			//then, if the left child of root is a leaf node, then the unspent amount is the amount left to the original owner
			if (currBC->root->left != NULL && currBC->root->left->right == NULL && currBC->root->left->left == NULL)
				printf("%lu\n", currBC->root->left->amount);
			//if root's left child is not a leaf node then all of this BitCoin has been involved into transactions
			else	printf("0\n");
			return;
		}

		currBC = currBC->nextBC;
	} while (currBC != NULL);

	//this line is reached only if the BCID does not resemble any BitCoin
	printf("BitCoin with ID %lu does not exist.\n", BCID);
	return;
}

//returns 1 if transID has not already been printed (in traceCoin) or, else, 0
int not_in(char **array, size_t index, char *transID){
	for (size_t i = 0; i < index; i++)
		if (!(strcmp(array[i], transID)))
			return 0;

	return 1;
}

void traceCoin(size_t BCID, BitCoin *allBitCoins){
	BitCoin *currBC = allBitCoins;
	do{
		if (BCID == currBC->ID){
			char **transArray = malloc(sizeof(char*) * currBC->numOfTransactions);		//save all transaction IDs already printed
			size_t index = 0;		//holds the position of last added string in transArray

			TransTree *node = currBC->root;
			if (node->right == NULL){
				printf("BitCoin %lu has not been involved in any transactions.\n", currBC->ID);
				free(transArray);
				return;
			}

			char *transID = NULL;
			char *senderID = NULL;
			char *receiverID = NULL;
			struct tm dateTime = {0};
			char dateTimeStr[32];

			while (node != NULL){
				if (node->right != NULL){
					node = node->right;
					transID = node->transaction->thisTrans->transactionID;
					if (not_in(transArray, index, transID)){
						senderID = node->parent->walletID->walletID;
						receiverID = node->walletID->walletID;
						gmtime_r(&(node->transaction->thisTrans->dateTime), &dateTime);
						strftime(dateTimeStr, 32, "%d-%m-%y %H:%M", &dateTime);
						printf("%s %s %s %lu %s\n",transID, senderID, receiverID, node->transaction->thisTrans->value, dateTimeStr);
						transArray[index] = malloc(sizeof(char)*(1 + strlen(transID)));
						strcpy(transArray[index], transID);
						index++;
					}
				}
				else if (node->left != NULL)
					node = node->left;
				else{
					transID = node->transaction->thisTrans->transactionID;
					if (not_in(transArray, index, transID)){
						senderID = node->parent->walletID->walletID;
						receiverID = node->walletID->walletID;
						gmtime_r(&(node->transaction->thisTrans->dateTime), &dateTime);
						strftime(dateTimeStr, 32, "%d-%m-%y %H:%M", &dateTime);
						printf("%s %s %s %lu %s\n",transID, senderID, receiverID, node->transaction->thisTrans->value, dateTimeStr);
						transArray[index] = malloc(sizeof(char)*(1 + strlen(transID)));
						strcpy(transArray[index], transID);
						index++;
					}
					while (node->parent->left == node){
						node = node->parent;
						if (node == currBC->root)
							break;
					}

					if (node != currBC->root)
						node = node->parent->left;
					else
						break;

				}
			}
			for (size_t i = 0; i < index; i++){
				free(transArray[i]);
			}
			free(transArray);
			return;
		}

		currBC = currBC->nextBC;
	} while (currBC != NULL);

	//this line is reached only if the BCID does not resemble any BitCoin
	printf("BitCoin with ID %lu does not exist.\n", BCID);
	return;
}