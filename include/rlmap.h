#ifndef _GB_RLMAP_H
#define _GB_RLMAP_H

#include <greg/gb_rlutil.h>

#define COPYBLANK		true
#define DISPLAYBLANK	true
#define NOCOPYBLANK 	false
#define NODISPLAYBLANK 	false

typedef const unsigned short cush;

cush playerglyph = '@'|rlutil::LIGHTGREEN<<8;
// DUNGEON
cush terr_unknown = 219|8<<8;		// ? not sure about this one
cush open_floor = ' '|0<<8;
cush floor_open = ' '|0<<8;
cush wall_stone = '#'|7<<8;
cush wall_wood = '#'|6<<8;
cush door_open = 'O'|6<<8;
cush door_closed = 'D'|6<<8;
cush stairs_up = '<'|14<<8;
cush stairs_down = '>'|14<<8;
// OUTDOOR
cush tree_values[5] = { 176,177,178,209,84 };
cush tree_colors[5] = { 38,98,106,162,166 };

struct rlchar {
	unsigned char value,color;
	rlchar(unsigned char v=' ',unsigned char c=DEFAULT_TEXT_COLOR):value(v),color(c) {}
	rlchar& operator=(const unsigned char &c) 	{ value=c; return *this; }
	rlchar& operator=(const ushort &c) { color=c>>8;value=(uchar)c; return *this; }
	rlchar& operator=(const rlchar &c) 			{ value=c.value; color=c.color; return *this; }
	rlchar& setcolor(const unsigned char &c)	{ color=c; return *this; }
	operator const unsigned short() const { 
		unsigned short ret = color;
		ret=ret<<8; ret|=value; return ret;
	}
	unsigned char getchar(void) 	const { return value; }
	unsigned char getcolor(void) 	const { return color; }
	unsigned char getfg(void) 		const { return color&0x0f; }
	unsigned char getbg(void) 		const { return color&0xf0; } 
};

struct rlstring {
	vector<rlchar>	str;
	rlstring(const string &s,unsigned char clr=DEFAULT_TEXT_COLOR) {
		for(auto i:s) { str.push_back(rlchar(i,clr)); }
	}
	void setcolor(const unsigned char &clr,const string &s="") {
		if(s.empty())	for(auto &i:str) i.setcolor(clr); 
		else {
			string wrk; wrk.reserve(str.size());
			for(auto i:str)	wrk.push_back(i.getchar());
			unsigned st = wrk.find(s);
			unsigned en = wrk.find_first_not_of(s,st);
			while(st<en)	{ str[st].setcolor(clr); st++; }
		}
	}
	void display(ushort r,ushort c) const { for(auto i:str)  DisplayChar(r,c++,i); }
};

struct coordinates {
	ushort row;
	ushort col;
	coordinates(ushort r=0,ushort c=0):row(r),col(c) {}
	bool operator<(const coordinates &c) const {
		if(row==c.row)	return col<c.col;
		return row<c.row;
	}
	coordinates operator()(ushort r,ushort c) { row=r;col=c; return *this; }
	coordinates delta(char r,char c) { 
		int nr = row+r; row = nr<0?0:nr;
		int nc = col+c; col = nc<0?0:nc;
		return *this;
	}
};

struct rlmap {
  //private:
	ushort		**_;
	ushort		rows,cols;
	vector<coordinates>	dirty;
	ushort*& operator[](ushort i) { if(i<rows) return _[i]; return _[0]; }
  public:
	rlmap(ushort r, ushort c):rows(r),cols(c) {
		_ = new ushort*[rows]; for(r=0;r<rows;r++)	_[r]=new ushort[cols];
	}
	~rlmap() {
		for(int r=0;r<rows;r++) delete [] _[r]; delete [] _;
	}
	void fill(const ushort i) {
		for(int r=0;r<rows;r++) for(int c=0;c<cols;c++) {
			_[r][c]=i;
			dirty.push_back(coordinates(r,c));
		}
	}
	void fill(const ushort (*f)(void)) {
		for(int r=0;r<rows;r++) for(int c=0;c<cols;c++) {
			_[r][c]=f();
			dirty.push_back(coordinates(r,c));
		}
	}
	bool fill(ushort ch, ushort sr, ushort sc, ushort rc, ushort cc) {
		if(sr>=rows)	return false; 
		if(sc>=cols)	return false; 
		if(sr+rc>=rows)	return false; 
		if(sc+cc>=cols)	return false; 
		for(int r=sr;r<sr+rc;r++) for(int c=sc;c<sc+cc;c++) { _[r][c]=ch; dirty.push_back(coordinates(r,c)); }
		return true;
	}
	bool fill(const ushort (*f)(void), ushort sr, ushort sc, ushort rc, ushort cc) {
		if(sr>=rows)	return false; 
		if(sc>=cols)	return false; 
		if(sr+rc>=rows)	return false; 
		if(sc+cc>=cols)	return false; 
		for(int r=sr;r<sr+rc;r++) for(int c=sc;c<sc+cc;c++) { _[r][c]=f(); dirty.push_back(coordinates(r,c)); }
		return true;
	}
	bool vertical(ushort ch,ushort sr,ushort sc,ushort count) { return fill(ch,sr,sc,1,count); }
	bool horizontal(ushort ch,ushort sr, ushort sc, ushort count) { return fill(ch,sr,sc,count,1); }
	bool copy(const rlmap &m,ushort sr,ushort sc, ushort rc, ushort cc,bool copyblanks=true) {
		if(sr>=rows||sc>=cols)			return false;
		sr=sr<1?1:sr;
		sc=sc<1?1:sc;
		//if(sr+rc>=rows||sc+cc>=cols)	return false;
		while(sr+rc>rows) rc--;
		while(sc+cc>cols) cc++;
		if(copyblanks) for(int r=sr;r<sr+rc;r++) for(int c=sc;c<sc+cc;c++) { 
			_[r][c]=m.get(r,c); dirty.push_back(coordinates(r,c)); 
		}
		else for(int r=sr;r<sr+rc;r++) for(int c=sc;c<sc+cc;c++) { 
			ushort t = m.get(r,c);
			if(t!=floor_open||t!=terr_unknown) { _[r][c]=m.get(r,c); dirty.push_back(coordinates(r,c)); } 
		}
		return true;
	}
	
	
	bool fill(ushort ch, coordinates co, coordinates count) {
		if(co.row>=rows)			return false; 
		if(co.col>=cols)			return false; 
		if(co.row+count.row>=rows)	return false; 
		if(co.col+count.col>=cols)	return false; 
		for(int r=co.row;r<co.row+count.row;r++) 
			for(int c=co.col;c<co.col+count.col;c++) { 
				_[r][c]=ch; dirty.push_back(coordinates(r,c)); 
			}
		return true;
	}
	void dump(uchar sr,uchar sc,bool displayblanks=false) {
		if(dirty.empty())	return;
		if(displayblanks==false) {
			for(auto i:dirty) {
				if(_[i.row][i.col]!=open_floor) DisplayChar(sr+i.row,sc+i.col,_[i.row][i.col]);
			}
		}
		else for(auto i:dirty) 	{
			DisplayChar(sr+i.row,sc+i.col,_[i.row][i.col]);
		}
		dirty.clear();
	}
	void set(ushort r,ushort c,const ushort v) { _[r][c]=v; dirty.push_back(coordinates(r,c)); }
	void set(coordinates c, ushort v) { _[c.row][c.col]=v; dirty.push_back(c); }
	cush get(ushort r, ushort c) const { return _[r][c]; }
	cush get(const coordinates c) const { return _[c.row][c.col]; }
};

bool room(rlmap &m, uchar sr, uchar sc, uchar rc, uchar cc, uchar doors=0) {
	bool roomdrawn = m.fill((' '|0<<8),sr,sc,rc,cc);
	uchar dr,dc;
	// extremely naive and wrong POC
	if(roomdrawn) while(doors--) {
		dr = rand()%rc;
		dc = rand()%cc;
		m.set(sr+dr,sc+dc,'D'|rlutil::BROWN<<8);
	}
	return roomdrawn;
}

time_t dig_dungeon2(rlmap &m,time_t s=0) {
	time_t seed = s?s:time(0);
	srand(seed);
	coordinates co;
	uchar rc,cc,count,choice;
	co.row = rrnd(2,48);
	co.col = rrnd(2,148);
	
	for(int i=0;i<50;i++) {
		//m.dump(0,0,true);
		//rlutil::anykey();
		choice = rand()%3;
		switch(choice) {
			case 0:	// dig a NS passage
				rc = rrnd(1,24);
				m.vertical(open_floor,co.row,co.col,rc);
				co.row+=rrnd(rc-1,rc+1);
				if(co.row<2||co.row>48)	co.row=rrnd(2,48);
				break;
			case 1: // dig an EW passage
				cc = rrnd(2,74);
				m.horizontal(open_floor,co.row,co.col,cc);
				co.col+=rrnd(cc-1,cc+1);
				if(co.col<2||co.col>148)	co.col = rrnd(2,148);
				break;
			case 2: // dig a room
				rc = rrnd(2,8);
				cc = rrnd(2,8);
				m.fill(open_floor,co,coordinates(rc,cc));
				co.row+=rrnd(rc-1,rc+1);
				co.col+=rrnd(cc-1,cc+1);
				if(co.row<2||co.row>48)		co.row=rrnd(2,48);
				if(co.col<2||co.col>148)	co.col = rrnd(2,148);
				break;
		}
		
	} // END BIG FOR LOOP
	// one stairway up...
	do {
			co.row = rrnd(2,48);
			co.col = rrnd(2,148);
			rc = (uchar)m.get(co.row,co.col);
	} while((uchar)rc!=' ');
	m.set(co.row,co.col,stairs_up);
	
	// and one stairway down
	do {
			co.row = rrnd(2,48);
			co.col = rrnd(2,148);
			rc = (uchar)m.get(co.row,co.col);
	} while((uchar)rc!=' ');
	m.set(co.row,co.col,stairs_down);
	// the end
	return seed;
}

time_t dig_dungeon(rlmap &m,time_t s=0) {
	time_t seed = s?s:time(0);
	srand(seed);
	uchar sr,sc,rc,cc,choice;
	for(int i=0;i<32;i++) {
		sr = rrnd(2,48);
		sc = rrnd(2,148);
		rc = rrnd(2,16);
		cc = rrnd(2,16);
		choice = rand()%16;
		switch(choice) {
			case 0:	
				if(!(rand()%2))	m.fill((' '|0<<8),sr,sc,1,cc);
				else			m.fill((' '|0<<8),sr,sc,rc,1);
				
				break;
			case 1:
				choice = rrnd(10,80);
				do {
					m.set(sr,sc,' '|0<<8);
					switch(rand()%4) {
						case 0: sr++; break;
						case 1: sr--; break;
						case 2: sc++; break;
						case 3: sc--; break;
					}
				
				} while(choice--&&sc>2&&sc<148&&sr>2&&sr<48);
				break;
			case 2:
				choice = rrnd(10,400);
				do {
					m.fill(' '|0<<8,sr,sc,1,rrnd(2,10)); //set(sr,sc,' '|0<<8);
					switch(rand()%4) {
						case 0: sr++; break;
						case 1: sr--; break;
						case 2: sc++; break;
						case 3: sc--; break;
					}
				
				} while(choice--&&sc>2&&sc<148&&sr>2&&sr<48);
				break;
			case 3:
				choice = rrnd(10,40);
				do {
					m.fill(' '|0<<8,sr,sc,rrnd(2,10),1); //m.set(sr,sc,' '|0<<8);
					switch(rand()%4) {
						case 0: sr++; break;
						case 1: sr--; break;
						case 2: sc++; break;
						case 3: sc--; break;
					}
					
				} while(choice--&&sc>2&&sc<148&&sr>2&&sr<48);
				break;
			default:	
				m.fill((' '|0<<8),sr,sc,rc,cc);
				
				break;
		}
		
	}
	
	// create one stairway up 
	do {
			sr = rrnd(2,48);
			sc = rrnd(2,148);
			rc = (uchar)m.get(sr,sc);
	} while((uchar)rc!=' ');
	m.set(sr,sc,stairs_up);
	
	// and one stairway down
	do {
			sr = rrnd(2,48);
			sc = rrnd(2,148);
			rc = (uchar)m.get(sr,sc);
	} while((uchar)rc!=' ');
	m.set(sr,sc,stairs_down);
	// the end
	return seed;
}



cush tree(void) { return tree_values[rand()%5]|tree_colors[rand()%5]<<8; }

bool room20x20(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,2,2); }
bool room20x30(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,2,3); }
bool room20x40(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,2,4); }
bool room30x20(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,3,2); }
bool room40x20(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,4,3); }
bool room30x30(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,3,3); }
bool room30x40(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,3,4); }
bool room30x50(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,3,5); }
bool room30x60(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,3,6); }
bool room40x30(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,4,3); }
bool room50x30(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,5,3); }
bool room60x30(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,6,3); }
bool room40x40(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,4,4); }
bool room40x50(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,4,5); }
bool room40x60(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,4,6); }
bool room50x40(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,5,4); }
bool room60x40(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,6,4); }
bool room50x50(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,5,5); }
bool room60x60(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,6,6); }
bool room50x20(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,5,2); }
bool room50x60(rlmap &m, uchar sr, uchar sc) { return room(m,sr,sc,5,6); }
typedef bool (*roomfunc)(rlmap &, uchar, uchar);

#define NORTH	digger.row=digger.row-1<2?2:digger.row-1
#define SOUTH	digger.row=digger.row+1>m.rows-2?m.rows-2:digger.row+1
#define EAST 	digger.col=digger.col+1>m.cols/2?m.cols/2:digger.col+1
#define WEST	digger.col=digger.col-1<2?2:digger.col-1
#define NORTH2	digger2.row=digger2.row-1<2?2:digger2.row-1
#define SOUTH2	digger2.row=digger2.row+1>m.rows-2?m.rows-2:digger2.row+1
#define EAST2	digger2.col=digger2.col+1>m.cols-2?m.cols-2:digger2.col+1
#define WEST2	digger2.col=digger2.col-1<m.cols/2?m.cols/2:digger2.col-1
	
time_t dig_dungeon3(rlmap &m, time_t s=1420161167,unsigned int iterations=300) {
	s = s?s:time(0);
	srand(s);
	//coordinates digger(rrnd(2,48),rrnd(2,148));
	coordinates digger(24,74);
	ushort rc;
	for(int i=0;i<iterations;i++) {
		switch(rand()%12) {
			case 0:	
				rc=3+rand()%5;
				while(rc--) {
					m.set(digger,open_floor);
					NORTH; 
				}
				break;
			case 1: 
				rc=3+rand()%5;
				while(rc--) {
					m.set(digger,open_floor);
					SOUTH; 
				}
				break;
			case 2:
				rc=3+rand()%5;
				while(rc--) {
					m.set(digger,open_floor);
					EAST; 
				}
				break;
			case 3:
				rc=3+rand()%5;
				while(rc--) {
					m.set(digger,open_floor);
					WEST; 
				}
				break;
				
			case 4:	
				rc=6+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					NORTH; 
				}
				break;
			case 5: 
				rc=6+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					SOUTH; 
				}
				break;
			case 6:
				rc=6+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					EAST; 
				}
				break;
			case 7:
				rc=6+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					WEST; 
				}
				break;
			default:
				rc = rand()%20;
				switch(rc) {
					case 0:		room20x20(m,digger.row,digger.col); break;
					case 1:		room20x30(m,digger.row,digger.col); break;
					case 2:		room20x40(m,digger.row,digger.col); break;
					case 3:		room30x20(m,digger.row,digger.col); break;
					case 4:		room30x30(m,digger.row,digger.col); break;
					case 5:		room30x40(m,digger.row,digger.col); break;
					case 6:		room30x50(m,digger.row,digger.col); break;
					case 7:		room30x60(m,digger.row,digger.col); break;
					case 8:		room40x20(m,digger.row,digger.col); break;
					case 9:		room40x30(m,digger.row,digger.col); break;
					case 10:	room40x40(m,digger.row,digger.col); break;
					case 11:	room40x50(m,digger.row,digger.col); break;
					case 12:	room40x60(m,digger.row,digger.col); break;
					case 13:	room50x20(m,digger.row,digger.col); break;
					case 14:	room50x30(m,digger.row,digger.col); break;
					case 15:	room50x40(m,digger.row,digger.col); break;
					case 16:	room50x50(m,digger.row,digger.col); break;
					case 17:	room50x60(m,digger.row,digger.col); break;
					case 18:	room60x30(m,digger.row,digger.col); break;
					case 19:	room60x40(m,digger.row,digger.col); break;
					default:	room60x60(m,digger.row,digger.col);
				}
				break;
		}
	}
	// one stairway up...
	do {
			digger.row = rrnd(2,48);
			digger.col = rrnd(2,148);
			rc = (uchar)m.get(digger.row,digger.col);
	} while((uchar)rc!=' ');
	m.set(digger.row,digger.col,stairs_up);
	// and one stairway down
	do {
			digger.row = rrnd(2,48);
			digger.col = rrnd(2,148);
			rc = (uchar)m.get(digger.row,digger.col);
	} while((uchar)rc!=' ');
	m.set(digger.row,digger.col,stairs_down);
	// the end
	return s;
}

time_t dig_dungeon4(rlmap &m, time_t s=0,unsigned int iterations=320) {
	s = s?s:time(0);
	srand(s);
	//coordinates digger(rrnd(2,48),rrnd(2,148));
	coordinates digger(m.rows/2,m.cols/2);
	coordinates digger2(m.rows/2,m.cols/2);
	ushort rc;
	for(int i=0;i<iterations;i++) {
		switch(rand()%12) {
			case 0:	
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					NORTH; 
				}
				break;
			case 1: 
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					SOUTH; 
				}
				break;
			case 2:
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					EAST; 
				}
				break;
			case 3:
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger,open_floor);
					WEST; 
				}
				break;
				
			case 4:	
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger2,open_floor);
					NORTH2; 
				}
				break;
			case 5: 
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger2,open_floor);
					SOUTH2; 
				}
				break;
			case 6:
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger2,open_floor);
					EAST2; 
				}
				break;
			case 7:
				rc=3+rand()%15;
				while(rc--) {
					m.set(digger2,open_floor);
					WEST2; 
				}
				break;
			default:
				rc = rand()%40;
				switch(rc) {
					case 0:		room20x20(m,digger.row,digger.col); break;
					case 1:		room20x30(m,digger.row,digger.col); break;
					case 2:		room20x40(m,digger.row,digger.col); break;
					case 3:		room30x20(m,digger.row,digger.col); break;
					case 4:		room30x30(m,digger.row,digger.col); break;
					case 5:		room30x40(m,digger.row,digger.col); break;
					case 6:		room30x50(m,digger.row,digger.col); break;
					case 7:		room30x60(m,digger.row,digger.col); break;
					case 8:		room40x20(m,digger.row,digger.col); break;
					case 9:		room40x30(m,digger.row,digger.col); break;
					case 10:	room40x40(m,digger.row,digger.col); break;
					case 11:	room40x50(m,digger.row,digger.col); break;
					case 12:	room40x60(m,digger.row,digger.col); break;
					case 13:	room50x20(m,digger.row,digger.col); break;
					case 14:	room50x30(m,digger.row,digger.col); break;
					case 15:	room50x40(m,digger.row,digger.col); break;
					case 16:	room50x50(m,digger.row,digger.col); break;
					case 17:	room50x60(m,digger.row,digger.col); break;
					case 18:	room60x30(m,digger.row,digger.col); break;
					case 19:	room60x40(m,digger.row,digger.col); break;
					case 20:		room20x20(m,digger2.row,digger2.col); break;
					case 21:		room20x30(m,digger2.row,digger2.col); break;
					case 22:		room20x40(m,digger2.row,digger2.col); break;
					case 23:		room30x20(m,digger2.row,digger2.col); break;
					case 24:		room30x30(m,digger2.row,digger2.col); break;
					case 25:		room30x40(m,digger2.row,digger2.col); break;
					case 26:		room30x50(m,digger2.row,digger2.col); break;
					case 27:		room30x60(m,digger2.row,digger2.col); break;
					case 28:		room40x20(m,digger2.row,digger2.col); break;
					case 29:		room40x30(m,digger2.row,digger2.col); break;
					case 30:	room40x40(m,digger2.row,digger2.col); break;
					case 31:	room40x50(m,digger2.row,digger2.col); break;
					case 32:	room40x60(m,digger2.row,digger2.col); break;
					case 33:	room50x20(m,digger2.row,digger2.col); break;
					case 34:	room50x30(m,digger2.row,digger2.col); break;
					case 35:	room50x40(m,digger2.row,digger2.col); break;
					case 36:	room50x50(m,digger2.row,digger2.col); break;
					case 37:	room50x60(m,digger2.row,digger2.col); break;
					case 38:	room60x30(m,digger2.row,digger2.col); break;
					case 39:	room60x40(m,digger2.row,digger2.col); break;
					default:	
						if(!rand()%2)	room60x60(m,digger.row,digger.col);
						else			room60x60(m,digger2.row,digger2.col);
				}
				break;
		}
	}
	// one stairway up...
	do {
			digger.row = rrnd(2,m.rows);
			digger.col = rrnd(2,m.cols);
			rc = (uchar)m.get(digger.row,digger.col);
	} while((uchar)rc!=' ');
	m.set(digger.row,digger.col,stairs_up);
	// and one stairway down
	do {
			digger.row = rrnd(2,m.rows);
			digger.col = rrnd(2,m.cols);
			rc = (uchar)m.get(digger.row,digger.col);
	} while((uchar)rc!=' ');
	m.set(digger.row,digger.col,stairs_down);
	// the end
	return s;
}
#endif
