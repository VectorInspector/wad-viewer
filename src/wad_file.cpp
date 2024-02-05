#include "wad_file.h"
#include "file_helper.h"

DoomWad::DoomWad () {
	ParseHeader();
}

DoomWad::DoomWad (std::string path) {
	SlurpByteFile(data, path);
	ParseHeader();
}

void DoomWad::ParseHeader () {
	// The DOOM wad header is 4 bytes of identification, followed by the count of lumps and
	// the location of the directory list. It can only be PWAD or IWAD.
	if(data.size() < 12) {
		header [0] = ' ';
		header [1] = ' ';
		header [2] = ' ';
		header [3] = ' ';
		lump_count = 0;
		table_offset = 0;
	}
	
	else {
		header [0] = data [0];
		header [1] = data [1];
		header [2] = data [2];
		header [3] = data [3];
		lump_count = *reinterpret_cast <int*> (&data [4]);
		table_offset = *reinterpret_cast <int*> (&data [8]);
	}
	
	// Determine correctness of wad header.
	auto pwad_header = "PWAD";
	auto iwad_header = "IWAD";
	
	if(
	std::equal(pwad_header, pwad_header + 4, header) ||
	std::equal(iwad_header, iwad_header + 4, header)) {
		has_wad_data = true;
	}
	
	else {
		has_wad_data = false;
	}
}

bool DoomWad::IsLoaded () const {
	return has_wad_data && 12 <= data.size();
}

void DoomWad::BeginLumpSearch () {
	lump_pos = 0;
	current_lump = reinterpret_cast <LumpInfo*> (&data [table_offset]);
}

bool DoomWad::FindLump (std::string name) {
	char buffer [8] = { '\0' };
	lump_exists = false;
	
	if(lump_count <= lump_pos) {
		return false;
	}
	
	int name_count = std::min(8ULL, name.size());
	std::copy(name.begin(), name.begin() + name_count, buffer);
	bool searching = true;
	
	while(searching) {
		
		if(std::equal(buffer, buffer + 8, current_lump->name)) {
			searching = false;
		}
		
		else {
			current_lump++;
			lump_pos++;
			
			if(lump_count <= lump_pos) {
				return false;
			}
		}
	}
	
	lump_exists = true;
	return true;
}

bool DoomWad::LumpExists () const {
	return lump_exists;
}
