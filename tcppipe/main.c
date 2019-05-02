#include "socket.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>

int wait_for_input(Socket* sock_a, Socket* sock_b, int wait);
void exit_error(char* err);
void tcp_pipe(Socket* server, char* destname, short destport);

int main(int argc, char* argv[])
{
   Socket server;
   Socket panic;
   Socket dummy;
   Socket* input;
   int localport;
   int remoteport;
   char* remoteaddr;

   if(argc < 4)
      exit_error("Usage: tcppipe <local_port> <remote_addr> <remote_port>\n");

   if((localport = atoi(argv[1])) == 0)
      exit_error("Invalid local port\n");
   if((remoteport = atoi(argv[3])) == 0)
      exit_error("Invalid remote port\n");
   remoteaddr = argv[2];

   printf("Piping TCP connections on port %d to %s:%d\n", localport, remoteaddr,
          remoteport);

   signal(SIGCHLD, SIG_IGN);

   if(!server.server(localport))
      exit_error("Error setting up server.\n");
   if(!panic.server(localport+1))
      exit_error("Error setting up panic port.\n");

   for(;;)
   {
      switch(wait_for_input(&server, &panic, -1))
      {
         case 0:
            break;
         case 1:
            tcp_pipe(&server, remoteaddr, remoteport);
            break;
         case 2:
            printf("Panic port accessed!  Exiting\n");
            dummy.accept(panic);
            dummy.disconnect();
            exit(0);
         case -1:
            exit_error("Exception encountered while waiting for input\n");
      }
   }
   exit(0);
}

int wait_for_input(Socket* sock_a, Socket* sock_b, int wait)
{
   fd_set in_set;
   fd_set out_set;
   fd_set exc_set;
   int maxfd = (sock_a->get_fd() > sock_b->get_fd() ? sock_a->get_fd()
                : sock_b->get_fd())+ 1;

   struct timeval  tval;
   struct timeval* wait_time = NULL;

   if(wait >= 0)
   {
      wait_time = &tval;
      wait_time->tv_sec = wait;
      wait_time->tv_usec = 0;
   }

   // initialize the fd sets.
   FD_ZERO(&in_set);
   FD_ZERO(&out_set);
   FD_ZERO(&exc_set);
   FD_SET(sock_a->get_fd(), &in_set);
   FD_SET(sock_b->get_fd(), &in_set);
   FD_SET(sock_a->get_fd(), &exc_set);
   FD_SET(sock_b->get_fd(), &exc_set);

   if(select(maxfd, &in_set, &out_set, &exc_set, wait_time) > 0)
   {
      if(FD_ISSET(sock_a->get_fd(), &in_set))
         return 1;
      if(FD_ISSET(sock_b->get_fd(), &in_set))
         return 2;
      if(FD_ISSET(sock_a->get_fd(), &exc_set) || FD_ISSET(sock_b->get_fd(), &in_set))
         return -1;
   }
   return 0;
}


void exit_error(char* err)
{
   printf(err);
   exit(-1);
}

void tcp_pipe(Socket* server, char* destname, short destport)
{
   Socket client;
   Socket remote;
   Socket* input;
   char buff[10000];
   int len;

   switch(fork())
   {
      case -1:
         exit_error("Can't fork!\n");
      case 0:
         break;
      default:
         sleep(1);
         return;
   }

   if(!client.accept(*server))
      exit_error("Error accepting connection!\n");

   if(!remote.connect(destname, destport))
      exit_error("Error connecting to remote\n");

   for(;;)
   {
      switch(wait_for_input(&client, &remote, -1))
      {
         case 0:
            break;
         case 1:
            if((len=client.read(buff, 10000)) < 1)
               exit(0);
            if(remote.write(buff, len) < 1)
               exit(0);
            break;
         case 2:
            if((len=remote.read(buff, 10000)) < 1)
               exit(0);
            if(client.write(buff, len) < 1)
               exit(0);
            break;
         case -1:
            exit_error("Exception encountered while waiting for input\n");
      }
   }
   exit(0);
}

