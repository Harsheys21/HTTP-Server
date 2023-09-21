Functionality:
The program created essentially takes in a HTTP request and if it's a get, it will output the text in the file but if it's a put, then it puts data into a file. The program is essentially a client so if a client fails or gives a wrong input, the server still runs but prints out error codes instead. The server also constantly runs using a while loop. The basics of this file are:
set up socket

read requests from clients through socket

parse request

service request

send response back

Files Included:
httpserver.c
asgn2_helper_funcs.a
asgn2_helper_funcs.h
Makefile
README.md

How to run:
type "./httpserver (portnumber)" in a bash script and in another write a request. This can be in the form of curl, nc, etc. 
