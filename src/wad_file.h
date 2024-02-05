#ifndef WAD_FILE_H
#define WAD_FILE_H

#include <string>
#include <vector>

struct DoomWad {
	
	// Empty wad file.
	DoomWad ();
	
	// Read from file.
	DoomWad (std::string path);
	
	void ParseHeader ();
	bool IsLoaded () const;
	
	bool has_wad_data;
	std::vector <char> data;
	
	// Header info.
	char header [4];
	int lump_count;
	int table_offset;
	
	// Lump iterator.
	struct LumpInfo {
		int offset;
		int size;
		char name [8];
	};
	
	void BeginLumpSearch ();
	bool FindLump (std::string name);
	bool LumpExists () const;
	
	bool		lump_exists;
	LumpInfo*	current_lump;
	int			lump_pos;
};

#endif
