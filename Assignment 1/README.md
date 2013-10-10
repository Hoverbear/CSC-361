# P1 - CSC 361 #
Andrew Hobden (V00788452) Lab B02

## Design ##

The program basically works as follows:

1. Spin up a worker thread to handle raw input so that we can satisfy the "Press 'q' to quit" functionality in the specifiations. (NOTE: Derived Code)
2. Bind to a socket
3. Upon recieving a packet, create a struct of data necessary and spin up a request thread.
4. In the request thread, figure out the status to send back, read the file if necessary, and then send back the response packet(s).

## Code Structure ##
    /* Request Struct
     * --------------
     * Used for threading requests.
     */

    /* Quit Worker
     * -----------
     * Called via a pthread to watch for exit signals.
     */
     
     /* Request Worker
      * --------------
      * Called via pthread to respond to a socket request.
      * Params:
      *   - struct request pointer
      */ 
      
      /*
       * A simple web server.
       * Params:
       *  - port: The port to use.
       *  - dir:  The directory to get files from.
       */


## Bonus Features ##
I made it a (simple) multi-threaded program. It was my first time doing threading so there will be bugs!

## Thoughts ##
Initially, I was super excited about this project. Then, I realized it was UDP and my enthusiasm took a drop, because outside of this toy lab it's virtually useless since the vast majority of HTTP requests are TCP based.

I was also very disappointed by the specifications requiring a "Press 'q' to quit" which ruled out being able to read from STDIN and resulted in me spending several hours trying to find a reasonable way to implementing this on Linux/Unix without requiring ncurses.

All in all the actual networking part of this lab was simple and straightforward. Most of the challenges came from the specification and the large amount of string manipulation.