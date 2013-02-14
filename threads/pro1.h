#include "copyright.h"
#include "system.h"
#include "synch.h"
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define OT_FREE 0
#define OT_BUSY 1
#define OT_WAIT 2

typedef struct{
			int six$Burger;   // Costs 6$	/* value = 0 -> not ordered
			int three$Burger; // Costs 3$ 	   value = 1 -> ordered
			int vegBurger;    // Costs 8$	   value = 2 -> item is reserved
			int fries;        // Costs 2$	*/
			int soda;         // Costs 1$
			bool dineType;    // Type of service the customer has opted for
							  // - Eat-in = 0
							  // - To-go  = 1    
			int ordered;		  // set if he already take the order
			
			int myOT;		  // index of the OrderTaker servicing the customer
			int tokenNo;	  // Unique number which identifies the order 
							  // of the current customer
			int tableNo;	  // where the eat-in customer is seated	
			int bagged;		  // set if order is bagged by Order Taker	
			int delivered;	  // set if the order is bagged and delivered
		}custDB; 

typedef struct{
			int six$Burger;
			int three$Burger;
			int vegBurger;
			int fries;
		}foodToBeCookedDB;



typedef struct{
			int six$Burger;
			int three$Burger;
			int vegBurger;
			int fries;
		}foodReadyDB;
