#ifndef GB_RLUTIL_H
#define GB_RLUTIL_H

#include <greg/rlutil.h>

#define SCREENROWS 	61
#define SCREENCOLS 	170
typedef unsigned char uchr;
typedef unsigned short ush;
typedef unsigned short ushort;
typedef unsigned char uchar;
#define limituc(x,y,z)	limit(x,(uchr)(y),(uchr)(z))
#include <stdio.h>
#include <string>
#include <greg/utility.h>
#include <greg/rrnd.h>
using std::string;


HWND GetConsoleHwnd(void) {
		char pszWindowTitle[1024];
		GetConsoleTitle(pszWindowTitle, 1024);
		return FindWindow(NULL, pszWindowTitle);
}

void MaximizeConsole(void) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD bSize = GetLargestConsoleWindowSize(hConsole);
	SMALL_RECT wSize = {0, 0, bSize.X-1, bSize.Y-1};
	MoveWindow(GetConsoleHwnd(),0,0,bSize.X,bSize.Y,TRUE);
	SetConsoleScreenBufferSize(hConsole, bSize);
	SetConsoleWindowInfo(hConsole, TRUE, &wSize);
}

void ReleaseConsole(void) {
	rlutil::setColor(rlutil::GREY);
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD bSize; bSize.X=50; bSize.Y=80;// = GetLargestConsoleWindowSize(hConsole);
	SMALL_RECT wSize = {0, 0, bSize.X-1, bSize.Y-1};
	SetConsoleScreenBufferSize(hConsole, bSize);
	SetConsoleWindowInfo(hConsole, TRUE, &wSize);
}

void fgcolor(uchar &r,uchar v) 						{ r|=v; }
void bgcolor(uchar &r,uchar v) 						{ r|=v<<4; }
void storecolor(uchar &r,uchar fg,uchar bg) 		{ r=fg|bg<<4; }
void storecolor(ushort &r,uchar fg,uchar bg) 		{ r&=0x00FF;r|= (fg|bg<<4)<<8; }
unsigned short colorchar(uchar v,uchar fg,uchar bg) { 
	ushort ret = v; storecolor(ret,fg,bg); return ret; 
	}
uchr readfg(const ushort &r) 	{ return 0x0f&r>>8; }
uchr readbg(const ushort &r)	{ return 0x0f&r>>12; }
uchr readfg(const uchar &r) 	{ return 0x0f&r; }
uchr readbg(const uchar &r)		{ return 0xf0&r; }

void DisplayChar(int row,int col, unsigned short c) {
	rlutil::locate(col+1,row+1);
	rlutil::setColor(c>>8);
	printf("%c",(unsigned char)c);
}

void DisplayNum(int row,int col,int v,int clr=-1) {
	rlutil::locate(col+1,row+1);
	if(clr>-1) rlutil::setColor(clr);
	printf("%d",v);
}
void DisplayString(int row,int col, const string &s,int clr=-1) {
	rlutil::locate(col+1,row+1);
	if(clr>-1) rlutil::setColor(clr);
	printf("%s",s.c_str());
}
void DisplayHorizontal(int row, int col, char c,int repeat) {
	limit(repeat,1,SCREENCOLS-col);
	while(repeat--)	DisplayChar(row,col++,c);
}
void DisplayVertical(int row, int col, char c, int repeat) {
	limit(repeat,1,SCREENROWS-row);
	while(repeat--) DisplayChar(row++,col,c);
}

#define DEFAULT_TEXT_COLOR	rlutil::GREY


#endif
