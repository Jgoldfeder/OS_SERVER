struct thread_info{

    int thread_id;
    int thread_count; /** total amount of requests this thread handled **/
    int thread_html_count; /** total amount of HTML requests this thread handled **/
    int thread_image_count; /** total amount of IMAGE requests this thread handled **/

};

struct request_info{

    int arrival_count; /** amount of requests that arrived before this one. THIS IS A SHARED VARIABLE **/
    int time_arrival; /** time that this request arrived as first seen by the master thread. Current time minus when web server started **/
    int dispatched_requests_count; /** total amount of requests that have been dispatched before this request. THIS IS A SHARED VARIABLE **/
    int dispatched_time; /** time that this request was dispatched. Current time minus when web server started **/
    int completed_count; /** number of reqquests that completed before this request.
 *                          Completed = just after file is read and before worker thread starts writing response to the socket.
 *                          THIS IS A SHARED VARIABLE**/
    int completed_time; /** time that thread completed this request. Current time minus when web server started **/
    int request_priority_count; /** number of request that arrived after this request but were dispatched before **/

};
