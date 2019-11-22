
#include <iostream>
#include <iomanip>
#include <algorithm> 
#include <fstream>
#include <string>
#include <vector>
#include "pole.h"


void showUsage() {
	std::cout << "showsheets {file}" << std::endl;
}

enum class fileType : unsigned char {
	OLESS,
	ZIP,
	Other
};

fileType ReadFileType(std::string file) {
	std::ifstream stream(file, std::ios::binary | std::ios::ate);
	if (stream.is_open()) {
		char buffer[4];
		stream.seekg(0, std::ios::beg);
		stream.read(buffer, 4);
		stream.close();

		if (((unsigned char)buffer[0]) == 0xD0 && ((unsigned char)buffer[1]) == 0xCF && ((unsigned char)buffer[2]) == 0x11 && ((unsigned char)buffer[3]) == 0xE0) {
			return fileType::OLESS;
		} else if (((unsigned char)buffer[0]) == 0x50 && ((unsigned char)buffer[1]) == 0x4B) {
			return fileType::ZIP;
		}
	}
	return fileType::Other;
}

struct RecordHeader {
	unsigned short Type;
	unsigned short Length;
};

// see: https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-xls/b9ec509a-235d-424e-871d-f8e721106501
struct BoundSheetHeader {
	unsigned int lbPlyPos;
	unsigned short hsState : 2;
	unsigned short unused1 : 6;
	unsigned short dt : 8;
};
// see: https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-xls/05162858-0ca9-44cb-bb07-a720928f63f8
struct ShortXLUnicodeString {
	unsigned char cch;
	unsigned short fHighByte : 1;
	unsigned short reserved1 : 7;
	std::string rgb;

	static ShortXLUnicodeString Read(const unsigned char* buffer, const unsigned int offset) {

		ShortXLUnicodeString ans;
		unsigned int index = offset;

		ans.cch = buffer[index];
		index += 1;

		ans.fHighByte = buffer[index] & 128;
		ans.reserved1 = buffer[index] & 127;
		index++;

		int byteCount = 0;
		if (ans.fHighByte == 0x0) {
			byteCount = ans.cch;
		}
		else {
			byteCount = ans.cch * 2;
		}

		std::string name(reinterpret_cast<char const*>(buffer + index), byteCount);
		ans.rgb = name;

		return ans;
	}
};

std::string GetVisibilityStr(unsigned short visibility) {
	std::string ans;
	switch (visibility) {
	case 0x00: ans = "Visible"; break;
	case 0x01: ans = "Hidden"; break;
	case 0x02: ans = "Very Hidden"; break;
	default: ans = "Unknown/Undocumented visiblitiy"; break;
	}
	return ans;
}

void makeSheetsVisible(std::string xlsFile) {
	POLE::Storage* storage = new POLE::Storage(xlsFile.c_str());
	storage->open(true, false);
	if (storage->result() == POLE::Storage::Ok) {

		std::list<std::string> entries;
		entries = storage->entries("/");
		std::list<std::string>::iterator it;
		for (it = entries.begin(); it != entries.end(); it++) {
			std::string name = *it;
			if (name == "Workbook" || name == "Book" || name == "WorkBook") {
				POLE::Stream* stream = new POLE::Stream(storage, "/" + name);

				unsigned char recordHeader[4];

				while (! stream->eof()) {
					auto bytesRead = stream->read(recordHeader, 4);
					if (bytesRead != 4) {
						break;
					}
					else {
						auto header = reinterpret_cast<RecordHeader*>(recordHeader);

						unsigned char* block = new unsigned char[header->Length];
						bytesRead = stream->read(block, header->Length);

						if (bytesRead == header->Length) {

							if (header->Type == 0x0085) { //BoundSheet

								auto bs = reinterpret_cast<BoundSheetHeader*>(block);
								auto name = ShortXLUnicodeString::Read(block, 6);
								std::cout << "Sheet: \"" << name.rgb << "\", Visiblity: \"" << GetVisibilityStr(bs->hsState) << "\"" << std::endl;
								if (bs->hsState != 0) {
									bs->hsState = 0;

									auto pos = stream->tell();
									stream->seek(pos-(header->Length));
									
									auto bytesWritten = stream->write(block, 6);
									if (bytesWritten != 6) {
										std::cout << "Could not write to stream!" << std::endl;
									}

									stream->seek(pos);
								}
							}
						}
						delete[] block;
					}
				}
				stream->flush();
				delete stream;
			}
		}
	}
	else {
		std::cerr << "Could not open file as OLESS file for writing" << std::endl;
	}

	storage->close();
	delete storage;
}


int main(int argc, char* argv[])
{
	std::vector<std::string> arguments(argv + 1, argv + argc);

	if (argc > 1) {
		auto file = arguments[0];
		auto ftype = ReadFileType(file);

		if (ftype == fileType::OLESS) {
			makeSheetsVisible(file);
		} else if (ftype == fileType::ZIP) {
			std::cerr << "I HAVEN'T DONE THIS WORK YET!!" << std::endl;
		} else {
			std::cerr << "Unsupported file type" << std::endl;
		}
	}
	else {
		showUsage();
	}
	return 0;
}
