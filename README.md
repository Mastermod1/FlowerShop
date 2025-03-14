## Order Center
- This is the store that customers enter and place orders from registers where a employee awaits them  
- There are 5 Registers (Cashiers) (different threads)  
- Only Two employees(Simon and Simon -> Simons) call to the Warehouse for to place orders  
- Simons read from the queue and calls the warehouse ensuring the order is placed  
- The order queue is a shared resource amongst cashiers and simon  
- Orders are collected in a local cached queue  
- Each order is a request to Warehouse  
- Response is received once the order is delivered to Warehouse (not when it's delivered)  
- Each Simon can make 1 phone-call at a time  
- Since each simon has a different phone line, they have different clients  
- producer-consumer is implemented with a semaphore(number of simons)  
- When an order is placed and each simon is talking, order waits for a simon to call the warehouse and get an approval  
- This is a blocking operation
- https://medium.com/coderscorner/tale-of-client-server-and-socket-a6ef54a74763
- https://github.com/cpuguy83/blog/blob/master/_posts/2019-02-18-non-blocking-io-in-go.md

## Wearhouse
 Warehouse manager is Dan  
- Each call from Simons is transferred to Dan  
- Dan places orders to queue where delivery cars will read from  
- Dan is the server  
- Warehouse has multiple DeliveryCars for orders  
- Each delivery car takes random amount of time to deliver a single order  
- Due to traffic and chaos a delivery may take EstimatedTime + RandomWastedTime(between 0-0.5 of estimated time)  
- Dan checks if the orders are made in time  
- If the random estimated time is 3 seconds, Dan gets mad if it's delivered in 1.2 times Estimated 3.6seconds  
- Each delivery car has its own thread  
- The orders are collected into a producer-consumer queue with mutex and cv  
- When delivery is made and car returns, Dan is notified (promise-future) (`queue<future<order>>`)  
- In a detached thread Dan keeps time for orders  
- If the critical time (time which makes Dan angry) is met and the car is not returned, Dan punches a wall `std::cerr`  
- If delivery is on time Dan is happy `std::cout`
