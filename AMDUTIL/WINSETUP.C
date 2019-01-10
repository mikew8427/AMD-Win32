#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
int WinSetup(HINSTANCE inst,char *cmdline,char *title,int show);
LRESULT WINAPI ClientWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
void BuildArgs(char *cmdline);
void DispMsg(void);

char *argc[20];				/* Max of 20 args passed */
int argn;					/* Number of args		 */

int WinSetup(HINSTANCE inst,char *cmdline,char *title,int show)
{
HWND hwnd ;
char szClassName[ 20]="GENERIC" ;
// WNDCLASSEX wc ;
WNDCLASS wc;

	BuildArgs(cmdline);

//	wc.cbSize = sizeof( wc) ;
	wc.style = CS_VREDRAW | CS_HREDRAW ;
	wc.lpfnWndProc = ClientWndProc ;
	wc.cbClsExtra = 0 ;
	wc.cbWndExtra = 0 ;
	wc.hInstance = inst ;
	wc.hIcon = LoadIcon( inst, "shellobj") ;
//	wc.hIconSm = LoadIcon( inst, "shellobj") ;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW) ;
	wc.hbrBackground = GetStockObject( WHITE_BRUSH) ;
	wc.lpszMenuName = NULL ;
	wc.lpszClassName = szClassName ;

	if( !RegisterClass( &wc))
	{
		MessageBeep( 0) ;
		return FALSE ;
	}

	hwnd = CreateWindowEx(	WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE,
							szClassName,
							title,
							WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
							CW_USEDEFAULT, 0,
							CW_USEDEFAULT, 0,
							NULL,
							NULL,
							inst,
							NULL) ;

	ShowWindow( hwnd, show) ;
	DispMsg();
}
void DispMsg(void)
{
MSG msg ;

	TranslateMessage( &msg) ;
	if(msg.message==WM_CLOSE) PostQuitMessage(0);
	DispatchMessage( &msg) ;
}
void BuildArgs(char *cmd)
{
int x=0;
	argn=1;
	argc[argn]=&cmd[x];
	while(cmd[x]>'\0')
		{
		if(cmd[x]=='\"') {
			x++;
			argc[argn]=&cmd[x];
			while(cmd[x]!='\"')x++;
			cmd[x++]='\0';
			while(cmd[x]==' ')x++;
			if(cmd[x]!='\0') argc[++argn]=&cmd[x];
			continue;
		}
		if(cmd[x]==' ')		
			{ 
			cmd[x++]='\0'; 
			while(cmd[x]==' ')x++;
			argc[++argn]=&cmd[x];
			continue;
			}
		x++;
		}
}
LRESULT WINAPI ClientWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	static HINSTANCE hInstance ;

	switch( msg)
	{
		case WM_CREATE:
			if(OleInitialize( NULL) != S_OK) MessageBeep( 0) ;
			break ;

		case WM_COMMAND:
			break;
		case WM_CLOSE:
			{
			PostMessage( hwnd, WM_QUIT, 0, 0) ;
			}
			break ;
		case WM_DESTROY:
			PostQuitMessage(0);
			ExitProcess(0);
			break ;
		default:
			break ;
	}
	return DefWindowProc( hwnd, msg, wParam, lParam) ;
}
