#include "syscall.h"

#define OT_FREE 0
#define OT_BUSY 1
#define OT_WAIT 2

int WAITER_COUNT = 0, COOK_COUNT = 0, CUST_COUNT = 0, OT_COUNT = 0;
int TABLE_COUNT = 20;
int tables[20];
int FILLED = 100, MININVENTORY = 50;
int SIXBURGER = 1, THREEBURGER = 2, VEGBURGER = 3, FRIES = 4;
int MINCOOKEDFOOD = 20 ;
int MAXDIFF = 4;		/* difference between the cooked food and the food to be cooked	*/
int orderTakerStatus[100];
int flag = 0;

/*------------------------------------------------------------------------ */
/* STRUCTURES */
/*------------------------------------------------------------------------ */
typedef struct{
			int sixBurger;   /* Costs 6$	/* value = 0 -> not ordered */
			int threeBurger; /* Costs 3$ 	   value = 1 -> ordered */
			int vegBurger;    /* Costs 8$	   value = 2 -> item is reserved */
			int fries;        /* Costs 2$	*/
			int soda;         /* Costs 1$	*/
			int dineType;    /* Type of service the customer has opted for */
							  /* - Eat-in = 0 */
							  /* - To-go  = 1 */   
			int ordered;		  /* set if he already take the order */
			
			int myOT;		  /* index of the OrderTaker servicing the customer */
			int tokenNo;	  /* Unique number which identifies the order  */
							  /* of the current customer */
			int tableNo;	  /* where the eat-in customer is seated	 */
			int bagged;		  /* set if order is bagged by Order Taker	 */
			int delivered;	  /* set if the order is bagged and delivered */
		}custDB; 

typedef struct{
			int sixBurger;
			int threeBurger;
			int vegBurger;
			int fries;
		}foodToBeCookedDB;

typedef struct{
			int sixBurger;
			int threeBurger;
			int vegBurger;
			int fries;
		}foodReadyDB;

/*------------------------------------------------------------------------ */
/* DATABASES */
/*------------------------------------------------------------------------ */
/* Customer database to store all the information related to a  */
/* particular customer */
custDB custData[100];

/* database to store the food to be cooked by the cooks */
foodToBeCookedDB foodToBeCookedData;

/* database to store the ready food cooked by the cooks */
foodReadyDB foodReadyData;	
	
/*------------------------------------------------------------------------ */
/* KERNEL LOCK INDEX VARIABLES */
/*------------------------------------------------------------------------ */

/* Locks - for customer-ordertaker interaction */
/* Lock to establish mutual exclusion on customer line */
int custLineLock = 0;          

/* Lock to establish mutual exclusion on the OT status  */
/* and for actions between customer and OT */
int orderTakerLock[100];	

/* Lock to establish mutual exclusion on the foodReadyDataBase */
int foodReadyDBLock = 0;

/* Lock to establish mutual exclusion on the foodToBeCookedDatabase */
int foodToBeCookedDBLock = 0;

/* Lock to get the next token number to be given to the customer */
int nextTokenNumberLock = 0;

/* Lock to access the location where all money collected at the restaurant */
/* is stored by the Order Takers */
int moneyAtRestaurantLock = 0;

/* Lock to wait in the to-go waiting line */
int waitToGoLock;

/* Lock to establish mutual exclusion on the tables data */
int tablesDataLock = 0;

/* Lock to wait in the eat-in table waiting line */
int eatInWaitingLock = 0;

/* Lock for the eat-in seated customers waiting for food */
int eatInFoodWaitingLock = 0;

/* Lock to update the foodToBag count */
int foodToBagLock = 0;

/* Lock to add/delete tokenNo into the List of bagged orders */
int foodBaggedListLock = 0;

/* Lock to access the raw materials inventory */
int inventoryLock = 0;

/* Lock to update the manager line length monitor */
int managerLock = 0;

/* Lock to update the what to cook value */
int whatToCookNextLock = 0;

/* Locks to set the stop cooking flags for different food items */
int stopSixBurgerLock = 0;
int stopThreeBurgerLock = 0;
int stopVegBurgerLock = 0;
int stopFriesLock = 0;

/* Lock for the communication between Manager and Waiter */
int waiterSleepLock = 0;

/* Customer data Lock */
int customerDataLock = 0;

/* Lock to enter a cook waiting queue / removing a cook from waiting queue */
int wakeUpCookLock = 0;

/* Lock to update the number of customers serviced */
int custServedLock = 0;

/* Lock to update the cooks on break count */
int cooksOnBreakCountLock = 0;

/* Lock to get the next customer index */
int nextCustomerIndexLock = 0;

/* Lock to get the next waiter index */
int nextWaiterIndexLock = 0;

/* Lock to get the next Cook Index */
int nextCookIndexLock = 0;

/* Lock to get the next ordertaker Index */
int nextOrderTakerIndexLock = 0;

/* Lock to set the item to be cooked by the cook when he is hired */
int whatToCookFirstLock = 0;

/*lock used when show how many working cooks for itme */
int workingCookLock = 0;

/* to access the bag the orders Lock */
int BagTheOrdersLock = 0;

/*------------------------------------------------------------------------ */
/* CONDITION VARIABLES */
/*------------------------------------------------------------------------ */							
/* CV - for customer-ordertaker interaction */
/* CV to sequence the customers in the customer line */
/* - customers to join the customer line */
/* - OT to remove a waiting customer from customer line */
int custLineCV = 0;

/* array of CV to sequence the actions between customer & OT */
int orderTakerCV[100];

/* CV for waiting in line to find a free table */
int tablesDataCV = 0;

/* CV for sequencing the customers waiting for the free table */
int eatInWaitingCV = 0;

/* CV for sequencing the eat-in seated customers waiting for the order */
int eatInFoodWaitingCV = 0;

/* CV for sequencing the eat-in customers in the manager reply waiting queue */
int managerCV = 0;

/* CV for waiters to wait for the signal from the manager */
int waiterSleepCV = 0;

/* Common cook waiting queue  */
int wakeUpCookCV = 0;

/* CV for the to-go customers to join the to-go waiting line */
int toGoCV = 0;

/*------------------------------------------------------------------------ */
/* GLOBAL MONITOR VARIABLES */
/*------------------------------------------------------------------------ */
int custLineLength = 0;		/* to access the line status one at a time */
int nextTokenNumber = 0;		/* to ensure that token number is unique */
int moneyAtRestaurant;		/* to store the money collected  */
int Refillcash;
int broadcastedTokenNo;		/* to store the current broadcasted token */
int managerLineLength;		/* to store information on the number of eat-in */
							/* customers waiting for manager reply  */
int tableAvailable;			/* manager sets this if table is available  */
int foodToBag;				/* number of orders still to be bagged	 */
int inventory = 100;				/* stores the current raw materials available */
							/* in the restaurant inventory */
int whatToCookNext;			/* tells the cook what to cook next	 */						
int stopSixBurger = 0;		/* Flags for the cooks to stop cooking */
int stopThreeBurger = 0;
int stopVegBurger = 0;
int stopFries = 0;		
int baggedList[200];		/* list to store the token numbers of bagged orders */
int custServed = 0;			/* Counter to keep a count of customers served */
int workingCooks[5];		/*indicate the number of working cooks */
int nextWaiterIndex = 1;	/* Every time a new waiter is spawned , he will set his index to */
							/* this value incremented once */
int nextCookIndex = 1;		/* Every time a new Cook is spawned , he will set his index to */
							/* this value incremented once */							
int nextCustomerIndex = 1;	/* Every time a new Customer is spawned , he will set his index to */
							/* this value incremented once */
int nextOrderTakerIndex = 1;/* Every time a new waiter is spawned , he will set his index to */
							/* this value incremented once */	

int whatToCookFirst = 0;	/* This value tells the Cook newly hired, what to start with */

int cooksOnBreakCount = 0;	/* Keeps track of the number of cooks who are on break
							   Manager checks this value to be 0 before hiring a new cook */
							   
/*------------------------------------------------------------------------ */
/* Randomly generates the customers order and fills the customer database */
/*------------------------------------------------------------------------ */
 
/* Customer thread body */  
void Customer()
{
    int index = 0;
	int internalOT = 0;
	int managerOT = 0;
	int myIndex = 0;
	
	/* Get the Index for this Customer before starting the order*/
	Acquire(nextCustomerIndexLock);
	myIndex = nextCustomerIndex;
	nextCustomerIndex++;
	Release(nextCustomerIndexLock);
	
    /* Acquire this LOCK to become the NEXT eligible customer */
	
	/* Check if any Order Taker is FREE */
	/* - if any then make him as the Order Taker for the current customer */
	/* - Make that Order Taker status as BUSY */
	for(index = 1; index <= OT_COUNT; index++){
		Acquire(custLineLock);
		if(orderTakerStatus[index] == OT_FREE){	
			orderTakerStatus[index] = OT_BUSY;
			Release(custLineLock);
			Acquire(customerDataLock);
			custData[myIndex].myOT = index;
			Release(customerDataLock);
			break;
		}
		Release(custLineLock);
	}
	
	/*If no Order Takers are free */
	/* - increment the linelength monitor variable */
	/* - then wait in the customer waiting line */
	Acquire(customerDataLock);
	if(custData[myIndex].myOT == 0){
		Release(customerDataLock);
		Acquire(custLineLock);
		custLineLength++;
		Wait(custLineCV, custLineLock);
		Release(custLineLock);
	}else	
		Release(customerDataLock);
		
	/* Customer is here because he received a signal from the Order Taker */
	/* - customer has also acquired the custLineLock */

	/* Check for the Order Taker who is WAITING */
	/* - make him as the Order Taker for the current customer */
	/* - Make that Order Taker status as BUSY */
	Acquire(customerDataLock);
	if(custData[myIndex].myOT == 0){
		Release(customerDataLock);
		for(index = 1; index <= OT_COUNT; index++){ 
			Acquire(custLineLock);
			if(orderTakerStatus[index] == OT_WAIT){	
				orderTakerStatus[index] = OT_BUSY;
				Release(custLineLock);
				Acquire(customerDataLock);
				custData[myIndex].myOT = index;
				Release(customerDataLock);	
				break;
			}else
				Release(custLineLock);
		
		}
	}else	
		Release(customerDataLock);	
	
	/* Acquire the orderTakerLock to wake up the Order Taker who is waiting */
	/* on the signal from the current customer */
	internalOT = custData[myIndex].myOT;
	Acquire(orderTakerLock[internalOT]);
	/* Send a signal to the Order Taker which indicates that the customer */
	/* is ready to give the order */
	Signal(orderTakerCV[internalOT], orderTakerLock[internalOT]);
    /* Customer goes on wait to receive a signal from the Order Taker */
	/* which indicates Order Taker is ready to take the order */
	Wait(orderTakerCV[internalOT], orderTakerLock[internalOT]);
	Release(orderTakerLock[internalOT]);
	/* Customer received a signal from the Order Taker to place an order */
	/* Order is randomly generated and is stored in the customer database */
	
	/* OG */
	if(managerOT != 1){
		Print_2Arg("Customer %d is giving order to OrderTaker %d\n$", myIndex, internalOT);
	}else{
		Print_1Arg("Customer %d is giving order to the manager\n$", myIndex);
	}
	
	/*OG */
	Acquire(customerDataLock);
	if(custData[myIndex].sixBurger == 1){
		Print_1Arg("Customer %d is ordering 6-dollar burger\n$", myIndex);
	}else{
		Print_1Arg("Customer %d is not ordering 6-dollar burger\n$", myIndex);
	}
	if(custData[myIndex].threeBurger == 1){
		Print_1Arg("Customer %d is ordering 3-dollar burger\n$", myIndex);
	}else{
		Print_1Arg("Customer %d is not ordering 3-dollar burger\n$", myIndex);
	}
	if(custData[myIndex].vegBurger == 1){
		Print_1Arg("Customer %d is ordering veggie burger\n$", myIndex);
	}else{
		Print_1Arg("Customer %d is not ordering veggie burger\n$", myIndex);
	}
	if(custData[myIndex].fries == 1){
		Print_1Arg("Customer %d is ordering french fries\n$", myIndex);
	}else{
		Print_1Arg("Customer %d is ordering french fries\n$", myIndex);
	}
	if(custData[myIndex].soda == 1){
		Print_1Arg("Customer %d is ordering soda\n$", myIndex);
	}else{
		Print_1Arg("Customer %d is not ordering soda\n$", myIndex);
	}
	if(custData[myIndex].dineType == 1){
		Print_1Arg("Customer %d chooses to to-go the food\n$", myIndex);
	}else{
		Print_1Arg("Customer %d chooses to eat-in the food\n$", myIndex);
	}
	Release(customerDataLock);
	/* Send a signal to the Order Taker which indicates that the customer */
	/* has placed an order */
	Acquire(orderTakerLock[internalOT]);
	Signal(orderTakerCV[internalOT],orderTakerLock[internalOT]);
	/* wait for the Order Taker to reply once he checks the food availability */
	Wait(orderTakerCV[internalOT],orderTakerLock[internalOT]);
	
	/* Received a signal from Order Taker which indicates that the order is */
	/* processed and its time to pay money */
	
	/* Send a signal to the Order Taker which indicates that the customer */
	/* has payed the bill amount */ 
	Signal(orderTakerCV[internalOT],orderTakerLock[internalOT]);
	
	/* Wait for the Order Taker to acknowledge the payment being done */ 
	/* successfully */
	Wait(orderTakerCV[internalOT], orderTakerLock[internalOT]);
	Release(orderTakerLock[internalOT]);
	/* Received a signal from the Order Taker to indicate that the payment */
	/* was completed and the customer can move to the next stage */ 

	/* If to-go then check if the order is bagged right away */
	/* else go and wait for a broadcast of the tokenNo */
	
	if(custData[myIndex].dineType == 1){
		/* if to-go and if the order is delivered then the customer takes the */
		/* bag and just leaves the restaurant */
		/* release the Order Taker Lock */
		Acquire(customerDataLock);
		if(custData[myIndex].delivered == 1){
			Release(customerDataLock);	
			/* OG */
			if(managerOT != 1){
				Print_2Arg("Customer %d receives food from the OrderTaker %d\n$",myIndex, internalOT);
			}else{
				Print_2Arg("Customer %d receives food from the Manager %d\n$",myIndex, internalOT);
			}
			
			/* Acquire the Lock to update the custer count served the food */
			Acquire(custServedLock);
			custServed++;
			/* @ */
			/* Print_2Arg("\n\nCUSTOMER %d SERVICED IS %d\n\n$", myIndex, custServed); */
			Release(custServedLock);
			/* Print_1Arg("To Go Customer %d calling E---X---I---T\n$", myIndex); */
			Exit(0);
	
			/* if to-go and if the order is not ready then customer has to wait */
			/* for a broadcast signal from the Order Taker when the order is bagged */
		
			/* Acquire the Lock which is used to match with To-go waiting */
			/* condition variable waitingLock */
		}else{
			Release(customerDataLock);	
			/* OG */
			if(managerOT != 1){
				Print_3Arg("Togo Customer %d is given token number %d by the OrderTaker %d\n$", myIndex, custData[myIndex].tokenNo, internalOT);
			}else{
				Print_2Arg("Customer %d is given token number %d by the Manager\n$",myIndex, custData[myIndex].tokenNo);
			}
			Acquire(waitToGoLock);
				
			/* Go on wait to receive the broadcast signal from the Order Taker */
			Wait(toGoCV,waitToGoLock);
			Release(waitToGoLock);
	
			while(1){
				/* Received a Broadcast signal from one of the Order Taker */
				/* - Check if the broadcasted tokenNo is customer's tokenNo */
				Acquire(customerDataLock);
				if(custData[myIndex].delivered = 1)	{
					Release(customerDataLock);
					/* Acquire the Lock to update the custer count served the food */
					Acquire(custServedLock);
					custServed++;
					/* @ */
					/* Print_2Arg("\n\nCUSTOMER %d SERVICED IS %d\n\n$", myIndex,custServed); */
					Release(custServedLock);
					/* Print_1Arg("Customer %d calling E---X---I---T\n$", myIndex); */
					Exit(0);
					break;
				}else{ 
					Release(customerDataLock);
					/* Go on wait to receive the next broadcast signal from the waiter */
					Acquire(waitToGoLock);
					Wait(toGoCV,waitToGoLock);
					Release(waitToGoLock);
				}	
			}
	    }
	}/* if dineType is eat-in */
	else if(custData[myIndex].dineType == 0){
		
		/*  OG */
		/* Print_3Arg("EatinCustomer %d is given token number %d by the OrderTaker %d\n$", myIndex, custData[myIndex].tokenNo, internalOT); */
		/* Acquire a Lock to add customer onto the queue waiting for */
		/* manager's reply */
		Acquire(managerLock);
		/* Increment the customer waiting count */
		managerLineLength++;
		/* go on wait till manager signals with table availability */
		Wait(managerCV,managerLock);

		/* Received a signal from manager */
		/* Customer has acquired the manager lock */
		/* Manager replied saying "restaurant is not full" */
		if(tableAvailable == 1){		
			/* OG */
			Print_1Arg("Customer %d is informed by the Manager-the restaurant is not full\n$", myIndex);
			tableAvailable = 0;
			Release(managerLock);
			for(index = 1; index <= TABLE_COUNT; index++){
				Acquire(tablesDataLock);
				if(tables[index] == 0){
					/* make the first table which is free as the customer */
					/* table */
					tables[index] = custData[myIndex].tokenNo;
					Release(tablesDataLock);
					Acquire(customerDataLock);
					custData[myIndex].tableNo = index;
					Release(customerDataLock);
					/* @ */
					/* Print_3Arg("table %d is taken by custom %d tokenNo %d\n$", index, myIndex, tables[index]);	 */
					break;
				}else
					Release(tablesDataLock);
			}
			
			/* if a table is found then go sit and wait for food */
			if(custData[myIndex].tableNo != 0){	
				/* OG	*/
				Print_2Arg("customer %d is seated at table number %d\n$",myIndex, custData[myIndex].tableNo);
					
				/* Before releasing the tablesDataLock Acquire the */
				/* eatInFoodWaitingLock to ensure that this customer is the */ 
				/* next one to join the queue waiting for the food to be bagged */

				/* customer goes on wait to receive a signal from the waiter */
				/* after the tokenNo is validated */
				while(1){
					Acquire(eatInFoodWaitingLock);
					
					Wait(eatInFoodWaitingCV,eatInFoodWaitingLock);
				
					Release(eatInFoodWaitingLock);
		 			/* upon receiving the signal check if order is delivered */
					Acquire(customerDataLock);
					if(custData[myIndex].delivered == 1){
						/* if delivered then the customer eats the food and */
						/* leaves the restaurant */
						Release(customerDataLock);
						Acquire(tablesDataLock);			
						tables[custData[myIndex].tableNo] = 0;
						Release(tablesDataLock);
						
						Acquire(eatInWaitingLock);
						Signal(eatInWaitingCV,eatInWaitingLock);
						Release(eatInWaitingLock);	
						/* Acquire the Lock to update the custer count served the food */
						Acquire(custServedLock);
						custServed++;
						/* @ */
						/* Print_2Arg("\n\nCUSTOMER %d SERVICED IS %d\n\n$", myIndex, custServed); */
						Release(custServedLock);
						/* Print_1Arg("Customer %d calling E---X---I---T\n$", myIndex); */
						Exit(0);	
					}
					Release(customerDataLock);
				}
			}
			
			/* Manager replied saying "restaurant is full"	*/
		}else if(tableAvailable == 0){
			Release(managerLock);
			/* OG */
			Print_1Arg("Customer %d is informed by the Manager-the restaurant is full\n$", myIndex);
			/* if restaurant is full and no table is available where the */
			/* customer can sit */
				
			/* Acquire the eat-in table waiting lock before releasing the */
			/* tablesDataLock so that customer wont get context switched */
			/* and he can be the next person to join the waiting queue */
				
			/* Release the tables Data lock	 */
			
			Acquire(eatInWaitingLock);
				
			/* OG */
			Print_1Arg("customer %d is waiting to sit on the table\n$", myIndex);
				
			/* Customer goes on wait till he receives a signal from */
			/* a seated customer who received his bag and is leaving */
			/* the restaurant */
			Wait(eatInWaitingCV,eatInWaitingLock);
			Release(eatInWaitingLock);
			/* Received a signal from one of the customer leaving */
			/* Acquire the Lock on the tables data to make the */
			/* freed table number as the current customer table */
			Acquire(tablesDataLock);	
				
			for(index = 1; index <= TABLE_COUNT; index++){
				if(tables[index] == 0){
					/* make the first table which is free as the customer */
					/* table */
					tables[index] = custData[myIndex].tokenNo;
					
					Release(tablesDataLock);
					Acquire(customerDataLock);
					custData[myIndex].tableNo = index;
					Release(customerDataLock);
					Acquire(tablesDataLock);
					break;
				}	
			}
				
			/* Release the tables data lock */
			Release(tablesDataLock);
			Acquire(customerDataLock);
			
			if(custData[myIndex].tableNo != 0){
				Release(customerDataLock);
		
				/* Before releasing the tablesDataLock Acquire the */
				/* eatInFoodWaitingLock to ensure that this customer is the */
				/* next one to join the queue waiting for the food to be bagged */
				Acquire(eatInFoodWaitingLock);
				/* customer goes on wait to receive a signal from the waiter */
				/* after the tokenNo is validated */	
				/* OG */
				Print_1Arg("Customer %d is waiting for the waiter to serve the food\n$",myIndex);
		
				Wait(eatInFoodWaitingCV,eatInFoodWaitingLock);
					/*potential problem	*/
				/* upon receiving the signal check if order is delivered */
				while(custData[myIndex].delivered != 1){
					/* else Go Back to wait */
					Wait(eatInFoodWaitingCV,eatInFoodWaitingLock);
				}
				Release(eatInFoodWaitingLock);
				/* OG */
				Print_1Arg("Customer %d is served by waiter\n$", myIndex);
				/* if delivered then the customer eats the food and */
				/* leaves the restaurant */
						
				/* OG */
				if(managerOT != 1){	
					Print_2Arg("Customer %d is leaving the restaurant after OrderTaker %d packed the food\n$",myIndex, internalOT);
				}else{
					Print_1Arg("Customer %d is leaving the restaurant after the Manager packed the food\n$",myIndex);
				}	
						Acquire(tablesDataLock);			
						tables[custData[myIndex].tableNo] = 0;
						Release(tablesDataLock);

				/* once the order is delivered, the seated customer before */
				/* leaving the restaurant signals one of the customers */
				/* who is waiting for a table to sit */
				Acquire(eatInWaitingLock);		
				Signal(eatInWaitingCV,eatInWaitingLock);
					
				Release(eatInWaitingLock);
				/* OG */	
				Print_1Arg("EAT-IN  Customer %d is leaving the restaurant after having food\n$",myIndex);
					 
				/* Acquire the Lock to update the custer count served the food	*/	
				Acquire(custServedLock);
				custServed++;		
				/* @ */
				/* Print_2Arg("\n\nCUSTOMER %d SERVICED IS %d\n\n$", myIndex, custServed);		 */
				Release(custServedLock);	
				/* Print_1Arg("Customer %d calling E---X---I---T\n$", myIndex);	 */
				Exit(0);
			}
			else
				Release(customerDataLock);
		}
	}
}


int BagTheOrders()
{
	int index = 0;	
	int cannotBeBagged = 0;
	
	/* if there are no more customers in the customer line */
	/* then Order Taker starts bagging the orders */
	for(index = 1; index <= CUST_COUNT; index++){
		cannotBeBagged = 0;
		
		Acquire(customerDataLock);
		/* check for all the orders which are not bagged */
		if(custData[index].bagged != 1)
		{
			if(custData[index].sixBurger == 1)
			{
				Release(customerDataLock);
				/*if six$burgers available */
				Acquire(foodReadyDBLock);
				if(foodReadyData.sixBurger != 0)
				{
					Release(foodReadyDBLock);
					/*reserve a burger for this customer */
					Acquire(customerDataLock);
					custData[index].sixBurger = 2;
					Release(customerDataLock);
					/* if greater than minimum amount of cooked food */
					/* then bag it, no worries */
					Acquire(foodReadyDBLock);
					if(foodReadyData.sixBurger > MINCOOKEDFOOD)
					{
						foodReadyData.sixBurger--;	
						Release(foodReadyDBLock);
						Acquire(foodToBeCookedDBLock);
						foodToBeCookedData.sixBurger--;
						Release(foodToBeCookedDBLock);
					}
					/* else if it is less than minimum then  */
					/* bag it and increment the food to be cooked */
					else{
						foodReadyData.sixBurger--;	
						Release(foodReadyDBLock);
						/* Do not decrement the foodtoBeCookedData because the */
						/* amount of cooked food is less than minimum */
					}
				}
				else if(foodReadyData.sixBurger == 0){
					cannotBeBagged = 1;
					Release(foodReadyDBLock);
				}
			}else
				Release(customerDataLock);
				/* if 3$burger is ordered */
				
				Acquire(customerDataLock);
				if(custData[index].threeBurger == 1)
				{
					Release(customerDataLock);
					/*if three$burgers available */
					Acquire(foodReadyDBLock);
					if(foodReadyData.threeBurger != 0)
					{
						Release(foodReadyDBLock);
						/*reserve a burger for this customer */
						Acquire(customerDataLock);
						custData[index].threeBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food */
						/* then bag it, no worries */
						Acquire(foodReadyDBLock);
						if(foodReadyData.threeBurger > MINCOOKEDFOOD)
						{
							foodReadyData.threeBurger--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.threeBurger--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then  */
						/* bag it and do not decrement the food to be cooked */
						else{
							foodReadyData.threeBurger--;	
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the */
							/* amount of cooked food is less than minimum */
						}
					}
					else if(foodReadyData.threeBurger == 0)
					{
						cannotBeBagged = 1;
						Release(foodReadyDBLock);
					}	
				}else
					Release(customerDataLock);
				
				
				/* if vegburger is ordered */
				Acquire(customerDataLock);
				if(custData[index].vegBurger == 1)
				{
					Release(customerDataLock);
					/*if vegburgers available */
					Acquire(foodReadyDBLock);
					if(foodReadyData.vegBurger != 0)
					{
						Release(foodReadyDBLock);
						/*reserve a burger for this customer */
						Acquire(customerDataLock);
						custData[index].vegBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food */
						/* then bag it, no worries */
						Acquire(foodReadyDBLock);
						if(foodReadyData.vegBurger > MINCOOKEDFOOD)
						{
							foodReadyData.vegBurger--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.vegBurger--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then  */
						/* bag it and do not decrement the food to be cooked */
						else{
							foodReadyData.vegBurger--;	
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the */
							/* amount of cooked food is less than minimum */
						}
					}
					else if(foodReadyData.vegBurger == 0)
					{
						cannotBeBagged = 1;
						Release(foodReadyDBLock);
					}
				}else
					Release(customerDataLock);
				
				
				/* if fries is ordered */
				Acquire(customerDataLock);
				if(custData[index].fries == 1)
				{
					Release(customerDataLock);
					/*if fries available */
					Acquire(foodReadyDBLock);
					if(foodReadyData.fries != 0)
					{
						Release(foodReadyDBLock);
						/*reserve a fries for this customer */
						Acquire(customerDataLock);
						custData[index].fries = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food */
						/* then bag it, no worries */
						Acquire(foodReadyDBLock);
						if(foodReadyData.fries > MINCOOKEDFOOD)
						{
							foodReadyData.fries--;
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);	
							foodToBeCookedData.fries--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then */
						/* bag it and do not decrement the food to be cooked */
						else{
							foodReadyData.fries--;	
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the */
							/* amount of cooked food is less than minimum */
						}
					}
					else if(foodReadyData.fries == 0){
						cannotBeBagged = 1;
						Release(foodReadyDBLock);
					}
				}else
					Release(customerDataLock);
					
					
				if(cannotBeBagged == 1)
				{
					continue;
				}else if(cannotBeBagged == 0)
				{					
					/* Order is bagged so update the food to bag count */
					Acquire(foodToBagLock);
					foodToBag-- ;
					Release(foodToBagLock);
					/* make the customer order status as bagged */
	
/*CHANGE:ADD ============*/	Acquire(BagTheOrdersLock);
					custData[index].bagged = 1;
/*CHANGE:ADD ============*/	Release(BagTheOrdersLock);
					
					return (index);
				}
		}
		else
		{
				Release(customerDataLock);
		}
	}	
	return -1;
}


/* OrderTaker thread body */  
void OrderTaker()
{
	int index = 0, indexj = 0;	/* index for looping through customer database	*/
	int custIndex = 0;			/* to store the cust index who being served 	*/
	int custHasToWait = 0;		/* if to-go and food not ready set this flag	*/
	int billAmount = 0;			/* to store the bill amount payed by customer	*/
	int myIndex = 0;
	
	/* Get the Index for this Customer before starting the order */
	Acquire(nextOrderTakerIndexLock);
	myIndex = nextOrderTakerIndex;
	nextOrderTakerIndex++;
	Release(nextOrderTakerIndexLock);
	
	while(1)
	{
		Acquire(custServedLock);
			if(custServed == CUST_COUNT){
				Release(custServedLock);
				/* @ */
				/* Print_1Arg("OrderTaker %d calling E---X--I---T\n$", myIndex); */
				Exit(0);
			}
		Release(custServedLock);
				
		/* Acquire custLineLock to access the custLineLength monitor variable	*/
		Acquire(custLineLock);
		/* Check for any customers waiting in the customer waiting line	*/
		if(custLineLength > 0){
			/* customers are available so I am releasing the food to bag lock	*/	
			/* There are customers in the customer waiting line	*/
			/* Signal a customer which indicates the customer to come out of	*/
			/* the waiting line and give his order	*/
			Signal(custLineCV, custLineLock);
			
			/* Decrement the custLineLength moniter variable value by 1	*/
			custLineLength--;
			
			/* Before releasing the custLineLock, 	*/
			/* Order Taker changes his status to WAITING	*/
			orderTakerStatus[myIndex] = OT_WAIT;
			Release(custLineLock);
		}		
		else{ 
			Release(custLineLock);
			/* Acquire the foodToBagLock to check if there are any orders to bag	*/
			Acquire(foodToBagLock);
		
			if(foodToBag > 0){
				/* If No customers in the customer waiting line and	*/
				/* if there is some food to bag	*/
			
				/* bag 1 at a time	*/
				/* index of the customer whose order was bagged is returned	*/
				Release(foodToBagLock);
				
				Acquire(BagTheOrdersLock);
				index = BagTheOrders();
				if(index > 0){
					/* OG	*/
					Print_2Arg("OrderTaker %d packed the food for Customer %d\n$", myIndex, index);
/*CHANGE:DELETE ============						Acquire(customerDataLock); */
					custData[index].bagged = 1;
					
/*CHANGE:DELETE============					Release(customerDataLock); */
					Release(BagTheOrdersLock);
					/* after bagging an order 	*/
					/* if the customer has chosen to-go then broadcast the signal	*/
					/* for the customers waiting in the toGoWaiting line	*/
					if(custData[index].dineType == 1){
						/* Acquire the Lock which is used to match with To-go	*/
						/* waiting condition variable waitingLock	*/
						Acquire(waitToGoLock);
						
						Broadcast(toGoCV, waitToGoLock);
						Release(waitToGoLock);	
						
						/* set the broadcastedTokenNo monitor variable to the	*/
						/* tokenNo of the customer whose order was bagged	*/
						Acquire(customerDataLock);
						if(custData[index].bagged == 1)
							custData[index].delivered = 1;
						
						/* Order Taker Broadcasts the signal to all to-go waiting	*/
						Release(customerDataLock);
						
						/* Release the waiting line Lock so that the customers	*/
						/* who received the signal can acquire the Lock and compare	*/
						/* their tokenNo with the broadcastedTokenNo	*/
						
					}/* else if the customer has chosen to eat-in then the tokenNo 	*/
					/* of the order which is bagged is added to the baggedList 	*/
					/* from which the waiter will remove the tokenNo and 	*/
					/* deliver it to the eat-in seated customer 	*/
					else if(custData[index].dineType == 0){
						/* Acquire the Lock to Append the tokenNo to the 	*/
						/* bagged List		*/
/*CHANGE:ADD ============*/	Acquire(BagTheOrdersLock);	
						custData[index].bagged = 1;
/*CHANGE:ADD ============*/	Release(BagTheOrdersLock);
						
						Acquire(foodBaggedListLock);
						for(indexj = 1; indexj <= CUST_COUNT; indexj++){
							if(baggedList[indexj] == 0){
								baggedList[indexj] = custData[index].tokenNo;
								break;
							}
						}
						/* OG	*/
						Print_3Arg("Ordertaker %d gives Token number %d to Waiter for Customer %d\n$", myIndex, custData[index].tokenNo, index);
						Release(foodBaggedListLock);
					}
				}
				Yield(); 
				Release(BagTheOrdersLock);
			} 
			else{
				/* no customers are available and no food to bag so	*/
				/* releasing the food to bag lock	*/
				Release(foodToBagLock);
				/* No customers to serve and no food to bag so the Order Taker 	*/
				/* has nothing to do, Set the status of that Order Taker as FREE	*/
				/* Check if all the Customers are serviced	*/
				Acquire(custServedLock);
				if(custServed == CUST_COUNT){
					Release(custServedLock);
					Print_1Arg("OrderTaker %d calling E---X---I---T\n$", myIndex);
					Exit(0);
				}
				Release(custServedLock);
				Acquire(custLineLock);
				orderTakerStatus[myIndex] = OT_FREE;	
				Release(custLineLock);
			}		
		}
	
		if((orderTakerStatus[myIndex] == OT_WAIT) || (orderTakerStatus[myIndex] == OT_FREE))
		{
			/* Acquire the orderTakerLock before releasing the custLineLock 	*/
			/* - To ensure that the Customer acquires the orderTakerLock 	*/
			/*   after Order Taker does	*/
			Acquire(orderTakerLock[myIndex]);
			/*Release the custLineLock 	*/
			/* - so that the Customer can access the updated status of the Order Taker	*/
		    /* @ */
			 /* Print_1Arg("OrderTaker %d is waiting\n$", myIndex); */
			
			/* Order Taker goes on wait to receive a signal from the Customer 	*/
			/* which indicates Customer is ready to place the order	*/
			Wait(orderTakerCV[myIndex], orderTakerLock[myIndex]);
			
		 	Release(orderTakerLock[myIndex]);
			
			Acquire(custServedLock);
			if(custServed == CUST_COUNT){
				Release(custServedLock);
				/* @ */
				/* Print_1Arg("OrderTaker %d calling E---X---I---T\n$", myIndex); */
				Exit(0);
			}
			Release(custServedLock);
			
			Acquire(orderTakerLock[myIndex]);
			
			/* Send a signal to the Customer which indicates that the Order Taker	*/
			/* is ready to take the order	*/
			Signal(orderTakerCV[myIndex], orderTakerLock[myIndex]);
			
			/* Order Taker will go on wait till he receives a signal from the 	*/
			/* customer after placing the order	*/
			Wait(orderTakerCV[myIndex], orderTakerLock[myIndex]);
			Release(orderTakerLock[myIndex]);
			
			Acquire(custServedLock);
			if(custServed == CUST_COUNT){
				Release(custServedLock);
				/* @ */
				/* Print_1Arg("OrderTaker %d calling E---X---I---T\n$", myIndex); */
				Exit(0);
			}
			Release(custServedLock);
			
			/* Received a signal from the customer after placing an order 	*/
			/* Find the customer index once the order is placed	*/
			Acquire(customerDataLock);
			for( index = 1; index <= CUST_COUNT; index++){
				/* find the customer index by finding the customer who has their Order	*/
				/* taker index as the current Order taker Index Thread	*/
				if(custData[index].myOT == myIndex){
					if(custData[index].ordered != 1){
						custIndex = index;
						custData[index].ordered = 1;
						break;
					}
				}
			}
			Release(customerDataLock);
			
			/* OG	*/
			Print_2Arg("OrderTaker %d is taking order of Customer %d\n$", myIndex, custIndex);
			
			/* if to-go customer then  - to check if the food is ready right away	*/
			/*						   - if not ready then make customer wait	*/
			/*						   - modify the food required monitor variable	*/
			
			if(custData[custIndex].dineType == 1){
				custHasToWait = 0;
				/* if 6$burger is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].sixBurger == 1){
					Release(customerDataLock);
					/* if 6$burger is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.sixBurger != 0){
						Release(foodReadyDBLock);
						/*reserve a burger for this customer	*/
						Acquire(customerDataLock);
						custData[custIndex].sixBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.sixBurger > MINCOOKEDFOOD){
							foodReadyData.sixBurger--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.sixBurger--;
							Release(foodToBeCookedDBLock);
						}
						else{
						/* else if it is less than minimum then 	*/
						/* bag it and increment the food to be cooked	*/
							foodReadyData.sixBurger--;	
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/
						}
					}	
					else{ /* customer has to wait	*/
						custHasToWait = 1;
						Release(foodReadyDBLock);
					}
				}
				else
					Release(customerDataLock);
				
				/* if 3$burger is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].threeBurger == 1){
					Release(customerDataLock);
					/* if 3$burger is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.threeBurger != 0){
						Release(foodReadyDBLock);
						/*reserve a burger for this customer	*/
						Acquire(customerDataLock);
						custData[custIndex].threeBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.threeBurger > MINCOOKEDFOOD){
						foodReadyData.threeBurger--;
						Release(foodReadyDBLock);
						Acquire(foodToBeCookedDBLock);
						foodToBeCookedData.threeBurger--;
						Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then 	*/
						/* bag it and do not decrement the food to be cooked	*/
						else{
							foodReadyData.threeBurger--;
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/						
						}						
					}	
					else{ /* customer has to wait	*/
							custHasToWait = 1;
							Release(foodReadyDBLock);
					}
				}
				else
					Release(customerDataLock);
				
				/* if vegburger is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].vegBurger == 1){
					Release(customerDataLock);
					/* if vegburger is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.vegBurger != 0){
					Release(foodReadyDBLock);
						/*reserve a burger for this customer*/
						Acquire(customerDataLock);
						custData[custIndex].vegBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.vegBurger > MINCOOKEDFOOD){
							foodReadyData.vegBurger--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.vegBurger--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then 	*/
						/* bag it and do not decrement the food to be cooked	*/
						else{
							foodReadyData.vegBurger--;	
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/
						}	
					}
					else {/* customer has to wait	*/
						custHasToWait = 1;
						Release(foodReadyDBLock);
					}
				}
				else
					Release(customerDataLock);
				
				/* if fries is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].fries == 1){
				Release(customerDataLock);
					/* if fries is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.fries != 0){
					Release(foodReadyDBLock);
						/*reserve a fries for this customer	*/
						Acquire(customerDataLock);
						custData[custIndex].fries = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.fries > MINCOOKEDFOOD){
							foodReadyData.fries--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.fries--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then 	*/
						/* bag it and do not decrement the food to be cooked	*/
						else{
							foodReadyData.fries--;
							Release(foodReadyDBLock);							
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/
						}		
					}
					else{ /* customer has to wait	*/
						custHasToWait = 1;
						Release(foodReadyDBLock);
					}
				}
				else 
					Release(customerDataLock);
			
				/* if to-go and if customer has to wait	*/
				if(custHasToWait == 1){
					/* set the token number for the customer	*/
					/* Acquire the lock to get the next token number to be given 	*/
					/* to the customer, all Order Takers access this value	*/
					/* - increment this monitor variable to generate the new unique	*/
					/*   token number and give it to customer	*/
					Acquire(nextTokenNumberLock);
					
					nextTokenNumber++;
					custData[custIndex].tokenNo = nextTokenNumber;
					
					/* Release the lock after obtaining the new token number	*/
					Release(nextTokenNumberLock);
					/* Acquire the foodToBagLock to increment the foodToBag count	*/
					Acquire(foodToBagLock);
					foodToBag++;
					/* Release the foodToBagLock after updating the foodToBag count	*/
					Release(foodToBagLock);
					
					/* OG	*/
					Print_3Arg("OrderTaker %d gives token number %d to TOGO Customer %d\n$", myIndex, custData[custIndex].tokenNo, custIndex);
				}else{
					/* food is ready and can be bagged	*/
					/* Even if the customer has ordered only soda	*/
					/* it is bagged here	*/
					/* Order is bagged so update the food to bag count
	
/*CHANGE:ADD ============*/	Acquire(BagTheOrdersLock);					
					/* make the customer order status as bagged	*/
					custData[custIndex].bagged = 1;
/*CHANGE:ADD ============*/	Release(BagTheOrdersLock);					
/*CHANGE:ADD ============*/	Acquire(customerDataLock);
			
				custData[custIndex].delivered = 1;
					/* OG	*/
					Print_2Arg("OrderTaker %d packs and gives food to TOGOCustomer %d\n$", myIndex, custIndex);
					/* Release the foodToBagLock after updating the foodToBag count	*/
					Release(customerDataLock);
					/* Order Taker will bag only one order at a time so break	*/
				}
			}	/* dineType = to-go		*/
				
			/* if eat-in	*/
			if(custData[custIndex].dineType == 0){
				custHasToWait = 0;
				/* always customer has to wait, he will not get food right away	*/
				/* set the token number for the customer	*/
				/* Acquire the lock to get the next token number to be given 	*/
				/* to the customer, all Order Takers access this value	*/
				/* - increment this monitor variable to generate the new unique	*/
				/*   token number and give it to customer	*/
				Acquire(nextTokenNumberLock);
				nextTokenNumber++;
				custData[custIndex].tokenNo = nextTokenNumber;		
				/* Release the lock after obtaining the new token number	*/
				/* OG	*/
				Print_3Arg("OrderTaker %d gives token number %d to EATINCustomer %d\n$", myIndex, custData[custIndex].tokenNo, custIndex);
				Release(nextTokenNumberLock);

				/* eat-in customer always waits	*/			
	
				/* Acquire the foodToBagLock to increment the foodToBag count	*/
				/* Release the foodToBagLock after updating the foodToBag count	*/
				
				/* Order taker has to update the foodToBeCooked & foodReady database	*/
				/* if 6$burger is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].sixBurger == 1){
					Release(customerDataLock);
					/* if 6$burger is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.sixBurger != 0){
					Release(foodReadyDBLock);
						/*reserve a burger for this customer	*/
						Acquire(customerDataLock);
						custData[custIndex].sixBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.sixBurger > MINCOOKEDFOOD){
							foodReadyData.sixBurger--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.sixBurger--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then 	*/
						/* bag it and increment the food to be cooked	*/
						else{
							foodReadyData.sixBurger--;	
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/
						}
					}
	 				else 
						custHasToWait = 1;
						Release(foodReadyDBLock);
				}
				else
				Release(customerDataLock);
				
				/* if 3$burger is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].threeBurger == 1){
					Release(customerDataLock);
					/* if 3$burger is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.threeBurger != 0){
						Release(foodReadyDBLock);
						/*reserve a burger for this customer	*/
						Acquire(customerDataLock);
						custData[custIndex].threeBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.threeBurger > MINCOOKEDFOOD){
						foodReadyData.threeBurger--;
						Release(foodReadyDBLock);
						Acquire(foodToBeCookedDBLock);
						foodToBeCookedData.threeBurger--;
						Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then 	*/
						/* bag it and do not decrement the food to be cooked	*/
						else{
							foodReadyData.threeBurger--;
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/						
						}						
					}	
					else{ /* customer has to wait	*/
							custHasToWait = 1;
							Release(foodReadyDBLock);
					}
				}
				else
				Release(customerDataLock);
				
			/* if vegburger is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].vegBurger == 1){
					Release(customerDataLock);
					/* if vegburger is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.vegBurger != 0){
					Release(foodReadyDBLock);
						/*reserve a burger for this customer*/
						Acquire(customerDataLock);
						custData[custIndex].vegBurger = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.vegBurger > MINCOOKEDFOOD){
							foodReadyData.vegBurger--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.vegBurger--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then 	*/
						/* bag it and do not decrement the food to be cooked	*/
						else{
							foodReadyData.vegBurger--;	
							Release(foodReadyDBLock);
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/
						}	
					}
					else {/* customer has to wait	*/
						custHasToWait = 1;
						Release(foodReadyDBLock);
					}
				}
				else
				Release(customerDataLock);
				
				/* if fries is ordered	*/
				Acquire(customerDataLock);
				if(custData[custIndex].fries == 1){
				Release(customerDataLock);
					/* if fries is ready	*/
					Acquire(foodReadyDBLock);
					if(foodReadyData.fries != 0){
					Release(foodReadyDBLock);
						/*reserve a fries for this customer	*/
						Acquire(customerDataLock);
						custData[custIndex].fries = 2;
						Release(customerDataLock);
						/* if greater than minimum amount of cooked food	*/
						/* then bag it, no worries	*/
						Acquire(foodReadyDBLock);
						if(foodReadyData.fries > MINCOOKEDFOOD){
							foodReadyData.fries--;	
							Release(foodReadyDBLock);
							Acquire(foodToBeCookedDBLock);
							foodToBeCookedData.fries--;
							Release(foodToBeCookedDBLock);
						}
						/* else if it is less than minimum then 	*/
						/* bag it and do not decrement the food to be cooked	*/
						else{
							foodReadyData.fries--;
							Release(foodReadyDBLock);							
							/* Do not decrement the foodtoBeCookedData because the	*/
							/* amount of cooked food is less than minimum	*/
						}		
					}
					else{ /* customer has to wait	*/
						custHasToWait = 1;
						Release(foodReadyDBLock);
					}
				}
				else 
				Release(customerDataLock);
				
				if (custHasToWait = 0){
			
/*CHANGE:ADD ============*/	Acquire(BagTheOrdersLock);
					custData[custIndex].bagged = 1;
					
/*CHANGE:ADD ============*/	Release(BagTheOrdersLock);
				
					Acquire(foodBaggedListLock);
					for(indexj = 1; indexj <= CUST_COUNT; indexj++){	
						if(baggedList[indexj] == 0){
						baggedList[indexj] = custData[custIndex].tokenNo;
						break;
						}
					}
					/* OG	*/
					Print_3Arg("Ordertaker %d gives Token number %d to Waiter for EATIN Customer %d rightnow \n$", myIndex, custData[custIndex].tokenNo, custIndex);
					Release(foodBaggedListLock);
				}else{
					/* Acquire the foodToBagLock to increment the foodToBag count	*/
					Acquire(foodToBagLock);
					foodToBag++;
					/* Release the foodToBagLock after updating the foodToBag count	*/
					Release(foodToBagLock);
				}
			}
		
			Acquire(customerDataLock);
			if(custData[custIndex].sixBurger == 2){
				Release(customerDataLock);
				Acquire(foodToBeCookedDBLock);
				foodToBeCookedData.sixBurger++;
				Release(foodToBeCookedDBLock);
			}else
				Release(customerDataLock);
			
			Acquire(customerDataLock);
			if(custData[custIndex].threeBurger == 2){
				Release(customerDataLock);
				Acquire(foodToBeCookedDBLock);
				foodToBeCookedData.threeBurger++;
				Release(foodToBeCookedDBLock);
			}else
				Release(customerDataLock);
			
			Acquire(customerDataLock);
			if(custData[custIndex].vegBurger == 2){
				Release(customerDataLock);
				Acquire(foodToBeCookedDBLock);
				foodToBeCookedData.vegBurger++;
				Release(foodToBeCookedDBLock);
			}else
				Release(foodToBeCookedDBLock);
			
			Acquire(customerDataLock);
			if(custData[custIndex].fries == 2){
				Release(customerDataLock);
				Acquire(foodToBeCookedDBLock);
				foodToBeCookedData.fries++;
				Release(foodToBeCookedDBLock);
			}else
			Release(foodToBeCookedDBLock);
 
			
			Acquire(orderTakerLock[myIndex]);
			/* Send a signal to the Customer which indicates that the 	*/
			/* Order Taker has processed the order	*/
			Signal(orderTakerCV[myIndex], orderTakerLock[myIndex]);
				
			/* wait for the customer to pay money	*/
			Wait(orderTakerCV[myIndex], orderTakerLock[myIndex]);
			Release(orderTakerLock[myIndex]);
				
			Acquire(custServedLock);
			if(custServed == CUST_COUNT){
				Release(custServedLock);
				/* @ */
				/* Print_1Arg("OrderTaker %d calling E---X---I---T\n$", myIndex); */
				Exit(0);
			}
			Release(custServedLock);
			
			/* received signal from customer which indicates that the customer	*/
			/* has paid the money	*/
			/* Time to put the money received in a place where all money is	*/
			/* stored by all the Order takers. This money will be used by the	*/ 
			/* manager to refill the inventory	*/
			Acquire(customerDataLock);
			if(custData[custIndex].sixBurger > 0)
				billAmount += 6;	/* 6$Burger	*/
			if(custData[custIndex].threeBurger > 0)
				billAmount += 3;	/* 3$Burger	*/
			if(custData[custIndex].vegBurger > 0)
				billAmount += 8;	/* vegBurger costs 8$ */
			if(custData[custIndex].fries > 0)
				billAmount += 2;	/* fries costs 2$ */
			if(custData[custIndex].soda > 0)
				billAmount += 1;	/* soda costs 1$ */
			Release(customerDataLock);
			
			Acquire(moneyAtRestaurantLock);
			moneyAtRestaurant += billAmount;
			/* release the lock after storing the money	*/
			Release(moneyAtRestaurantLock);
			/* Send a signal to the Customer which indicates that the 	*/
			/* Order Taker has processed the order	*/
			Acquire(orderTakerLock[myIndex]);
			Signal(orderTakerCV[myIndex],orderTakerLock[myIndex]);	
			/* interaction with the current customer is completed so the 	*/
			/* Order Taker releases the Lock	*/
			Release(orderTakerLock[myIndex]);
			
			Acquire(custServedLock);
			if(custServed == CUST_COUNT){
				Release(custServedLock);
				/* @ */
				/* Print_1Arg("OrderTaker %d calling E---X---I---T\n$", myIndex); */
				Exit(0);
			}
			Release(custServedLock);	
		}
		
	}   /* While(1) 	*/
}	


/* Waiter thread body */  
void Waiter()
{
	int index = 0, indexj = 0;
	int tableNo = 0;
	int baggedListNotEmpty = 0;
	int custHasNoTable = 0;
	int myIndex = 0;
	
	/* Get the Index for this Cook before starting the job */
	Acquire(nextWaiterIndexLock);
	myIndex = nextWaiterIndex;
	nextWaiterIndex++;
	Release(nextWaiterIndexLock);
	
	while(1){
		
		index = 0;
		tableNo = 0;
		baggedListNotEmpty = 0;
		/* Acquire Lock to check if any orders are bagged*/
		Acquire(foodBaggedListLock);
		
		/* check if token numbers of bagged food are present in bagged list*/
		for(index = 1; index <= CUST_COUNT; index++){
			if(baggedList[index] != 0){
				baggedListNotEmpty = baggedList[index];
				baggedList[index] = 0;
				break;
			}
		}
		
		Release(foodBaggedListLock);
		
		if(baggedListNotEmpty != 0){
			int firstTokenNo = 0;
			
			/* remove first token number from the bagged list */
			/* to serve it for the customer waiting for it*/
			firstTokenNo = baggedListNotEmpty;
			/* Acquire tableDataLock to obtain the table number of the */
			/* Eat-in seated customer */			
			Acquire(tablesDataLock);
	
			/* Determine the table number of the Eat-in Customer*/
			
			for(index = 1; index <= TABLE_COUNT; index++){
				/* Find the table number using token number removed from */
				/* bagged list*/
				if(tables[index] == firstTokenNo){
					tableNo = index;
				    break;
				}
			}
			
			Release(tablesDataLock);
			
			/* Search for a particular token number with all customers */
			for(index = 1; index <= CUST_COUNT; index++){
				custHasNoTable = 0;
				/* If token number of customer matches set the delivered which*/
				/* indicates the food has been served.*/
				
				Acquire(customerDataLock);
				if(custData[index].tokenNo == firstTokenNo){
					/* check if the customer is seated */
					if(custData[index].tableNo != 0){ 
					
						/* OG */
						Print_2Arg("Waiter %d validates the token number for Customer %d and serves the food\n$", myIndex, index);
						
						
						if(custData[index].bagged == 1)
							custData[index].delivered = 1;
							
						Release(customerDataLock);
						/* OG */
						Print_2Arg("Waiter %d serves food to Customer %d\n$", myIndex, index);
						/* Notifying the Eatin seated customer that the order */
						/* has been delivered*/
						Acquire(eatInFoodWaitingLock);
						
						/* Broadcasting the notification about the order*/
						/* delivered. This signal will be processed only by*/
						/* that customer whose order was bagged, others*/
						/* will go back to wait again.*/
						Broadcast(eatInFoodWaitingCV, eatInFoodWaitingLock);
						Release(eatInFoodWaitingLock);
					}
					/* else if the customer has not yet got his table to sit*/
					/* then put back the bagged order into the List*/
					else{ 
						Release(customerDataLock);
						Acquire(foodBaggedListLock);
						for(indexj = 1; indexj <= CUST_COUNT; indexj++){
							if(baggedList[indexj] == 0){
								baggedList[indexj] = firstTokenNo;
								custHasNoTable = 1;
								break;
							
							}
						}
						Release(foodBaggedListLock);
					}	
					if(custHasNoTable == 0)
						break;
				}
				else
					Release(customerDataLock);
			}
		}
		if((baggedListNotEmpty == 0) || (custHasNoTable == 1)){
			/* Waiter Acquires Lock and Goes on wait (break) till he receives */
			/* a wake up signal from the manager */
			Acquire(waiterSleepLock);
			/* OG */
			Print_1Arg("Waiter %d is going on break\n$", myIndex);
			Wait(waiterSleepCV,waiterSleepLock);
			/* Received a signal from the manager, waiter starts with his job */
			Release(waiterSleepLock);
			/* Check if all the Customers are serviced */
			Acquire(custServedLock);
			if(custServed == CUST_COUNT){			
				Release(custServedLock);
				/* @ */
				/* Print_1Arg("Waiter %d calling E---X---I---T\n$", myIndex); */
				Exit(0);
			}
			Release(custServedLock);
			/* OG */
			Print_1Arg("Waiter %d returned from break\n$", myIndex);
		}
	}
}


/* Cook thread body */ 
void Cook()
{	
	int myIndex = 0;
	int whatToCook = 0;
	
	/* Get the Index for this Cook before starting the job */
	Acquire(nextCookIndexLock);
	myIndex = nextCookIndex ;
	nextCookIndex++;
	Release(nextCookIndexLock);

	/* To know what the manager wants this cook to cook */
	Acquire(whatToCookFirstLock);
	while(whatToCookFirst == 0){
		Release(whatToCookFirstLock);	
		Yield();
		Acquire(whatToCookFirstLock);
	}
	whatToCook = whatToCookFirst;
	whatToCookFirst = 0;
	Release(whatToCookFirstLock);
	
	/* Acquire a lock on inventory to check status*/
	while(1){	
		/* Check if the stop cooking flag is set by the manager*/
		/* Acquire the current cooking item lock*/
		/* Check if the stop cooking flag is set*/
		/* if set then it indicates manager wants the Cook to go on break*/
		/* Add the current cook thread to the cooks on break queue*/
		/* Cooks goes on break (sleep)*/
		
		if(whatToCook == SIXBURGER){
			Acquire(stopSixBurgerLock);
			
			if(stopSixBurger == 1){
				Release(stopSixBurgerLock);
				/* Increase the cooks on break Count to indicate the manager who checks this value
					before hiring a new cook */
				Acquire(cooksOnBreakCountLock);
				cooksOnBreakCount++;
				Release(cooksOnBreakCountLock);
				
				Acquire(workingCookLock);
				workingCooks[whatToCook]--;
				Release(workingCookLock);
				
				/* OG @ */
				Print_2Arg("Cook %d is going on break\n$", myIndex, whatToCook);
				
				Acquire(wakeUpCookLock);
				Wait(wakeUpCookCV,wakeUpCookLock);
				Release(wakeUpCookLock);
				
				Acquire(custServedLock);
				if(custServed == CUST_COUNT){
					Release(custServedLock);
					/* @ */
					/* Print_1Arg("Cook %d calling E---X---I---T\n$", myIndex); */
					Exit(0);
				}
				Release(custServedLock);
				
				/* cook is back from break*/
				/* Acquire the what to cook next lock and check */
				/* what to cook next*/
				Acquire(whatToCookNextLock);
				/* update what the cook was cooking before going to */
				/* sleep with what he has to cook after the break*/
				while(whatToCookNext == 0){
					Release(whatToCookNextLock);
					Yield();
					Acquire(whatToCookNextLock);
				}	
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				/* release the lock after setting the value*/
				Release(whatToCookNextLock);
				
				/* OG */
				Print_2Arg("Cook %d returned from break\n$", myIndex, whatToCook);	
			}else{	
				Release(stopSixBurgerLock);
			}	
		}
		else if(whatToCook == THREEBURGER){
			Acquire(stopThreeBurgerLock);
			if(stopThreeBurger == 1){
				Release(stopThreeBurgerLock);
				/* Increase the cooks on break Count to indicate the manager who checks this value
					before hiring a new cook */
				Acquire(cooksOnBreakCountLock);
				cooksOnBreakCount++;
				Release(cooksOnBreakCountLock);
				
				Acquire(workingCookLock);
				workingCooks[whatToCook]--;
				Release(workingCookLock);
				
				/* OG */
				Print_2Arg("Cook %d is going on break\n$", myIndex, whatToCook);
				
				Acquire(wakeUpCookLock);
				Wait(wakeUpCookCV, wakeUpCookLock);
				Release(wakeUpCookLock);
				
				Acquire(custServedLock);
				if(custServed == CUST_COUNT){
					Release(custServedLock);
					/* @ */
					/* Print_1Arg("Cook %d calling E---X---I---T\n$", myIndex); */
					Exit(0);
				}
				Release(custServedLock);
				/* cook is back from break*/
				/* Acquire the what to cook next lock and check */
				/* what to cook next*/
				Acquire(whatToCookNextLock);
				/* update what the cook was cooking before going to */
				/* sleep with what he has to cook after the break*/
				while(whatToCookNext == 0){
					Release(whatToCookNextLock);
					Yield();
					Acquire(whatToCookNextLock);
				}	
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				/* release the lock after setting the value*/
				Release(whatToCookNextLock);
				
				/* OG*/
				Print_2Arg("Cook %d returned from break\n$", myIndex, whatToCook);
				
			}else{	
				Release(stopThreeBurgerLock);
			}	
		}
		else if(whatToCook == VEGBURGER){
		
			Acquire(stopVegBurgerLock);
			if(stopVegBurger == 1){
			
				Release(stopVegBurgerLock);
				/* Increase the cooks on break Count to indicate the manager who checks this value
					before hiring a new cook */
				Acquire(cooksOnBreakCountLock);
				cooksOnBreakCount++;
				Release(cooksOnBreakCountLock);
				
				Acquire(workingCookLock);
				workingCooks[whatToCook]--;
				Release(workingCookLock);
				
				/* OG*/
				Print_2Arg("Cook %d is going on break\n$", myIndex, whatToCook);
				Acquire(wakeUpCookLock);
				
				Wait(wakeUpCookCV,wakeUpCookLock);
				Release(wakeUpCookLock);
				
				Acquire(custServedLock);
				if(custServed == CUST_COUNT){
					Release(custServedLock);
					/* @ */
					/* Print_1Arg("Cook %d calling E---X---I---T\n$", myIndex); */
					Exit(0);
				}
				Release(custServedLock);
				/* cook is back from break*/
				/* Acquire the what to cook next lock and check */
				/* what to cook next*/
				Acquire(whatToCookNextLock);
				/* update what the cook was cooking before going to */
				/* sleep with what he has to cook after the break*/
				while(whatToCookNext == 0){
					Release(whatToCookNextLock);
					Yield();
					Acquire(whatToCookNextLock);
				}	
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				/* release the lock after setting the value*/
				Release(whatToCookNextLock);
				
				/* OG*/
				Print_2Arg("Cook %d returned from break\n$", myIndex, whatToCook);
				
			}else{
				Release(stopVegBurgerLock);
			}	
		}
		else if(whatToCook == FRIES){
			Acquire(stopFriesLock);
			
			if(stopFries == 1){
				Release(stopFriesLock);
				
				/* Increase the cooks on break Count to indicate the manager who checks this value
					before hiring a new cook */
					
				Acquire(cooksOnBreakCountLock);
				cooksOnBreakCount++;
				Release(cooksOnBreakCountLock);
				
				Acquire(workingCookLock);
				workingCooks[whatToCook]--;
				Release(workingCookLock);
				
				/* OG*/
				Print_2Arg("Cook %d is going on break\n$", myIndex, whatToCook);
				Acquire(wakeUpCookLock);
				Wait(wakeUpCookCV, wakeUpCookLock);
				Release(wakeUpCookLock);
				
				
				Acquire(custServedLock);
				if(custServed == CUST_COUNT){
					Release(custServedLock);
					/* @ */
					/* Print_1Arg("Cook %d calling E---X---I---T\n$", myIndex); */
					Exit(0);
				}
				Release(custServedLock);
				
				/* cook is back from break*/
				/* Acquire the what to cook next lock and check */
				/* what to cook next*/
				Acquire(whatToCookNextLock);
				/* update what the cook was cooking before going to */
				/* sleep with what he has to cook after the break*/
				while(whatToCookNext == 0){
					Release(whatToCookNextLock);
					Yield();
					Acquire(whatToCookNextLock);
				}	
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				/* release the lock after setting the value*/
				Release(whatToCookNextLock);
				
				/* OG*/
				Print_2Arg("Cook %d returned from break\n$", myIndex, whatToCook);
				
			}else{
				Release(stopFriesLock);
			}	
		} 
		
		/* If all the customers are serviced then the cooks will Go Home*/
		/* Check if all the Customers are serviced*/
		Acquire(custServedLock);
		if(custServed == CUST_COUNT){
			Release(custServedLock);
			/* @ */
			/* Print_1Arg("Cook %d calling E---X---I---T\n$", myIndex); */
			Exit(0);
		}
		Release(custServedLock);
		
		/* decrement one from the inventory*/
		Acquire(inventoryLock);
		while(inventory == 0){
			Release(inventoryLock);
			Yield();
			Acquire(inventoryLock);
		}	
		inventory--;
		Release(inventoryLock);
		
		/* check what Manager has ordered the cook to prepare.*/
		/* Increase the food count in Ready DataBase*/
		if(whatToCook == SIXBURGER){
			/* OG*/
			Print_1Arg("Cook %d is going to cook 6-dollar burger\n$", myIndex);
			Acquire(foodReadyDBLock);
			foodReadyData.sixBurger++; 
			Release(foodReadyDBLock);
		}
		
		if(whatToCook == THREEBURGER){		
			/* OG*/
			Print_1Arg("Cook %d is going to cook 3-dollar burger\n$", myIndex);
			Acquire(foodReadyDBLock);
			foodReadyData.threeBurger++;		
			Release(foodReadyDBLock);
		}
		
		if(whatToCook == VEGBURGER){
			/* OG*/
			Print_1Arg("Cook %d is going to cook veggie burger\n$", myIndex);	 
			Acquire(foodReadyDBLock);
			foodReadyData.vegBurger++; 
			Release(foodReadyDBLock);
		}
		
		if(whatToCook == FRIES){
			/* OG*/
			Print_1Arg("Cook %d is going to cook french fries\n$", myIndex);	
			Acquire(foodReadyDBLock);
			foodReadyData.fries++;  
			Release(foodReadyDBLock);
		}	
		
		/* cooks takes some Time to prepare the food that */
		/* is instructed by the manager */
		Yield(); 
	}
} 

/* Manager Thread body */
void Manager()
{
	int inventoryRefillCost = 50;
	int timeToGoToBank = 10;
	int tablesFree = 0;
	int foodRequired[5];
	int cookCount = 0;
	int index = 0, indexj = 0;
	int baggedListNotEmpty = 0;

    while(1){
		/* Check if all the Customers are serviced*/
		/* @ */
		/* Print_Stmt("Manager checking inventory\n$"); */
		Acquire(custServedLock);
		if(custServed == CUST_COUNT){
			Release(custServedLock);
			
			/* Waking up all the waiters and telling them to go Home*/
			Acquire(waiterSleepLock);
			Broadcast(waiterSleepCV,waiterSleepLock);
			Release(waiterSleepLock);
			
			/* Waking up all the Cooks and telling them to go Home*/
			Acquire(wakeUpCookLock);
			Broadcast(wakeUpCookCV, wakeUpCookLock);
			Release(wakeUpCookLock);
			
			/* Signal all the ordertakers and tell them to go home*/
			for(index = 1; index <= OT_COUNT; index++)
			{
				Acquire(orderTakerLock[index]);
				Signal(orderTakerCV[index],orderTakerLock[index]);
				Release(orderTakerLock[index]);
			}
			/* @ */
			Print_Stmt("\n\n\t-------------------------------------------------------------\n");
			Print_Stmt("\n\tCARL'S JR RESTAURANT SIMULATION COMPLETED SUCCESSFULLY\n$");
			Print_Stmt("\n\t-------------------------------------------------------------\n\n");
			Exit(0);
		}
		Release(custServedLock);
		
		/* Acquire the Inventory Lock */
		Acquire(inventoryLock);
		/* Ckeck the inventory Level*/
		if(inventory <= MININVENTORY){
			/* take a Lock to access the common location where all the money*/
			/* is stored by the OrderTakers*/
			Release(inventoryLock);
			Acquire(moneyAtRestaurantLock);
			if(moneyAtRestaurant < inventoryRefillCost){
				/* Take all the money in the restaurant*/
				moneyAtRestaurant = 0;
				
				/* Manager goes to bank to withdraw the remaining money*/
				/* This process of going to bank and withdrawing will take*/
				/* a minimum of five times the time */
				/* OG*/
				Print_Stmt("Manager goes to bank to withdraw the cash\n$");
				Release(moneyAtRestaurantLock);
				for(index = 1; index <= timeToGoToBank; index++){
					Yield();
				}					
				Acquire(moneyAtRestaurantLock);
				/* OG*/
				Print_Stmt("moneyAtRestautant is loaded in the restaurant\n$");
			}		
			moneyAtRestaurant = moneyAtRestaurant - inventoryRefillCost;									
			Release(moneyAtRestaurantLock);
			/* Once He comes Back from the Bank The Inventory will be filled*/
			Acquire(inventoryLock);
				inventory = FILLED;
			/* OG */
			Print_Stmt("Manager refills the inventory\n$");
			Release(inventoryLock);
		}else
			Release(inventoryLock);
		
		tablesFree = 0;
		/* Manager Interaction With The Customer*/
		Acquire(managerLock);
		if(managerLineLength > 0){
			managerLineLength--;
			Release(managerLock);
			/* Acquire the tables data lock to check the tables free*/
			Acquire(tablesDataLock);
			/* Find the Number of Tables available free*/
			for(index = 1; index <= TABLE_COUNT; index++) {
				if(tables[index] == 0){
					tablesFree = 1;
					break;
				}
			}
			Release(tablesDataLock);
			Acquire(managerLock);
			if(tablesFree == 1){
				while(tableAvailable != 0){
					Release(managerLock);
					Yield();
					Acquire(managerLock);
				}
				tableAvailable = 1;	
				Signal(managerCV,managerLock);
			}else{
				while(tableAvailable != 0){
					Release(managerLock);
					Yield();
					Acquire(managerLock);
				}
				tableAvailable = 0;
				Signal(managerCV,managerLock);
			}				
		} 
		Release(managerLock);
		/* Manager interaction with the cook*/
		
		/* Acquire the ready food and food to be cooked locks so that the manager*/
		/* can decide whether to make a cook go on break or bring back a cook on*/
		/* break or to hire a new cook*/
		Acquire(foodToBeCookedDBLock);
		foodRequired[SIXBURGER] = foodToBeCookedData.sixBurger;
		foodRequired[THREEBURGER] = foodToBeCookedData.threeBurger;
		foodRequired[VEGBURGER] = foodToBeCookedData.vegBurger ;
		foodRequired[FRIES] = foodToBeCookedData.fries;
		Release(foodToBeCookedDBLock);
		
		Acquire(foodReadyDBLock);
		foodRequired[SIXBURGER] = (foodRequired[SIXBURGER] - (foodReadyData.sixBurger - MINCOOKEDFOOD));
		foodRequired[THREEBURGER] = (foodRequired[THREEBURGER] - (foodReadyData.threeBurger  - MINCOOKEDFOOD));
		foodRequired[VEGBURGER] = (foodRequired[VEGBURGER] - (foodReadyData.vegBurger - MINCOOKEDFOOD));
		foodRequired[FRIES] = (foodRequired[FRIES] - (foodReadyData.fries - MINCOOKEDFOOD));
		Release(foodReadyDBLock);

		/* For all the items in the restaurant*/
		for(index = 1; index < 5; index ++){
			/* if foodToBeCooked is  a little greater than the foodready*/
			if((foodRequired[index] <= MAXDIFF) && (foodRequired[index] > 0)){
				/* Reset the stop cooking flag for this item if it is set*/
				if(index == SIXBURGER){
					Acquire(stopSixBurgerLock);
					stopSixBurger = 0;
					Release(stopSixBurgerLock);
				}
				else if(index == THREEBURGER){
					Acquire(stopThreeBurgerLock);
					stopThreeBurger = 0;
					Release(stopThreeBurgerLock);
				}
				else if(index == VEGBURGER){
					Acquire(stopVegBurgerLock);
					stopVegBurger = 0;
					Release(stopVegBurgerLock);
				}
				else if(index == FRIES){
					Acquire(stopFriesLock);
					stopFries = 0;
					Release(stopFriesLock);
				}
				/* if there are no working cooks for this item*/
				Acquire(workingCookLock);
				if(workingCooks[index] == 0){
					Release(workingCookLock);
					/* if there are no cooks on break*/
					Acquire(cooksOnBreakCountLock);
					/* if cooks on break count is 0 then hire a new cook */
					if(cooksOnBreakCount == 0){
						Release(cooksOnBreakCountLock);
						/* Then Create a new Cook*/
						Acquire(whatToCookFirstLock);
						while(whatToCookFirst != 0){
							Release(whatToCookFirstLock);
							Yield();
							Acquire(whatToCookFirstLock);
						}
						whatToCookFirst = index;
						Release(whatToCookFirstLock);
						if(cookCount < COOK_COUNT){
							Fork(Cook);
							cookCount++;
							Acquire(workingCookLock);
							workingCooks[index]++;
							Release(workingCookLock);
						}
					}
					else{
						cooksOnBreakCount--;
						Release(cooksOnBreakCountLock);
						/* bring back the cook on break*/
						/* Acquire the what to cook next lock and tell the cook*/
						/* who is back from the break "what to cook next"*/
						Acquire(whatToCookNextLock);
						/* make what to cook next as the current food item*/
						while(whatToCookNext != 0){
							Release(whatToCookNextLock);
							Yield();
							Acquire(whatToCookNextLock);
						}
						whatToCookNext = index;
						/* release the lock after setting the value*/
						Release(whatToCookNextLock);
				
						Acquire(wakeUpCookLock);
						Signal(wakeUpCookCV, wakeUpCookLock);
						Release(wakeUpCookLock);
						/* increment working cooks for this item by 1*/
						Acquire(workingCookLock);
						workingCooks[index]++;
						Release(workingCookLock);
					}		
				}else 
					Release(workingCookLock);
			}		
			/* else if the food required is greater than the maximum difference*/ 
			/* that can be handled by the current cooks*/
			else if((foodRequired[index] > MAXDIFF)){
				/* Reset the stop cooking flag for this item if it is set */
				if(index == SIXBURGER){
					Acquire(stopSixBurgerLock);
					stopSixBurger = 0;
					Release(stopSixBurgerLock);
				}
				else if(index == THREEBURGER){
					Acquire(stopThreeBurgerLock);
					stopThreeBurger = 0;
					Release(stopThreeBurgerLock);
				}
				else if(index == VEGBURGER){
					Acquire(stopVegBurgerLock);
					stopVegBurger = 0;
					Release(stopVegBurgerLock);
				}
				else if(index == FRIES){
					Acquire(stopFriesLock);
					stopFries = 0;					
					Release(stopFriesLock);
				}
				
				/* if no cooks are on break */
				Acquire(cooksOnBreakCountLock);
				/* if cooks on break count is 0 then hire a new cook */
				if(cooksOnBreakCount == 0){
					Release(cooksOnBreakCountLock);
					/* if a new cook can be created*/
					if(cookCount < COOK_COUNT){
						/* Create a new Cook*/
						Acquire(whatToCookFirstLock);
						while(whatToCookFirst != 0){
							Release(whatToCookFirstLock);
							Yield();
							Acquire(whatToCookFirstLock);
						}
						whatToCookFirst = index;
						Release(whatToCookFirstLock);
						Fork(Cook);
						
						cookCount++;
						Acquire(workingCookLock);
						workingCooks[index]++;
						Release(workingCookLock);
					}
				}
				else{
					cooksOnBreakCount--;
					Release(cooksOnBreakCountLock);
					/* bring back the cook on break*/
					/* Acquire the what to cook next lock and tell the cook*/
					/* who is back from the break "what to cook next"*/
					Acquire(whatToCookNextLock);
					/* make what to cook next as the current food item*/
					while(whatToCookNext != 0){
						Release(whatToCookNextLock);
						Yield();
						Acquire(whatToCookNextLock);
					}
					whatToCookNext = index;
					/* release the lock after setting the value*/
					Release(whatToCookNextLock);
					/* call the cook on break back to work*/ 
					Acquire(wakeUpCookLock);
					Signal(wakeUpCookCV,wakeUpCookLock);
					Release(wakeUpCookLock);

					/* increment working cooks for this item by 1*/
					Acquire(workingCookLock);
					workingCooks[index]++;
					Release(workingCookLock);
				}
			}/* if the food ready is greater than the required then */
			/* set the stop cooking monitor for that item */
			else if((foodRequired[index] < 0)){
				if(index == SIXBURGER){
					Acquire(stopSixBurgerLock);
					stopSixBurger = 1;
					Release(stopSixBurgerLock);
				}
				else if(index == THREEBURGER){
					Acquire(stopThreeBurgerLock);
					stopThreeBurger = 1;
					Release(stopThreeBurgerLock);
				}
				else if(index == VEGBURGER){
					Acquire(stopVegBurgerLock);
					stopVegBurger = 1;
					Release(stopVegBurgerLock);
				}
				else if(index == FRIES){
					Acquire(stopFriesLock);
					stopFries = 1;
					Release(stopFriesLock);
					
				}
			}
		}/* for all the items*/
		
		Acquire(foodBaggedListLock);
		baggedListNotEmpty = 0;
		for(indexj = 1; indexj <= CUST_COUNT; indexj++){
			if(baggedList[indexj] != 0){
				baggedListNotEmpty = 1;
				break;
			}
		}
		Release(foodBaggedListLock);
		
		if(baggedListNotEmpty == 1 ){
			/* Manager Acquires Lock and signals the waiters waiting */
			Acquire(waiterSleepLock);
			Broadcast(waiterSleepCV,waiterSleepLock);
			/* OG */
			Print_Stmt("Manager calls back all Waiters from break\n");
			Release(waiterSleepLock);
		}	
		
		Yield();
		
    }/* while	*/
}				


void main()
{
	int index = 0;
	int otCount = 0, custCount = 0, cookCount = 0, waiterCount = 0;
	
	/* User Input for the number of entities to be present in the simulation	*/
	
	/* Receive the number of Order Takers to be present in the simulation	*/
	Print_Stmt("\nEnter the Number of OrderTakers in Carls Jr \n$");		 
	otCount = Scan("%d");
	OT_COUNT = otCount;
	
	/* Receive the number of Waiters to be present in the simulation	*/
	Print_Stmt("\nEnter the Number of Waiters in Carls Jr \n$");			 
	waiterCount = Scan("%d");
	WAITER_COUNT = waiterCount;
	
	/* Receive the number of Cooks to be present in the simulation	*/
	Print_Stmt("\nEnter the Number of Cooks in Carls Jr \n$");			 
	cookCount = Scan("%d");
	COOK_COUNT = cookCount;
	
	/* Receive the number of Customers to be present in the simulation	*/
	Print_Stmt("\nEnter the Number of Customers in Carls Jr \n$");		 
	custCount = Scan("%d");
	CUST_COUNT = custCount; 
	
	/*------------------------------------------------------------	*/
	/* Create all the Locks using Syscalls	*/
	/*------------------------------------------------------------	*/
	custLineLock = CreateLock("custLineLock", 12);
	foodReadyDBLock = CreateLock("foodReadyDBLock", 15);
	foodToBeCookedDBLock = CreateLock("foodToBeCookedDBLock", 20);
	nextTokenNumberLock = CreateLock("nextTokenNumberLock", 19);
	moneyAtRestaurantLock = CreateLock("moneyAtRestaurantLock", 21);
	waitToGoLock = CreateLock("waitToGoLock", 12);
	tablesDataLock = CreateLock("tablesDataLock", 14);
	eatInWaitingLock = CreateLock("eatInWaitingLock", 16);
	eatInFoodWaitingLock = CreateLock("eatInFoodWaitingLock", 20);
	foodToBagLock = CreateLock("foodToBagLock", 13);
	foodBaggedListLock = CreateLock("foodBaggedListLock", 18);
	inventoryLock = CreateLock("inventoryLock", 13);
	managerLock = CreateLock("managerLock", 11);
	whatToCookNextLock = CreateLock("whatToCookNextLock", 18);
	stopSixBurgerLock = CreateLock("stopSixBurgerLock", 18);
	stopThreeBurgerLock = CreateLock("stopThreeBurgerLock", 20);
	stopVegBurgerLock = CreateLock("stopVegBurgerLock", 17);
	stopFriesLock = CreateLock("stopFriesLock", 13);
	waiterSleepLock = CreateLock("waiterSleepLock", 15);
	customerDataLock = CreateLock("customerDataLock", 16);
	wakeUpCookLock = CreateLock("wakeUpCookLock", 14);
	custServedLock = CreateLock("custServedLock", 14);
	nextWaiterIndexLock = CreateLock("nextWaiterIndexLock", 19);
	nextCookIndexLock = CreateLock("nextCookIndexLock", 17);
	nextCustomerIndexLock = CreateLock("nextCustomerIndexLock", 21);
	nextOrderTakerIndexLock = CreateLock("nextOrderTakerIndexLock", 23);
	whatToCookFirstLock = CreateLock("whatToCookFirstLock", 19); 
	cooksOnBreakCountLock = CreateLock("cooksOnBreakCountLock", 21);
	workingCookLock = CreateLock("workingCookLock", 15);
	BagTheOrdersLock	= CreateLock("BagTheOrdersLock", 16);	
	
	/* Create OT_COUNT number of locks, one Lock for each Ordertaker	*/
	for(index = 1; index <= OT_COUNT; index++)
		orderTakerLock[index] = CreateLock("orderTakerLock", 14);
	
	orderTakerLock[(OT_COUNT + 1)] = CreateLock("managerOtLock", 13);
	
	/*------------------------------------------------------------	*/
	/* Create all the Condition variables using Syscalls	*/
	/*------------------------------------------------------------	*/
	custLineCV = CreateCV("custLineCV", 10);
	tablesDataCV = CreateCV("tablesDataCV", 12); 
	eatInWaitingCV = CreateCV("eatInWaitingCV", 14);
	eatInFoodWaitingCV = CreateCV("eatInFoodWaitingCV", 18);
	managerCV = CreateCV("managerCV", 9);
	waiterSleepCV = CreateCV("waiterSleepCV", 13);
	wakeUpCookCV = CreateCV("wakeUpCookCV", 12);
	toGoCV = CreateCV("toGoCV", 6); 
	/* Create OT_COUNT number of Cvs, one CV for each Ordertaker	*/
	for(index = 1; index <= OT_COUNT; index++)
		orderTakerCV[index] = CreateCV("orderTakerCV", 12);
		
	orderTakerCV[(OT_COUNT + 1)] = CreateCV("ManagerOT", 9);
	
	/*------------------------------------------------------------	*/
	/*Printing the Initial Values before start of Simulation	*/
	/*------------------------------------------------------------	*/
	Print_1Arg("o Number of Ordertakers = %d\n$", OT_COUNT);
	Print_1Arg("o Number of Waiters = %d\n$", WAITER_COUNT);
	Print_1Arg("o Number of Cooks = %d\n$", COOK_COUNT);
	Print_1Arg("o Number of Customers = %d\n$", CUST_COUNT);
	Print_Stmt("\n Restaurant\n$");
	Print_1Arg("\t o Total Number of tables in the Restaurant = %d\n$", TABLE_COUNT);
	Print_1Arg("\t o Minimum Number of cooked 6-dollar burger = %d\n$", MINCOOKEDFOOD);
	Print_1Arg("\t o Minimum Number of cooked 3-dollar burger = %d\n$", MINCOOKEDFOOD);
	Print_1Arg("\t o Minimum Number of cooked veggie burger = %d\n$", MINCOOKEDFOOD);
	Print_1Arg("\t o Minimum Number of cooked french fries = %d\n$", MINCOOKEDFOOD);
	
	Acquire(customerDataLock);
	/* random order is generated for all the customers	*/
	/* generateCustomerOrder(); */
	for(index = 1; index <= CUST_COUNT; index++) {
		if(index%2 == 0)
			custData[index].sixBurger = 1;
		else
			custData[index].sixBurger = 0;
			
		if(index%2 == 1)	
			custData[index].threeBurger = 1;
		else
			custData[index].threeBurger = 0;
			
		if(index%2 == 0)		
			custData[index].vegBurger = 0;
		else
			custData[index].vegBurger = 1;
			
		if(index%2 == 1)			
			custData[index].fries = 1;
		else
			custData[index].fries = 0;
			
		if(index%2 == 0)				
			custData[index].soda = 0;
		else
			custData[index].soda = 1;
			
		if(index%2 == 1)
			custData[index].dineType = 1;
		else
			custData[index].dineType = 0;
	}		
	
	Release(customerDataLock);	
	
	foodReadyData.sixBurger = MINCOOKEDFOOD; 
	foodReadyData.threeBurger = MINCOOKEDFOOD;
	foodReadyData.vegBurger = MINCOOKEDFOOD;
	foodReadyData.fries = MINCOOKEDFOOD; 
	
	/* Find the amount of each type of food still yet to be cooked 	*/
	foodToBeCookedData.sixBurger  = 0;
	foodToBeCookedData.threeBurger = 0;
	foodToBeCookedData.vegBurger  = 0;
	foodToBeCookedData.fries = 0;

	/* set the initial status of all the ordertakers as BUSY	*/
	/* - So every customer will join the customer queue	*/
	for(index = 1; index <= OT_COUNT; index++)		 
		orderTakerStatus[index] = OT_BUSY;	
	
	/*--------------------------------------------------------	*/
	/* SPAWNING THE CARLS JR ENTITIES	*/
	/*--------------------------------------------------------	*/
	/* Spawning the OrderTakers	*/
	for(index = 1; index <= OT_COUNT; index++)      		 	
        Fork(OrderTaker);
	
	/* Spawning the Waiters	*/
 	for(index = 1; index <= WAITER_COUNT; index++)
        Fork(Waiter); 
		
	/* Spawning a Manager */
    	Fork(Manager); 
 
	/* Spawning the Customers */
 	for(index = 1; index <= CUST_COUNT; index++)  
		Fork(Customer);
	Exit(0);
}

/* END OF CODE 	*/
