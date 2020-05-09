
//{{BLOCK(map)

//======================================================================
//
//	map, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 17 tiles (t|f reduced) not compressed
//	+ regular map (flat), not compressed, 32x32 
//	Total size: 512 + 544 + 2048 = 3104
//
//	Time-stamp: 2020-05-09, 22:47:48
//	Exported by Cearn's GBA Image Transmogrifier, v0.8.15
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_MAP_H
#define GRIT_MAP_H

#define mapTilesLen 544
extern const unsigned int mapTiles[136];

#define mapMapLen 2048
extern const unsigned short mapMap[1024];

#define mapPalLen 512
extern const unsigned short mapPal[256];

#endif // GRIT_MAP_H

//}}BLOCK(map)
