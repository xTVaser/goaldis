#include "goaldis.h"
#include "compression.h"

set<string> seen;

struct dgo_header
{
	uint32_t length;
	char rootname[60];
};

bool isArtGroup(uint8_t *data, size_t size)
{
	const char *str = "-ag.go";
	size_t ex = strlen(str);
	size -= ex;
	for (size_t n=0;n<size;n++)
		if (memcmp(&data[n], str, ex) == 0)
			return true;
	return false;
}

void loadDgo(const char *dgoname, bool shouldDump)
{
	printf("Loading %s...\n", dgoname);

	FILE *file = fopen(dgoname, "rb");
	if (!file)
	{
		perror(dgoname);
		exit(1);
	}

	uint32_t fileSize;
	uint8_t *fileData;

	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	fileData = new uint8_t[fileSize];
	fread(fileData, 1, fileSize, file);
	fclose(file);

	decompress(&fileData, &fileSize);

	uint32_t headerCount = *(uint32_t *)(fileData + 0);

	dgo_header *dgo = (dgo_header *)fileData;
	dgo_header *entry = (dgo_header *)(dgo + 1);
	for (uint32_t n=0;n<headerCount;n++)
	{
		uint8_t *entryData = (uint8_t *)(entry + 1);

		// Auto-detect art-group entries and rename them to avoid conflicts.
		string name = entry->rootname;
		if (isArtGroup(entryData, entry->length))
			name += "-ag";

		if (seen.find(name) != seen.end())
		{
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
		memcpy(&go->rawdata[0], entryData, entry->length);
		metaGoFiles.push_back(go);

		metaLoadingGo = go;
		link_and_exec(entryData, entry->length);
		metaLoadingGo = NULL;
		
		entry = (dgo_header *)((char *)entryData + entry->length);
	}

	delete[] fileData;
}

void decompress(const char *dgoname, const char *outdir)
{
	printf("Decompressing %s...\n", dgoname);

	FILE *file = fopen(dgoname, "rb");
	if (!file)
	{
		perror(dgoname);
		exit(1);
	}

	uint32_t fileSize;
	uint8_t *fileData;

	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	fileData = new uint8_t[fileSize];
	fread(fileData, 1, fileSize, file);
	fclose(file);

	decompress(&fileData, &fileSize);

	dgo_header *dgo = (dgo_header *)fileData;

	char tmp[256];
	sprintf(tmp, "%s/%s.bin", outdir, dgo->rootname);
	printf("%s -> %s\n", dgo->rootname, tmp);
	FILE *out = fopen(tmp, "wb");
	if (!out)
	{
		perror(tmp);
		exit(1);
	}
	fwrite(fileData, 1, fileSize, out);
	fclose(out);
}

bool dumpAsm(const char *outdir, MetaGoFile *go)
{
	char fileName[1024];
	sprintf(fileName, "%s\\%s.asm", outdir, go->fileName.c_str());

	FILE *fp = fopen(fileName, "w");
	if (!fp)
	{
		perror(fileName);
		exit(1);
	}

	bool ok = disasmFile(fp, go, false);
	if (!ok) {
		printf("goaldis: Error - Unable to Disassemble File\n");
	}
	if (ok) {
		ok = disasmFile(fp, go, true);
		if (!ok) {
			printf("goaldis: Error - Unable to Disassemble File on Final Pass\n");
		}
	}
	fclose(fp);
	return ok;
}

void dumpRawBin(const char *outdir, MetaGoFile *go)
{
	char tmp[256];
	sprintf(tmp, "%s/%s.bin", outdir, go->fileName.c_str());
	printf("%s -> %s\n", go->name.c_str(), tmp);
	FILE *out = fopen(tmp, "wb");
	if (!out)
	{
		perror(tmp);
		exit(1);
	}
	fwrite(&go->rawdata[0], 1, go->rawdata.size(), out);
	fclose(out);
}

int main(int argc, char *argv[])
{
	/*if (argc != 4)
	{
		fprintf(stderr, "Usage: goaldis mode output-dir input.dgo\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Inputs can be .CGO or .DGO\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "e.g. goaldis mode C:\\kernel CGO\\KERNEL.CGO\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "    Mode:\n");
		fprintf(stderr, "        -dcp       Decompress raw binary files\n");
		fprintf(stderr, "        -bin       Extract raw binary files\n");
		fprintf(stderr, "        -asm       Disassemble back to MIPS assembly\n");
		return 1;
	}*/

	if (argc != 3) {
		fprintf(stderr, "TEMPORARY USAGE - `goaldis <INPUT FILE (.CGO/.DGO)> <OUTPUT DIRECTORY>");
		return 1;
	}


	//const char *mode = argv[1];
	//const char *outDir = argv[2];
	//const char *inDir = argv[3];
	// NOTE - Change Paths! Hacky Testing
	const char *mode = "-asm";
	const char *inputFile = argv[1]; //"C:\\Users\\xtvas\\Repositories\\goaldis\\test\\COMMON.CGO";
	const char *outputDirectory = argv[2]; //"C:\\Users\\xtvas\\Repositories\\goaldis\\test\\output";
	
	_mkdir(outputDirectory);

	if (!strcmp(mode, "-dcp")) {
		decompress(inputFile, outputDirectory);
		return 0;
	}

	InitMachine();

	loadDgo(inputFile, true);

	if (!strcmp(mode, "-asm")) {
		for each (MetaGoFile * go in metaGoFiles) {
			printf("goaldis: Disassembling - '%s'\n", go->fileName.c_str());
			bool success = dumpAsm(outputDirectory, go);
			if (!success) {
				printf("goaldis: Unable to Disassemble - '%s\n'", go->fileName.c_str());
			}
			else {
				printf("goaldis: Successfully Disassembled - '%s'\n", go->fileName.c_str());
			}
		}
	} else if (!strcmp(mode, "-bin")) {
		for (MetaGoFile *go : metaGoFiles)
			dumpRawBin(outputDirectory, go);
	} else {
		fprintf(stderr, "goaldis: Invalid mode '%s'.\n", mode);
		return 1;
	}

	return 0;
}

