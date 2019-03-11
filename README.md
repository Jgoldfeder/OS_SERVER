# OS_SERVER

1: Are group consisted of Yehuda Goldfeder, Tom Milman and Nate Saada.

2)Server: The master thread and worker threads act in product consumer relationship. We used a semaphore, as seen in the class slies, to make sure threads could not take from an empty buffer, or add to a full buffer, and used a mutex to allow only 1 thread in the buffer at once. A buffer data structure was implemented as a priority queue. In FIFO mode, each new element had decreasing priority. In the other modes, html or pictures (depending on the mode) were given a higher priority, with new elements of the same tyoe given a lower priority realtive to older ones. 

Client: the client takes in command line arguments that decide on the policy the client should work with, the amount of threads it should run with, and the files it should requests. Then the client goes on creating the amount of threads, and depanding on the policy it sends requests to the server. Each thread waits via a barrier to the other threads to get their response. Once all of them got the response, the threads run again sending requests. The file that they request may differ if 2 files were given to the client.

3)ANY policy was same as FIFO

4) We tested the FIFO by simply making sure everything was executed in the order it came. To test HPIC, we ran 2 clients at once, one giving pictures and one giving html, to make sure the proper priority was given. We tested HPHC in much the same way. The log file was very useful in ascertaing that everthing was functional and we used it to print out alot of extra info to make sure it all worked. The above method worked with a buffer larger than the number of server threads, and a number of client threads smaller than the buffer size. (Note that if the number of threads in the client was larger than the server buffer, what eventually happened was that the buffer became full of the lower priority item and started fulfilling those requests.)

To compile server, type: gcc -pthread -o server server.c pool.c buffer.c

