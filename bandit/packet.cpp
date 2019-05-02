#include "slidebuf.h"
#include "socket.h"
#include "packet.h"
#include <string.h>


UDPSocket* datasock;                // UDP socket for sending data
ControlSock* ctlsock;               // TCP socket for sending control info
SlideBuffer* SBuf;                  // sliding buffer to sequence incoming data

unsigned long sequence_number = 0;  // sequence number for sequencing data


//==============================================================================
// name:   clear_buffer
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Clears out all the data in the sliding window buffer.
// Inputs: void
// Output: void
void clear_buffer()
{
   char* packet;

   while(SBuf->num_in_buff() > 0)
      if( (packet=SBuf->next()) != NULL )
         delete [] packet;
}


//==============================================================================
// name:   init_packet_layer
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Initializes some stuff for the packet layer
// Inputs: hWnd - handle to the window of the controlling application
//         DataMsg - the message to send to the windows callback (UDP)
//         CtlMsg  - the message to send to the windows callback (TCP)
// Output: void
void init_packet_layer(HWND hWnd, unsigned int DataMsg, unsigned int CtlMsg)
{
   datasock = new UDPSocket(hWnd, DataMsg);
   ctlsock = new ControlSock(hWnd, CtlMsg);
   SBuf = new SlideBuffer(RWAV_WINDOW_SIZE, RWAV_MAX_SEQ_NUM, xoff, xon);
}

//==============================================================================
// name:   reset_seq_num
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Resets the packet sequencing number
// Inputs: void
// Output: void
void reset_seq_num()
{
   sequence_number = 0;
}


//==============================================================================
// name:   send_buffer
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Sends a packet to the remote side via UDP
// Inputs: buff        - The buffer containing the data
//         packet_size - the size of the packets
//         num_packets - the number of packets in this buffer
// Output: TRUE/FALSE (success/fail)
bool send_buffer(char* buff, int packet_size, int num_packets)
{
   unsigned long temp;
   int i;
   int input_len = packet_size*num_packets;
   int seq_size = sizeof(sequence_number);
   int real_packet_size = input_len + seq_size;
   char* real_packet = new char[real_packet_size];


   // for each packet
   for(i=0;i<input_len;i+=packet_size)
   {
      // copy in the data
      memcpy(real_packet, (char*)(buff+i), packet_size);
      // now paste in the sequence number
      temp = htonl(sequence_number++);
      memcpy((char*)(real_packet+packet_size), &temp, seq_size);
      // write it to the UDP socket
      if(datasock->write(real_packet, real_packet_size) <= 0)
      {
         delete [] real_packet;
         return false;
      }
   }
   delete [] real_packet;
   return true;
}


//==============================================================================
// name:   send_buffer
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Read a packet from the UDP socket and add it to the sliding window
//         buffer.
// Inputs: packet_size - the size of the packets
// Output: TRUE/FALSE (success/fail)
bool add_data(int packet_size)
{
   unsigned long seq_num;
   int real_packet_size = packet_size + sizeof(seq_num);
   char* packet = new char[real_packet_size];

   // read a packet
   if(datasock->read(packet, real_packet_size) <= 0)
   {
      delete [] packet;
      return false;
   }
   // extract the sequence number
   seq_num = ntohl(*(unsigned long*)(packet+packet_size));
   // add it to the sliding window buffer
   if(!SBuf->add(packet, seq_num))
   {
      // delete it if we fail to stop a memory leak
      delete [] packet;
      return false;
   }
   return true;
}

//==============================================================================
// name:   wait_for_buffer_fill
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   Wait for the buffer to fill a certain amount before returning.
// Inputs: packet_size - the size of the packets
// Output: TRUE/FALSE (success/fail)
// Notes:  This is probably not going to be used.
void wait_for_buffer_fill()
{
   // pollin pollin pollin.. keep them dogies pollin
   while(SBuf->percent_full() < 50)
      Sleep(1);
}


//==============================================================================
// name:   recv_buffer
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   takes a buffer from the sliding window buffer
// Inputs: buff        - The buffer to fill
//         packet_size - the size of the packets
//         num_packets - the number of packets to put in
// Output: void
void recv_buffer(char* buff, int packet_size, int num_packets)
{
   int i;
   int len = packet_size*num_packets;
   char* packet;

   // for each packet
   for(i=0;i<len;i+=packet_size)
   {
      // get it from the sliding window buffer
      if( (packet=SBuf->next()) != NULL )
      {
         memcpy((char*)(buff+i), packet, packet_size);
         delete [] packet;
      }
      else  // if that slot had no data, return a bunch of 0's
         memset((char*)(buff+i), 0, packet_size);
   }
}

//==============================================================================
// name:   xon
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   handle an xon message generated by the sliding window buffer
// Inputs: void
// Output: void
void xon()
{
   ctlsock->xon();   // tell the other side
}

//==============================================================================
// name:   xoff
// design: Karl Stenerud
// code:   Karl Stenerud
// desc:   handle an xoff message generated by the sliding window buffer
// Inputs: void
// Output: void
void xoff()
{
   ctlsock->xoff();   // tell the other side
}

