#ifndef _PACKET_H
#define _PACKET_H

#define RWAV_MAX_SEQ_NUM 0xefffffff
#define RWAV_WINDOW_SIZE 100

void clear_buffer();
void init_packet_layer(HWND hWnd, unsigned int DataMsg, unsigned int CtlMsg);
void reset_seq_num();
bool send_buffer(char* buff, int packet_size, int num_packets);
bool add_data(int packet_size);
void wait_for_buffer_fill();
void recv_buffer(char* buff, int packet_size, int num_packets);
void xon();
void xoff();

#endif
