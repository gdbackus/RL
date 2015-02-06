#include "rlmap.h"
#include <set>
using std::set;
#include <map>
using std::map;

struct object {
	ushort 		id;
	coordinates	pos;
	ushort		dispglyph;
	object(ushort c=0):dispglyph(c) {}
};

struct actor {
	ushort		id;
	coordinates pos;
	ushort		dispglyph;
	uchar		vision;
	rlmap		*knownmap;
	void 		*ai;
	actor(ushort c=0):dispglyph(c),vision(3),knownmap(NULL),ai(NULL) { }
	~actor() { 
		if(knownmap)	delete knownmap;
	}
	bool create_knownmap(ushort r, ushort c) {
		knownmap = new rlmap(r,c);
		return knownmap?true:false;
	}
	void destroy_knownmap(void) { if(knownmap) delete knownmap; }
	void display_knownmap(ushort r=0,ushort c=0) { if(knownmap) knownmap->dump(r,c,true); }
};

struct DM {
	rlmap	*terrain;
	rlmap	*actors;
	rlmap	*objects;
	map<ushort,actor>	actor_db;
	map<ushort,object>	object_db;
	ushort create_actor(ushort c) {
		ushort k;
		do { k = rrnd(1,3000); } while(actor_db.find(k)!=actor_db.end());
		actor_db[k].dispglyph=c; actor_db[k].id=k; return k;
	}
	ushort create_object(ushort c) {
		ushort k;
		do { k = rrnd(3001,6000); } while(object_db.find(k)!=object_db.end());
		object_db[k].dispglyph=c;
		object_db[k].id = k; return k;
	}
	bool			has_actor(ushort k) const 	{ return actor_db.find(k)!=actor_db.end(); }
	actor& 			ref_actor(ushort k) 		{ return actor_db[k]; }
	bool			has_object(ushort k) const 	{ return object_db.find(k)!=object_db.end(); }
	object&			ref_objest(ushort k) 		{ return object_db[k]; }
	DM(ushort r,ushort c) {
		terrain = new rlmap(r,c);
		actors = new rlmap(r,c);
		objects = new rlmap(r,c);
	}
	~DM() {
		delete terrain;
		delete actors;
		delete objects;
	}
	coordinates loc_stairs_up(void) const {
		ushort r=0,c=0,rc=terrain->rows,cc=terrain->cols;
		for(r=0;r<rc;r++) {
			for(c=0;c<cc;c++) {
				if( (*terrain)[r][c] == stairs_up)	{ 
					return coordinates(r,c);
				}
			}
		}
		return coordinates(0,0);
	}
	coordinates loc_stairs_down(void) const { 
		ushort r=0,c=0,rc=terrain->rows,cc=terrain->cols;
		for(r=0;r<rc;r++) {
			for(c=0;c<cc;c++) {
				if( (*terrain)[r][c] == stairs_down)	{ 
					return coordinates(r,c);
				}
			}
		}
		return coordinates(0,0);
	}
	coordinates	random_open_space(void) const {
		ushort r=0,c=0;
		while(terrain->get(r,c)!=open_floor) {
			r = rrnd(2,terrain->rows-2);
			c = rrnd(2,terrain->cols-2);
		}
		return coordinates(r,c);
	}
	
	bool query_move_actor(ushort fr, ushort fc,ushort tr, ushort tc) const {
		if(actors->get(fr,fc)==open_floor)	return false; // no actor there
		if(actors->get(tr,tc)!=open_floor) return false; // already occupied by another actor
		if(terrain->get(tr,tc)==open_floor) return true; // open space
		if(terrain->get(tr,tc)==door_open) return true; // open door
		if(terrain->get(tr,tc)==stairs_up) return true;
		if(terrain->get(tr,tc)==stairs_down) return true;
		return false;
	}
	bool place_actor(actor *a) {
		if(!a||!terrain||!actors)							return false;
		if(actors->get(a->pos.row,a->pos.col)!=open_floor)	return false;
		ushort	terr = terrain->get(a->pos.row,a->pos.col);
		switch(terr) {
			case stairs_up:		// intentional fall through
			case stairs_down:	// intentional fall through
			case open_floor:	actors->set(a->pos,a->dispglyph); 
								draw_actor_map(a); return true; break;
		}
		return false;
	}
	void move_actor(actor *a,ushort tr,ushort tc) {
		if(!query_move_actor(a->pos.row,a->pos.col,tr,tc))	return;
		actors->set(a->pos,open_floor);		// clear position in actors
		terrain->dirty.push_back(a->pos);	// tell terrain to redraw
		objects->dirty.push_back(a->pos);	// tell objects also
		a->pos.row=tr;						// set position in the actor
		a->pos.col=tc;
		actors->set(a->pos,a->dispglyph);	// set position in actor map
	}
	void old_bad_draw_actor_map(actor *a) {
		if(!a||!terrain||!actors||!objects)	return;
		if(!a->knownmap)	a->create_knownmap(terrain->rows,terrain->cols);
		int sr = a->pos.row-a->vision,
			sc = a->pos.col-a->vision,
			rc = 1 + (a->vision*2),
			cc = 1 + (a->vision*2);
		if(sr<0) sr = 0;
		if(sc<0) sc = 0;
		while(sr+rc>terrain->rows) rc--;
		while(sc+cc>terrain->cols) cc--;
		
		a->knownmap->copy(*terrain,sr,sc,rc,cc);
		// this is naive and does not take into account vision being blocked by anything - POC
		// debug DisplayNum(51,0,sr); DisplayNum(51,4,sc);
		// debug DisplayNum(52,0,rc); DisplayNum(52,4,cc);
		a->knownmap->set(a->pos,a->dispglyph);
	}
	void draw_actor_map(actor *a,bool drawplayer=true,bool allowrecurse=true) {
		if(!a||!terrain||!actors||!objects)	return;
		if(!a->knownmap)	a->create_knownmap(terrain->rows,terrain->cols);
		ushort 		lastseen, actorvis=a->vision,vc=1; // vision counter
		coordinates viewpos(a->pos.row,a->pos.col);
		coordinates realpos(a->pos.row,a->pos.col);
		
		set<coordinates> visited;
		
		/* NN */ vc=1; do {
			lastseen = objects->get(viewpos.row-vc, viewpos.col);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row-vc, viewpos.col);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row-vc, viewpos.col);
			a->knownmap->set(viewpos.row-vc,viewpos.col,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else { 
				if(allowrecurse) visited.insert(coordinates(viewpos.row-vc,viewpos.col));
				vc++;
			}
		} while(vc<=actorvis);
		/* EE */ vc=1; do {
			lastseen = objects->get(viewpos.row, viewpos.col+vc);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row, viewpos.col+vc);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row, viewpos.col+vc);
			a->knownmap->set(viewpos.row,viewpos.col+vc,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else {
				if(allowrecurse) visited.insert(coordinates(viewpos.row,viewpos.col+vc));
				vc++;
			}
		} while(vc<=actorvis);
		/* SS */ vc=1; do {
			lastseen = objects->get(viewpos.row+vc, viewpos.col);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row+vc, viewpos.col);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row+vc, viewpos.col);
			a->knownmap->set(viewpos.row+vc,viewpos.col,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else {
				if(allowrecurse) visited.insert(coordinates(viewpos.row+vc,viewpos.col));
				vc++;
			}
		} while(vc<=actorvis);
		/* WW */ vc=1; do {
			lastseen = objects->get(viewpos.row, viewpos.col-vc);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row, viewpos.col-vc);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row, viewpos.col-vc);
			a->knownmap->set(viewpos.row,viewpos.col-vc,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else {
				if(allowrecurse) visited.insert(coordinates(viewpos.row,viewpos.col-vc));
				vc++;
			}
		} while(vc<=actorvis);
		/* NW */ vc=1; do {
			lastseen = objects->get(viewpos.row-vc, viewpos.col-vc);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row-vc, viewpos.col-vc);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row-vc, viewpos.col-vc);
			a->knownmap->set(viewpos.row-vc,viewpos.col-vc,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else {
				if(allowrecurse) visited.insert(coordinates(viewpos.row-vc,viewpos.col-vc));
				vc++;
			}
		} while(vc<=actorvis);
		/* NE */ vc=1; do {
			lastseen = objects->get(viewpos.row-vc, viewpos.col+vc);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row-vc, viewpos.col+vc);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row-vc, viewpos.col+vc);
			a->knownmap->set(viewpos.row-vc,viewpos.col+vc,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else {
				if(allowrecurse) visited.insert(coordinates(viewpos.row-vc,viewpos.col+vc));
				vc++;
			}
		} while(vc<=actorvis);
		/* SW */ vc=1; do {
			lastseen = objects->get(viewpos.row+vc, viewpos.col-vc);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row+vc, viewpos.col-vc);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row+vc, viewpos.col-vc);
			a->knownmap->set(viewpos.row+vc,viewpos.col-vc,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else {
				if(allowrecurse) visited.insert(coordinates(viewpos.row+vc,viewpos.col-vc));
				vc++;
			}
		} while(vc<=actorvis);
		/* SE */ vc=1; do {
			lastseen = objects->get(viewpos.row+vc, viewpos.col+vc);
			if(lastseen==floor_open) lastseen = actors->get(viewpos.row+vc, viewpos.col+vc);
			if(lastseen==floor_open) lastseen = terrain->get(viewpos.row+vc, viewpos.col+vc);
			a->knownmap->set(viewpos.row+vc,viewpos.col+vc,lastseen);
			if(lastseen==wall_stone||lastseen==wall_wood||lastseen==door_closed) vc = actorvis+1; // break out
			else {
				if(allowrecurse) visited.insert(coordinates(viewpos.row+vc,viewpos.col+vc));
				vc++;
			}
		} while(vc<=actorvis);
		if(allowrecurse) {
			a->vision = 1;
			if(visited.size()) {
				for(auto i:visited) {
					a->pos = i;
					draw_actor_map(a,false,false);
				}
			}
			
		}
		a->pos = realpos;
		a->vision = actorvis;
		if(drawplayer)	a->knownmap->set(a->pos,a->dispglyph);
		
	}
	void dump(ushort r=0,ushort c=0) {
		terrain->dump(r,c,true); 	// terrain displays open_floor
		objects->dump(r,c);			// objects & actors does not
		actors->dump(r,c);
	}
};

int main(int argc, char **argv)
{	srand(time(0));
	MaximizeConsole();
    rlutil::hidecursor();
	//DM dm(50,150);
	DM dm(50,50);
	dm.terrain->fill(wall_stone);
	dm.objects->fill(floor_open);
	dm.actors->fill(floor_open);
	
	
	time_t dseed = dig_dungeon4(*dm.terrain);
	
	char inp;
	//ushort a1 = dm.create_actor(playerglyph);
	//dm.ref_actor[a1].pos = dm.loc_stairs_up();
	//dm.place_actor(&dm.ref_actor(a1));
	actor a1(playerglyph);
	a1.pos = dm.loc_stairs_up();
	a1.vision = 3;
	dm.place_actor(&a1);
	
	dm.draw_actor_map(&a1);
	a1.display_knownmap(0,60);
	
	actor a2('M'|rlutil::LIGHTRED<<8);
	a2.pos = dm.loc_stairs_down();
	dm.actors->set(a1.pos,a1.dispglyph);
	dm.actors->set(a2.pos,a2.dispglyph);
	object o1('$'|rlutil::YELLOW<<8);
	o1.pos = dm.random_open_space();
	dm.objects->set(o1.pos,o1.dispglyph);
	while(inp!='q') {
		inp = getch();
		switch(inp) {
			case '1':	dm.move_actor(&a1,a1.pos.row+1,a1.pos.col-1); break;
			case '2':	dm.move_actor(&a1,a1.pos.row+1,a1.pos.col); break;
			case '3':	dm.move_actor(&a1,a1.pos.row+1,a1.pos.col+1); break;
			case '4':	dm.move_actor(&a1,a1.pos.row,a1.pos.col-1); break;
			case '6':	dm.move_actor(&a1,a1.pos.row,a1.pos.col+1); break;
			case '7':	dm.move_actor(&a1,a1.pos.row-1,a1.pos.col-1); break;
			case '8': 	dm.move_actor(&a1,a1.pos.row-1,a1.pos.col); break;
			case '9':	dm.move_actor(&a1,a1.pos.row-1,a1.pos.col+1); break;
		}
		dm.move_actor(&a2,a2.pos.row+rrnd(-1,1),a2.pos.col+rrnd(-1,1));
		
		dm.draw_actor_map(&a1);
		a1.display_knownmap(0,60);
		dm.dump();
	}
	rlutil::setColor(8);
	DisplayNum(55,0,dseed);
	rlutil::showcursor();
	return 0;
}

