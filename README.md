# IOS_Project2_2023

## Job Description (Mail)
There are 3 types of processes in the system: (0) main process, (1) postman and (2) customer. 
Each customer goes to the post office to handle one of three types of requests: letter services, parcels, money services. 
Each request is uniquely identified by a number (letters:1, parcels:2, money services:3). Upon arrival, they join the queue according\
to the activity they can handle. Each clerk serves all queues (choosing one of the queues at random each time). If no customer is currently\
waiting, the clerk takes a short break. After the post office closes, the clerks finish serving all the customers in the queue and drop\
them home after all the queues are empty. Any customers who come after the post office closes go home (tomorrow is also a day).
