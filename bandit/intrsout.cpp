//intrsout.cpp  --  This file contains the main window procedure for the bandit
//                   application.
//GROUP MEMBERS:
//       Rob Blasutig
//       Michael Gray
//       Cam Mitchner
//       Karl Stenerud
//       Chris Torrens
//Application Features: Voice Streaming in the cb radio style
//                      File Streaming in the real audio style 
#include <winsock.h>
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <process.h>
#include "resource.h"
#include "socket.h"
#include "packet.h"
#include "slidebuf.h"
#include "ctlproto.h"
#include "initwin.h"
#include "intrsout.h"
#include "lowlistenwave.h"
#include "lowtalkwave.h"
#include "highwave.h"
#include "buildwave.h"

#define WIN32_LEAN_AND_MEAN

#pragma warn -use
#pragma warn -pia
#pragma warn -aus

#define PORTNUM						7000
#define MIC_PACKET_SIZE          2048
#define MIC_BUFF_UP              3
#define FILE_PACKET_SIZE         4096
#define FILE_BUFF_UP             5
#define FILE_LOW                 30
#define FILE_HIGH                85
#define MIC_LOW                  5
#define MIC_HIGH                 15

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HINSTANCE hInst;   // current instance

LPCTSTR lpszAppName = "InTrsOut";
LPCTSTR lpszTitle   = "Bandit";

//hold various messages displayed to the user
char msg[MSG_LEN+1];

//originally declared in buildwave.cpp
extern HMMIO recFile;
extern char recFileName[256];
extern MMCKINFO recRiffChunk;
extern MMCKINFO recDataChunk;
extern MMCKINFO recFmtChunk;

//originally declared in lowtalkwave.cpp
extern HWAVEIN  hwi;
extern WAVEHDR* talkBuffers[MAX_BUFFERS];

//originally declared in lowlistenwave.cpp
extern HWAVEOUT hwo;
extern WAVEHDR* listenBuffers[MAX_BUFFERS];

//originally declared in initwin.cpp
extern HWND statusWnd;
extern HWND localWnd;
extern HWND remoteWnd;
extern HWND trackWnd;
extern HWND progressWnd;
extern HWND errorWnd;
extern WNDPROC lList;
extern WNDPROC rList;

//originally declared in highwave.cpp
extern UINT deviceID;

//originally declared in packet.cpp
extern Socket* datasock;
extern ControlSock* ctlsock;
extern SlideBuffer* SBuf;

//new global variables
int packetSize;  //according to whether or not we are streaming file or voice
bool TALK;  //we are talking
bool LISTEN;  //we are listening
bool PLAYING_SONG;	//used to indicate we are using high level functions
bool PAUSE_SEND = false;  //triggered by an xoff

//these two variables track the progress of the playback of a file that has
//finished being received so that we can stop listening
bool FILE_ENDED = false;  //indicates if we have received all the file data
bool WAITING_BUFFER_EMPTY = false;  //indicates if we played all data in buffer
int countDown = 0;  //countsdown in order stop playback at the right time

//bool OUT_PAUSE = false;
bool BUFF_IT;
bool FILE; //this indicates whether or not we are file streaming or using mic
int buffer; //this is the amount we will buffer before playing sound info
char address[20] = {'\0'}; //address from dialog box
HWND hwnd;

//used for the thread that will send file information when file streaming
DWORD fileStreamID;
HANDLE fileStreamHandle;
char serveFileName[256];

//prototypes
bool StartListening(HWND, WAVEFORMATEX);
BOOL CALLBACK IPDialogFunc(HWND IPWnd, UINT message, WPARAM wParam,
                                                      LPARAM lParam);
bool ServeFile(LPVOID);

//==============================================================================
// name:    WinMain
// design:  Michael Gray
// code:    Michael Gray
// desc:    The regular WinMain function to initialize the application and main
//          window.  Only other significant thing this function does is to
//          initialize the packet layer and set the data socket (UDP) and the
//          control socket (TCP)
// Inputs:  Regular windows parameters
// Output:
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine, int nCmdShow)
{
   MSG      mainMsg;
   HWND     hWnd;
   WNDCLASS wc;

   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = WndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
   wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
   wc.lpszMenuName  = "INTRSOUTMENU";
   wc.lpszClassName = lpszAppName;

	RegisterClass(&wc);

   hInst = hInstance;

   hWnd = CreateWindow( lpszAppName, lpszTitle, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, 480, 500, NULL, NULL, hInstance, NULL);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   init_packet_layer(hWnd, WSA_ASYNC_DATA, WSA_ASYNC_CTL);
   if(!datasock->server(PORTNUM))
      MessageBox(hWnd, "Error setting up data recv socket to server mode",
                                                          "Error", MB_OK);
   if(!ctlsock->server(PORTNUM))
      MessageBox(hWnd, "Error setting up control socket to server mode",
                                                          "Error", MB_OK);

   while(GetMessage(&mainMsg, NULL, 0, 0) )
   {
      TranslateMessage(&mainMsg);
      DispatchMessage(&mainMsg);
   }

   return(mainMsg.wParam);
}

//==============================================================================
// name:    WndProc
// design:  Michael Gray
// code:    Michael Gray
// desc:    The windows procedure that:
//             *catches all the user requests
//             *catches messages from the sound device
//             *catches WinSock asynchronous messages
// Inputs:  Regular WndProc parameters
// Output:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WORD data_WSAEvent;
   WORD data_WSAErr;
	WORD ctl_WSAEvent;
   WORD ctl_WSAErr;
   WSADATA stWSAData;
   int index;
   char localFileName[256];
   static packCount = 0;
   static HMENU menu;
	hwnd = hWnd;

   switch(uMsg)
   {
      case WM_COMMAND:
      	switch(LOWORD(wParam))
         {
            case USR_PLAYLOCAL:        //listbox that holds local wave files
            	if (HIWORD(wParam) == LBN_DBLCLK)
               {
               	if (TALK)
                  {
                  	MessageBox(hWnd,
                         "You have to stop talking before you can play a file!",
                         "INSTRUCTION", MB_ICONEXCLAMATION | MB_OK);
                     return (0);
                  }
                  else if (LISTEN)
                  {
                  	MessageBox(hWnd,
                      "You have to stop listening before you can play a file!",
                       "INSTRUCTION", MB_ICONEXCLAMATION | MB_OK);
                     return (0);
                  }

                  index = SendMessage(localWnd, LB_GETCURSEL, 0, 0);

                  EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);

                  SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1,
                                 (LPARAM)"Wave File:  Playing...");
                  SendMessage(localWnd, LB_GETTEXT, index,
                                    (LPARAM)localFileName);
                  PlayFile(hWnd, localFileName);
               }
               break;

				case USR_PLAYREMOTE:       //listbox that holds remote wave files
            	if (HIWORD(wParam) == LBN_DBLCLK)
               {
                  //make sure that the sound device is not taken
               	if (TALK)
                  {
                  	MessageBox(hWnd,
                        "You have to stop talking before you can play a file!",
                        "INSTRUCTION", MB_ICONEXCLAMATION | MB_OK);
                     return (0);
                  }
                  else if (LISTEN)
                  {
                  	MessageBox(hWnd,
                       "You have to stop listening before you can play a file!",
                       "INSTRUCTION", MB_ICONEXCLAMATION | MB_OK);
                     return (0);
               	}

                  EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_ENABLED); //fail-safe

                  BUFF_IT = true;
                  FILE = true;
                  //ensure that the sliding window buffer is clear and set
                  //low and high water marks for receiving a file
                  clear_buffer();
                  SBuf->set_low(FILE_LOW);
                  SBuf->set_high(FILE_HIGH);
                  packetSize = FILE_PACKET_SIZE;
                  buffer = FILE_BUFF_UP;

                  //send remote machine the filename the user requested to
                  //play
                  index = SendMessage(remoteWnd, LB_GETCURSEL, 0, 0);
                  SendMessage(remoteWnd, LB_GETTEXT, index,
                                       (LPARAM)recFileName);

                  //set the progress bar range to that used for file streaming
                  SendMessage(progressWnd, PBM_SETRANGE, 0,
                                    (LPARAM)MAKELONG(0, 100));

                  ctlsock->get(recFileName);
               }
               break;
                                       //refresh the local file list
				case USR_REFRESH_LOCAL:    //it will change as we receive files
            	SendMessage(localWnd, LB_RESETCONTENT, 0, 0);
					SendMessage(localWnd, LB_DIR, DDL_READWRITE, (LPARAM) "*.wav");
               break;

				case USR_REFRESH_REMOTE:   //refresh the remote file list
               SendMessage(remoteWnd, LB_RESETCONTENT, 0, 0);
               if(!ctlsock->dir())
						MessageBox(hWnd, "Error sending dir message", "Error", MB_OK);
               break;

				case USR_REFRESH_BEHAVIOUR:  //refresh the behaviour application box
               SendMessage(errorWnd, LB_RESETCONTENT, 0, 0);
               break;

            case USR_STOP_LOCAL:       //stop playing local file
               mciSendCommand(deviceID, MCI_STOP, 0, (DWORD)0);
               wsprintf(msg, "Wave File:  Stopped");
					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1, (LPARAM) msg);
               break;

            case USR_PAUSE_LOCAL:      //pause local file
               mciSendCommand(deviceID, MCI_PAUSE, 0, (DWORD)0);
               wsprintf(msg, "Wave File:  Paused...");
					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1, (LPARAM) msg);
               break;

            case USR_RESUME_LOCAL:     //resume local file
               mciSendCommand(deviceID, MCI_PLAY, 0, (DWORD)0);
               wsprintf(msg, "Wave File:  Playing...");
					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1, (LPARAM) msg);
               break;

            case USR_PAUSE_REMOTE:     //pause playing the remote file
               if (FILE)
               {
                  ctlsock->pause_file();
                  waveOutPause(hwo);
                  wsprintf(msg, "Wave File:  Paused...");
		   			SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1, (LPARAM) msg);
               }
               break;

            case USR_RESUME_REMOTE:    //resume playing the remote file
               if (FILE)
               {
                  ctlsock->resume_file();
                  waveOutRestart(hwo);
                  wsprintf(msg, "Wave File:  Playing...");
		   			SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1, (LPARAM) msg);
               }
               break;

            case USR_STOP_REMOTE:      //stop playing the remote file
               if (FILE)
               {
                  ctlsock->stop_file();
                  FILE_ENDED = true;
                  countDown = MAX_BUFFERS + SBuf->num_in_buff();
                  WAITING_BUFFER_EMPTY = false;
                  wsprintf(msg, "Wave File:  Stopped");
		   			SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1, (LPARAM) msg);
               }
               break;

            case IDM_CONNECT:       //connect to a remote machine
					//get ip address from user
		         DialogBox(hInst, "IPDIALOG", hWnd, IPDialogFunc);

			      if(!datasock->connect(address, PORTNUM))
               {
      		   	MessageBox(hWnd, "Error connecting Data socket",
                                                    "Error", MB_OK);
                  return (0);
               }
	       		if(!ctlsock->connect(address, PORTNUM))
               {
               	MessageBox(hWnd, "Error connecting Control socket",
                                                      "Error", MB_OK);
                  return (0);
               }

               //connected successfully so...
               EnableMenuItem(GetMenu(hWnd), IDM_CONNECT, MF_GRAYED);
               EnableMenuItem(GetMenu(hWnd), IDM_DISCONNECT, MF_ENABLED);
 	            EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_ENABLED);
      	      EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd), IDM_GETREMOTEDIR, MF_ENABLED);

               wsprintf(msg, "Connection:  %s", address);
					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 2, (LPARAM) msg);
            	break;

            case IDM_DISCONNECT:          //disconnect from remote
               //let remote user know of disconnection
               ctlsock->quit();
               datasock->disconnect();
               ctlsock->disconnect();
               //move back to server state so we can accept connections from
               //other clients
               datasock->server(PORTNUM);
               ctlsock->server(PORTNUM);

               clear_buffer();
               SBuf->reset();

               EnableMenuItem(GetMenu(hWnd), IDM_CONNECT, MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd), IDM_DISCONNECT, MF_GRAYED);
               EnableMenuItem(GetMenu(hWnd), IDM_GETREMOTEDIR, MF_GRAYED);
	            EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_GRAYED);
   	         EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);
      	      EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_GRAYED);
         	   EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);
               wsprintf(msg, "Connection:  None");
					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 2, (LPARAM) msg);
               break;

            case IDM_TALK:          //start talking
            	{
            		int counter;

                  //enable other menu items accordingly -- user can only stop
                  //talking from here
                  EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_ENABLED);

                  //let user know you will talk
	               if(!ctlsock->talk())
   	            	MessageBox(hWnd, "Error sending talk message",
                                                      "Error", MB_OK);

                  //set variables to microphone support
                  FILE = false;
                  TALK = true;
                  packetSize = MIC_PACKET_SIZE;

                  //set talk buffers to null before we allocate them
                  for (counter = 0; counter < MAX_BUFFERS; counter++)
	               	talkBuffers[counter] = NULL;

                  AllocTalkBuffers();

                  //now we can wait until the remote user begins to listen
                  //see CTL_START_LISTENING under FD_READ of control socket
					}
            	break;

				case IDM_LISTEN:        //start listening
            	{
               	WAVEFORMATEX wfx;
                  UINT uState;

                  //settings set for microphone support
                  clear_buffer();
                  SBuf->set_low(MIC_LOW);
                  SBuf->set_high(MIC_HIGH);
                  buffer = MIC_BUFF_UP;
                  packetSize = MIC_PACKET_SIZE;

                  EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_GRAYED);
                  EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_ENABLED); //fail-safe

                  FILE = false;

                  //settings for opening output device for mic communciation
                  wfx.nChannels = 2;  //wic.wChannels;
                  wfx.nSamplesPerSec = 22050;

               	wfx.wFormatTag      = WAVE_FORMAT_PCM;
                  wfx.wBitsPerSample  = 8;
                  wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
                  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
                  wfx.cbSize          = 0;

	               if(!ctlsock->start_listening())
   	            	MessageBox(hWnd, "Error sending start listening message",
                                                               "Error", MB_OK);

                  //set the progress bar range to deal with microphone
                  SendMessage(progressWnd, PBM_SETRANGE, 0,
                                    (LPARAM) MAKELONG(0, 10));

                  StartListening(hWnd, wfx);
                  //set global so we will restart the playback upon receiving
                  //(under FD_READ of the data socket) -- don't put this before
                  //StartListening because it will cause the device to restart
                  BUFF_IT = true;
               }
               break;

				case IDM_STOPTALK:      //stop talking
            	TALK = false;
					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 0,
                                     (LPARAM) "Voice:  None");
               //let remote user know we have stopped talking
	            if(!ctlsock->stop_talking())
   	            MessageBox(hWnd, "Error sending stop talking message",
                                                          "Error", MB_OK);
               EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);
   	         StopRecordTest(hWnd);
      	      break;

				case IDM_STOPLISTEN:    //stop listening
               SendMessage(progressWnd, PBM_SETPOS, (WPARAM) 0, (LPARAM) 0);
               LISTEN = false;
               //let remote user know we have stopped listening
	            if(!ctlsock->stop_listening())
   	            MessageBox(hWnd, "Error sending stop listening message",
                                                             "Error", MB_OK);
               EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);
	         	StopPlayBackTest(hWnd);
	            break;

				case IDM_GETREMOTEDIR:     //send command to get remote wave files
            	if(!ctlsock->dir())
               	MessageBox(hWnd, "Error sending dir message", "Error", MB_OK);
					break;

				case IDM_EXIT:       //exit application
	         	if (LISTEN && PLAYING_SONG)
   	         	CloseDevice();
					DestroyWindow( hWnd );
         	   break;
			}
         break;	//end of command switch

      case WSA_ASYNC_DATA:
      	//We received a WSAAsyncSelect() FD_ notification message
         //Parse the message to extract FD_ event value and error
         //value (if there is one).
         data_WSAEvent = WSAGETSELECTEVENT (lParam);
         data_WSAErr   = WSAGETSELECTERROR (lParam);
        	if (data_WSAErr)
        	{
         	// Error in asynch notification message: display to user
            wsprintf(msg, "Asynch notification failed");
  	         SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
          	//continue in order to call reenabling function for this event
        	}
        	switch (data_WSAEvent)
         {
		  		case FD_READ:
            	//disable the async
               datasock->disable_asyncselect();

					if (!add_data(packetSize))
               {
		            wsprintf(msg, "Failed on add");
     	            SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
               }

               wsprintf(msg, "Number of packets Received: %d, %d",
                                                packCount++, BUFF_IT);
  	            SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);

               SendMessage(progressWnd, PBM_SETPOS, (WPARAM)SBuf->num_in_buff(),
                                                               (LPARAM) 0);

               //want to buffer packets on file stream before we begin playing
               if (BUFF_IT)
               {
						if (SBuf->num_in_buff() >= buffer)
                  {
                     BUFF_IT = false;
                     packCount = 0;
		               wsprintf(msg, "File has been buffed up!");
	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     if (FILE)
                     {
                        wsprintf(msg, "Wave File:  Playing...", address);
	   	   			   SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1,
                                                         (LPARAM) msg);
                     }
                     //add buffers to queue now that we have data & restart
                  	AddInitialBuffersToQueue(hWnd);
                  }
               }

               //re-enable the async
					datasock->enable_asyncselect();
            	break;
         }
         break;

      case WSA_ASYNC_CTL:
      	//We received a WSAAsyncSelect() FD_ notification message
         //Parse the message to extract FD_ event value and error
         //value (if there is one).
         ctl_WSAEvent = WSAGETSELECTEVENT (lParam);
         ctl_WSAErr   = WSAGETSELECTERROR (lParam);

        	if (ctl_WSAErr)
        	{
            if(ctl_WSAEvent == FD_CLOSE)
            {
               ctlsock->disconnect();
               datasock->disconnect();

               wsprintf(msg, "Remote user has closed the connection");
	            SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);

               if(!datasock->server(PORTNUM))
                  MessageBox(hWnd,
                     "Error setting up data recv socket to server mode!",
                     "ERROR", MB_OK);
               if(!ctlsock->server(PORTNUM))
                  MessageBox(hWnd,
                        "Error setting up control socket to server mode!",
                        "ERROR", MB_OK);
            }
            else
              MessageBox(hWnd, "Async error on control socket!", "ERROR", MB_OK);
        	}
        	switch (ctl_WSAEvent)
         {
		  		case FD_READ:
            {
               //turn off wsaasync so that this callback is not interrupted
               ctlsock->disable_asyncselect();

            	static int xoffcount = 0;
               char operation;
               char enterFileName[257];
               int fileNameLength;

               if(!ctlsock->read(&operation, 1))
    			      MessageBox(hWnd, "Error reading operation", "Error", MB_OK);

               switch(operation)
               {
                  case CTL_QUIT:
                     //remote client has disconnected!
                     wsprintf(msg, "Remote user has disconnected!");
                     SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     datasock->disconnect();
                     ctlsock->disconnect();
                     //put back into server mode so we can accept
                     datasock->server(PORTNUM);
                     ctlsock->server(PORTNUM);

                     clear_buffer();
                     SBuf->reset();

		               EnableMenuItem(GetMenu(hWnd), IDM_CONNECT, MF_ENABLED);
                     EnableMenuItem(GetMenu(hWnd), IDM_DISCONNECT, MF_GRAYED);
                     EnableMenuItem(GetMenu(hWnd), IDM_GETREMOTEDIR, MF_GRAYED);
			            EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_GRAYED);
   			         EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);
	      	   	   EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_GRAYED);
         	   		EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);

      		         wsprintf(msg, "Connection:  None");
							SendMessage(statusWnd, SB_SETTEXT, (WPARAM)2, (LPARAM)msg);
                     break;

                  case CTL_XON:
                     wsprintf(msg, "XON Received");
	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);

                     //if we are streaming a file resume the thread
                     if (FILE && PAUSE_SEND)
                     {
                         ResumeThread(fileStreamHandle);
    		                wsprintf(msg, "DEBUG: we resumed thread!", xoffcount);
    	                   SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     }

                     //we want to ignore xons while we buff up and
                     PAUSE_SEND = false;
                     break;

                  case CTL_XOFF:
		               wsprintf(msg, "XOFF Received");
	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);

                     //if we are streaming a file pause the thread
                     if (FILE && !PAUSE_SEND)
                     {
								SuspendThread(fileStreamHandle);
    		               wsprintf(msg, "DEBUG: we paused thread!", xoffcount);
    	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     }

							//setting this variable will cause mic to drop packets
      	            PAUSE_SEND = true;
                     break;

                  case CTL_TALK:
                     wsprintf(msg, "The remote user requests to talk to you!");
	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     break;

                  case CTL_GET:
                     //set packetSize
                     packetSize = FILE_PACKET_SIZE;
                     PAUSE_SEND = false;
                     //ensure that the xoffs suspend sending thread!
                     FILE = true;

                     if(!ctlsock->extract_filename(serveFileName))
          			      MessageBox(hWnd, "Error getting filename", "Error",
                                                                     MB_OK);

                     wsprintf(msg, "%s", serveFileName);
 	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);

                     //create a thread to serve the file
                     fileStreamHandle = CreateThread(NULL, 0,
                           (LPTHREAD_START_ROUTINE)ServeFile,
                           NULL, 0, &fileStreamID);
                     break;

                  case CTL_FLIST_ENTRY:
                     if(!ctlsock->extract_filename(enterFileName))
          			      MessageBox(hWnd, "Error getting filename!", "Error",
                                                                     MB_OK);
                     SendMessage(remoteWnd, LB_ADDSTRING, 0,
                                       (LPARAM)enterFileName);
                     break;

                  case CTL_DIR:
                     {
	                     int counter;
   	                  int numEntries;

      	               //get number of entries in local list box
         	            numEntries = SendMessage(localWnd, LB_GETCOUNT, 0, 0);

            	         //send them to the remote machine
               	      for (counter = 0; counter < numEntries; counter++)
                  	   {
                     	   SendMessage(localWnd, LB_GETTEXT, counter,
                                                (LPARAM)enterFileName);
                        	if(!ctlsock->flist_entry(enterFileName))
             			      	MessageBox(hWnd, "Error sending flist entry",
                                                            "Error", MB_OK);
	                     }
      	            }
         	         break;

                  case CTL_WAVE_FORMAT:
                     {
	                     WAVEFORMATEX wfx;
   	                  int wfxLen;

      	               wfxLen = sizeof(WAVEFORMATEX);

                        //read the socket for the WAVEFORMATEX structure
            	         if(!ctlsock->read((LPSTR) &wfx, wfxLen))
               	      {
       		      		   MessageBox(hWnd, "Error reading WAVEFORMATEX!",
                                                            "ERROR", MB_OK);
                     	}

                        //create a wave to save to
                        CreateWave(hWnd, wfx);
            	         StartListening(hWnd, wfx);
                     }
                     break;

      				case CTL_FILE_END:
                     //add 10 to account for any more packets arriving
                     FILE_ENDED = true;
                     WAITING_BUFFER_EMPTY = true;
                     //make sure that files less than the size of initial buf up
                     //are played
                     if (BUFF_IT)
                     {
                        BUFF_IT = false;
	                     packCount = 0;
                     	AddInitialBuffersToQueue(hWnd);
	   	               wsprintf(msg, "File less than amount buffered!");
	                     SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     }
                     break;

                  case CTL_PAUSE_FILE_SEND:
                     if (FILE)
                     {
								SuspendThread(fileStreamHandle);
    		               wsprintf(msg, "PAUSE: we paused thread!");
    	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     }
                     break;

                  case CTL_RESUME_FILE_SEND:
                     if (FILE)
                     {
                         ResumeThread(fileStreamHandle);
    		                wsprintf(msg, "RESUME: we resumed thread!", xoffcount);
    	                   SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     }
                     break;

                  case CTL_STOP_FILE_SEND:
                     if (FILE)
                     {
                        //stop thread by setting global that it checks
                        FILE = false;

    		               wsprintf(msg, "STOP: we stopped thread!");
    	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     }
                     break;

                  case CTL_STOP_TALKING:
                     wsprintf(msg, "Remote user has stopped talking!");
    	               SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     break;

                  case CTL_START_LISTENING:
                     wsprintf(msg, "Remote user has started to listen!");
    	               SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     if (TestOpenInputDevice(hWnd))
                     {
		   					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 0,
                                          (LPARAM)"Voice:  Talking...");
                        StartRecordTest(hWnd);
                     }
                     else
                     {
                     	CleanUpTalkBuffers();
                        TALK = false;
                     }
                     break;

                  case CTL_STOP_LISTENING:
                     wsprintf(msg, "Remote user has stopped listening!");
    	               SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     break;

                  default:
                     wsprintf(msg,
                        "Received a control message that was not interpreted!");
	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                     break;

               }//switch on operation
               ctlsock->enable_asyncselect();
            }
            break;  //break out of fd_read

				case FD_ACCEPT:
					if(!ctlsock->accept())
    			      MessageBox(hWnd, "Error on accept", "Error", MB_OK);
			      else if(!datasock->connect(ctlsock->get_address(), PORTNUM))
      		      MessageBox(hWnd, "Error connecting Data socket",
                                                   "Error", MB_OK);

               //display address in status bar
               wsprintf(msg, "Connection:  %s", ctlsock->get_address());
					SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 2, (LPARAM) msg);
               //the client will be the bandit and the server will be smokie
               //this is an allusion to the 80's movie "Smokie and the Bandit"
               //if you do not know this movie was a comedy in which a trucker
               //that needed to get a semi full of beer accross a dry county
               //use his CB RADIO (like the cb in this program) to outwit some
               //rather stupid smokies (otherwise known as sherrifs).
               //...staring Burt Reynolds, Jackie Gleson, and Sally Fields
               //...available at all your local movie rental stores
               //...I do not recommend it for non-barbarians
               wsprintf(msg, "Smokey");
               SetWindowText(hWnd, msg);
               EnableMenuItem(GetMenu(hWnd), IDM_CONNECT, MF_GRAYED);
               EnableMenuItem(GetMenu(hWnd), IDM_DISCONNECT, MF_ENABLED);
 	            EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_ENABLED);
      	      EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_ENABLED);
               EnableMenuItem(GetMenu(hWnd), IDM_GETREMOTEDIR, MF_ENABLED);
               break;
         }
         break;  //wsa_ctlasync

      case USR_INBLOCK:
      	{
         	//received data buffer block with recorded data,
            MMRESULT  rc;
            MMTIME    mmtime;
            LPWAVEHDR lpwh = (LPWAVEHDR)lParam;

            if (TALK)
            {
               //PAUSE_SEND would be true if we received an xoff from remote
					if (!PAUSE_SEND)
               {
						send_buffer(lpwh->lpData, packetSize, 1);
               }

            	//reuse data buffer block
               rc = waveInPrepareHeader(hwi, lpwh, sizeof(WAVEHDR));

	            if (rc == MMSYSERR_NOERROR)
						rc = waveInAddBuffer(hwi, lpwh, sizeof(WAVEHDR));

	            if (rc != MMSYSERR_NOERROR)
   	         {
      	      	waveInGetErrorText(rc, msg, MSG_LEN),
         	      MessageBox(hWnd, msg, NULL, MB_OK);
						TALK = FALSE;	//stop the recording
               }
				}
         }
         break;

      case MM_WOM_DONE:
			if (LISTEN)
         {
	      	PostMessage(hWnd, USR_OUTBLOCK, 0, lParam);
         }
         break;

      case MM_MCINOTIFY:
         EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_ENABLED);
         EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_ENABLED);
         EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);
         CloseDevice();
         break;

      case USR_OUTBLOCK :
      	{
         	//received data buffer block that has
            //completed playback, reload and requeue
            MMRESULT  rc;
            LPWAVEHDR lpwh = (LPWAVEHDR)lParam;

            //write the data to the file if we are file streaming
            if (FILE)
               mmioWrite(recFile, (HPSTR)lpwh->lpData, (LONG)packetSize);

				if (LISTEN)
            {
            	//we want to start a count down if we are still playing a
               //file that has been sent in its entirety
               if (FILE_ENDED)
               {
                  if (SBuf->num_in_buff() == 0 && WAITING_BUFFER_EMPTY)
                  {
                     countDown = MAX_BUFFERS;
                     WAITING_BUFFER_EMPTY = false;
                  }
               	if (countDown <= 0 && !WAITING_BUFFER_EMPTY)
                  {
                     FILE_ENDED = false; //reset for another file
                     LISTEN = false;
                     SendMessage(progressWnd, PBM_SETPOS, (WPARAM) countDown,
                                                                  (LPARAM) 0);

                     //reset packet size and buff up to mic settings
                     packetSize = MIC_PACKET_SIZE;
                     buffer = MIC_BUFF_UP;
                     clear_buffer();
                     SBuf->set_low(MIC_LOW);
                     SBuf->set_high(MIC_HIGH);

			            EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_ENABLED);
   			         EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);
	      	   	   EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_ENABLED);
         	   		EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);

                     //finish saving the wave file and close it
                     FinishCreateWave(hWnd);
                     StopPlayBackTest(hWnd);
                     return (1);
                  }
                  if (!WAITING_BUFFER_EMPTY)
                  {
                     --countDown;
                     wsprintf(msg, "Counting Down: %d", countDown);
	                  SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);
                  }
               }

					recv_buffer(lpwh->lpData, packetSize, 1);

               //we want to update the delay view in both FD_READ (UDP) and here
               //because if the sender is paused because of xoff the bar won't
               //be updated.  Need it in FD_READ because of buffing file up.
               SendMessage(progressWnd, PBM_SETPOS, (WPARAM)SBuf->num_in_buff(),
                                                                  (LPARAM) 0);

				   lpwh->dwBufferLength = packetSize;

               rc = waveOutPrepareHeader(hwo, lpwh, sizeof(WAVEHDR));

	            //write buffers to the queue
	            if (rc == MMSYSERR_NOERROR)
	            	rc = waveOutWrite(hwo, lpwh, sizeof(WAVEHDR));

	            if (rc != MMSYSERR_NOERROR)
	            {
	            	waveOutGetErrorText(rc, msg, MSG_LEN),
	               MessageBox(hWnd, msg, NULL, MB_OK);
                  StopPlayBackTest(hWnd);  // free allocated memory
                  LISTEN = false;
               }
            }
         }
      	break;

		case WM_CTLCOLORSTATIC:
      	SetTextColor((HDC)wParam, RGB(255, 255, 0));
         SetBkColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)GetStockObject (BLACK_BRUSH);
         break;

		case WM_CREATE:
			{
            //unless we are sending a file, assume microphone
	      	packetSize = MIC_PACKET_SIZE;
            buffer = MIC_BUFF_UP;
	         TALK = FALSE;
   	      LISTEN = FALSE;
            BUFF_IT = false;

				//since we are not connected yet, user can't talk or listen
            EnableMenuItem(GetMenu(hWnd), IDM_LISTEN, MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd), IDM_STOPLISTEN, MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd), IDM_TALK, MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd), IDM_STOPTALK, MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd), IDM_GETREMOTEDIR, MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd), IDM_DISCONNECT, MF_GRAYED);

            //intialize the child windows of the user interface
            InitLocalList(hWnd);
            InitRemoteList(hWnd);
				InitStatusBar(hWnd);
            InitProgressBar(hWnd);
            InitErrorList(hWnd);
            InitButtons(hWnd);
         }
	      break;

      case WM_DESTROY :
      	PostQuitMessage(0);
         break;

      default :
         return(DefWindowProc(hWnd, uMsg, wParam, lParam));
   }

   return(0);
}

//==============================================================================
// name:    waveInProc
// design:  Michael Gray
// code:    Michael Gray
// desc:    Callback for when a buffer has finished being filled with recorded
//				data.
// Inputs:  regular windows dialog box parameters
// Output:  none
// Notes:   Straight out of Windows 95 Multimedia & ODBC API Bible
VOID CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance,
                                    DWORD dwParam1, DWORD dwParam2)
{
   switch (uMsg)
   {
      case WIM_OPEN :
      case WIM_CLOSE:
              break;  // don't care

      case WIM_DATA :
              {
                 // post message to process this input block received
                 // NOTE: callback cannot call other waveform functions
                 PostMessage((HWND)dwInstance, USR_INBLOCK, 0, dwParam1);
                 break;
              }
   }
}

//==============================================================================
// name:   	IPDialogFunc
// design:  Michael Gray
// code:    Michael Gray
// desc:   	callback function for the dialog box that gets ip address from user
// Inputs:  none
// Output: 	BOOL
// Notes:
BOOL CALLBACK IPDialogFunc(HWND IPWnd, UINT message, WPARAM wParam,
                                                      LPARAM lParam)
{
	UINT bytesRead;

	switch (message)
   {
   	case WM_COMMAND:
      	switch(LOWORD (wParam))
         {
         	case ID_CANCEL:
            	EndDialog(IPWnd, 0);
               break;

            case ID_OK:
					bytesRead = GetDlgItemText(IPWnd, ID_IPADDR, address, 20);
               if (bytesRead == 0)
               {
               	MessageBox(hwnd, "Please fill in an IP address", "IP Missing",
                                                   MB_OK | MB_ICONEXCLAMATION);
               }
               else
               {
               	EndDialog(IPWnd, 0);
               }
               break;
         }
         break;
   }
   return FALSE;
}
//==============================================================================
// name:    StartListening
// design:  Michael Gray
// code:    Michael Gray
// desc:    This function is a container that opens the output device and then
//          starts playback.
// Inputs:  hWnd -- handle to the main window
//          wfx -- the structure that allows the output device to be opened
//                 according the the sound data being received.
// Output:  none
bool StartListening(HWND hWnd, WAVEFORMATEX wfx)
{
   if (LISTEN)
   {
      MessageBox(hWnd, "You are already listening!", "ERROR",
                                 MB_ICONEXCLAMATION | MB_OK);
      return false;
   }

	LISTEN = true;

	AllocListenBuffers();
   if (TestOpenOutputDevice(hWnd, wfx))
   {
      if (FILE)
         SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 1,
                        (LPARAM)"Wave File: Playing...");
      else
         SendMessage(statusWnd, SB_SETTEXT, (WPARAM) 0,
                           (LPARAM)"Voice:  Listening...");
	   StartPlayBackTest(hWnd);
   }
   else
      CleanUpListenBuffers();

   return true;
}
//==============================================================================
// name:    ServeFile
// design:  Michael Gray
// code:    Michael Gray
// desc:    This is a thread that opens the file to be sent and then sends the
//          data.  If FILE ever becomes false it is a signal to stop.
// Inputs:  none
// Output:  none
bool ServeFile(LPVOID pvoid)
{
	MMRESULT	rc;
   LONG		bytesRead;
   HMMIO		fileStreamID;
   MMCKINFO	dataChunk;
   MMCKINFO formatChunk;
   MMCKINFO parentChunk;
   LPSTR		data;
   LPSTR    fmt;
   LONG     fmtSize;
   LPWAVEFORMATEX lpwfx;

   wsprintf(msg, "In thread: %s", serveFileName);
	SendMessage(errorWnd, LB_INSERTSTRING, 0, (LPARAM)msg);

   //open file for buffered i/o
   fileStreamID = mmioOpen(serveFileName, NULL, MMIO_READ | MMIO_ALLOCBUF);
   if (!fileStreamID)
   {
    	//let the client know
   	MessageBox(hwnd, "Open wave file for serving did not work!", "ERROR",
                                                                        MB_OK);
   }

	//ensure we are at the beginning of a file
   mmioSeek(fileStreamID, 0, SEEK_SET);

	//ensure it is a wave file and get descend file pointer into parent chunk
   parentChunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
   rc = mmioDescend(fileStreamID, (LPMMCKINFO) &parentChunk, NULL,
                                                   MMIO_FINDRIFF);
   if (rc != MMSYSERR_NOERROR)
   {
   	//let the client know
      MessageBox(hwnd, "The selected file was not in waveform-audio format!",
                                                               "ERROR", IDOK);
      mmioClose(fileStreamID, 0);
      return false;
   }

   //descend to the fmt chunk of the wave file to get format information
   formatChunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
   if (mmioDescend(fileStreamID, &formatChunk, &parentChunk, MMIO_FINDCHUNK))
   {
      MessageBox(hwnd, "Data chunk of file not found!", "ERROR", IDOK);
      mmioClose(fileStreamID, 0);
      return false;
   }

   fmtSize = formatChunk.cksize;
   fmt = (char*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fmtSize);

   if (mmioRead(fileStreamID, (HPSTR)fmt, fmtSize) != fmtSize)
   {
      MessageBox(hwnd, "Data chunk of file not found!", "ERROR", IDOK);
      HeapFree(GetProcessHeap(), 0, fmt);
      mmioClose(fileStreamID, 0);
      return false;
   }

   lpwfx = (LPWAVEFORMATEX)fmt;

   mmioAscend(fileStreamID, &formatChunk, 0);

   ctlsock->wave_format((char*)lpwfx, fmtSize);

   //descend to the data chunk of the wave file
   dataChunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
   rc = mmioDescend(fileStreamID, &dataChunk, &parentChunk, MMIO_FINDCHUNK);
   if (rc != MMSYSERR_NOERROR)
   {
   	//let the client know
      MessageBox(hwnd, "Data chunk of file not found!", "ERROR", IDOK);
      mmioClose(fileStreamID, 0);
   }

   data = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, packetSize);

   //read data from the file and send it off to the client
   while ((bytesRead = mmioRead(fileStreamID, (HPSTR)data, packetSize)) > 0)
   {
      if (FILE)
      {
   		send_buffer(data, bytesRead, 1);
         Sleep(20);
      }
      else //remote user wants to stop
      {
         DWORD exitCode;

         GetExitCodeThread(fileStreamID, &exitCode);
         ExitThread(exitCode);
      }
   }

   Sleep(1000);

   //let the other side know that the entire file contents have been sent
   ctlsock->file_end();

	//close the file
   mmioClose(fileStreamID, 0);

   //free allocated memory
	HeapFree(GetProcessHeap(), 0, data);

   CloseHandle(fileStreamHandle);
   fileStreamHandle = NULL;
   FILE = false;

   return true;
}
