#include "goaldis.h"
#include <string>

set<string> seen;

struct dgo_header {
	uint32_t length;
	char rootname[60]; // TODO assume that the lengths of the names is never any longer than 60 for jak2/3, might be an issue later

    dgo_header(uint32_t length, char rootname) {
        this->length = length;
        this->rootname[60] = rootname; // TODO might not work as intended, check
    }
};

bool isArtGroup(uint8_t *data, size_t size) {
	const char *str = "-ag.go";
	size_t ex = strlen(str);
	size -= ex;
	for (size_t n = 0;n<size;n++)
		if (sizeof &data[n] >= 6 && memcmp(&data[n], str, ex) == 0)
			return true;
	return false;
}

void loadDgo(const char *dgoname, bool shouldDump) {
	printf("Loading %s...\n", dgoname);

	FILE *file = fopen(dgoname, "rb");
	if (!file) {
		perror(dgoname);
		exit(1);
	}

	uint32_t fileSize;
	uint8_t *fileData;

	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	fileData = new uint8_t[fileSize];
	// Load all of the bytes from the file into the array
	fread(fileData, 1, fileSize, file);
	fclose(file);

	// The first value of the header is the number of files to go through as the files are just concatenated together
	// The headers for Jak 2 the first 4 bytes is the same across all files.
	// Originally + 0 because it is the first value in the file
	uint32_t headerCount = *(uint32_t *)(fileData + 0);

	// This structure assumes that the first 4 bytes of the file is an integer holding the number of files contained
	// It also assumes that the name of the file will be within the next 60 bytes.
	// In the Jak 2 files, there is no such constant size, however they appear to whitespace terminated '\32'
	dgo_header *dgo = (dgo_header *)fileData;

	// This then assumes that the size (number of bytes) will be in the next 4byte block, followed by another 60 bytes for the first files name
	// This appears to be there in the jak 2 files as well but not at a consistent spacing
	dgo_header *entry = (dgo_header *)(dgo + 1);

	for (uint32_t n = 0; n<headerCount; n++) {
		// Move to after the reserved space for the filename, and store that location
		uint8_t *entryData = (uint8_t *)(entry + 1);

		// Auto-detect art-group entries and rename them to avoid conflicts.
		string name = entry->rootname;
		if (isArtGroup(entryData, entry->length))
			name += "-ag";

		if (seen.find(name) != seen.end()) {
			printf("Multiple entries for %s\n", name.c_str());
			exit(1);
		}
		seen.insert(name);

		MetaGoFile *go = new MetaGoFile;
		go->shouldDump = shouldDump;
		go->name = entry->rootname;
		go->fileName = name;
		go->dgoname = dgoname;
		go->rawdata.resize(entry->length);

		// Main error here, access violation, the length value is way too huge due to above assumptions not working for jak2/3
		memcpy(&go->rawdata[0], entryData, entry->length);
		metaGoFiles.push_back(go);

        // Display current go file?
		metaLoadingGo = go;
		link_and_exec(entryData, entry->length);
		metaLoadingGo = NULL;

		// Go to the next file by making a new header that points to the next file
		entry = (dgo_header *)((char *)entryData + entry->length);
	}

	delete[] fileData;
}

void dumpAsm(const char *outdir, MetaGoFile *go) {
	char fileName[1024];
	sprintf(fileName, "%s\\%s.asm", outdir, go->fileName.c_str());

	FILE *fp = fopen(fileName, "w");
	if (!fp) {
		perror(fileName);
		exit(1);
	}

	disasmFile(fp, go, false);
	disasmFile(fp, go, true);
	fclose(fp);
}

void dumpRawBin(const char *outdir, MetaGoFile *go) {
	char tmp[256];
	sprintf(tmp, "%s/%s.bin", outdir, go->fileName.c_str());
	printf("%s -> %s\n", go->name.c_str(), tmp);
	FILE *out = fopen(tmp, "wb");
	if (!out) {
		perror(tmp);
		exit(1);
	}
	fwrite(&go->rawdata[0], 1, go->rawdata.size(), out);
	fclose(out);
}

// Takes in one file that is already in a workable .go or .o format
void dumpAsmFile(const char *outdir, const char *inputFile) {

    printf("Loading %s...\n", inputFile);

    FILE *file = fopen(inputFile, "rb");

    uint32_t fileSize;
    uint8_t *fileData;

    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fileData = new uint8_t[fileSize];
    // Load all of the bytes from the file into the array
    fread(fileData, 1, fileSize, file);
    fclose(file);

    // Setup metagofile structure
    MetaGoFile *go = new MetaGoFile;
    go->shouldDump = true;
    go->name = "eichar-ag"; // TODO temporary fixes here
    go->fileName = "eichar-ag"; // In the ps3 files, they are already prefixed
    go->dgoname = "ART.CGO";
    go->rawdata.resize(fileSize);

    // TODO hopefully this works, but just read the entire contents of the file into the struct
    memcpy(&go->rawdata[0], fileData, fileSize);

    // TODO Display current go file, probably can get rid of this 
    metaLoadingGo = go;
    link_and_exec(fileData, fileSize);
    metaLoadingGo = NULL;

    // Dissassemble process
    dumpAsm(outdir, go);
}

int main(int argc, char *argv[]) {

	// Add -file parameter to pass in an individual .go or .o files rather than a CGO or DGO container
	if (argc < 4) {
		fprintf(stderr, "PS2 DGO/CGO Usage: goaldis -bin/-asm output-dir input.dgo\n");
		fprintf(stderr, "Inputs can be .CGO or .DGO\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "e.g. goaldis -asm C:\\kernel CGO\\KERNEL.CGO\n");
		fprintf(stderr, "PS3 Individual File Usage: goaldis mode output-dir input.go\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Inputs can be .go or .o\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "e.g. goaldis -file C:\\kernel CGO\\KERNEL.CGO\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "    Mode:\n");
		fprintf(stderr, "        -bin       Extract raw binary files\n");
		fprintf(stderr, "        -asm       Disassemble back to MIPS assembly\n");
		fprintf(stderr, "        -file      Disassemble an individual .go or .o file\n");
		return 1;
	}

	// TODO remove this comment later,  dumpall.bat H:\Jak2\disassembly H:\Jak2\

	const char *mode = argv[1];
	const char *outdir = argv[2];
	const char *inputFile = argv[3];
	
	// TODO dont delete a directory if already created
	_mkdir(outdir);
	InitMachine();

	if (!strcmp(mode, "-bin")) {

		loadDgo(inputFile, true);
		for (MetaGoFile *go : metaGoFiles)
			dumpRawBin(outdir, go);
	}
	else if (!strcmp(mode, "-asm")) {

		loadDgo(inputFile, true);
		for each (MetaGoFile *go in metaGoFiles)
			dumpAsm(outdir, go);
	}
	else if (!strcmp(mode, "-file")) {
		dumpAsmFile(outdir, inputFile);
	}

	// Error occured
	else {
		fprintf(stderr, "goaldis: Invalid mode '%s'.\n", mode);
		return 1;
	}
	return 0;
}

