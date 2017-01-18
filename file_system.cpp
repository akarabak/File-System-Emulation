#include "file_system.h"
using namespace std;

extern const int blockLength = 64;
static unsigned int MASK[32];
static const int k = 7;

const bool testing = false;

struct directoryEntry { //8 entries per block
	char name[4];
	int fileDescriptorIndex = -1;
};

struct descriptor { //4 descriptors per block
	int length = -1;
	int blocks[3] = { -1, -1, -1 };
};

struct OFTentry {
	char block[blockLength];
	int position = -1; //negative position means free
	int index;
	int length;
};

const int OFTsize = 4;
static OFTentry OFT[OFTsize];

bool init(string filename) {
	ifstream in (filename);
	if (!in.fail()) {
		char block[blockLength];
		for (int i = 0; i < 64; i++) {
			in.read(block, blockLength);
			write_block(i, block);
		}
		in.close();
		descriptor descriptors[4];
		read_block(1, (char*)descriptors);

		OFT[0].index = 0;
		OFT[0].position = 0;
		OFT[0].length = descriptors[0].length;

		for (int i = 1; i < OFTsize; i++)
			OFT[i].position = -1;
		read_block(descriptors[0].blocks[0], OFT[0].block);
		
		return true;
	}
	else {
		for (int i = 0; i < 32; i++) {
			MASK[i] = 1 << (31 - i);
		}
		//empty bitmap except first k blocks are bitmap and 6 blocks of descriptors
		unsigned int BM[2]{ 0,0 };
		for (int i = 0; i < k; i++) {
			BM[0] = BM[0] | MASK[i];
		}

		write_block(0, (char*)BM);

		descriptor descriptors[4];
		for (int i = 2; i < k; i++) {
			write_block(i, (char*)descriptors);
		}

		OFT[0].index = 0;
		OFT[0].length = 0;
		OFT[0].position = 0;

		for (int i = 1; i < OFTsize; i++)
			OFT[i].position = -1;

		descriptors[0].length = 0;
		write_block(1, (char*)descriptors);
		return false;
	}
}

void save(const string filename) {
	for (int i = OFTsize - 1; i >= 0; i--) {
		close(i);
	}
	ofstream out;
	out.open(filename);
	char block[blockLength];
	for (int i = 0; i < 64; i++) {
		read_block(i, block);
		out.write(block, blockLength);
	}
	out.close();
}

int findFreeDescriptor() { //findFreeDescriptor and mark it as used
	descriptor descriptors[4];
	for (int i = 1; i < k; i++) {
		read_block(i, (char*)descriptors);
		for (int j = 0; j < 4; j++) {
			if (descriptors[j].length < 0) {
				descriptors[j].length = 0;
				write_block(i, (char*)descriptors);
				return (i-1) * 7 + j;
			}
		}
	}
	return -1; //no free descriptors
}

int findBlock() { //find free block and mark it used
	unsigned int BM[16];
	read_block(0, (char*)BM);
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 32; j++) {
			if ((BM[i] & MASK[j]) == 0) {
				BM[i] = BM[i] | MASK[j];
				write_block(0, (char*)BM);
				return (i * 32) + j;
			}
		}
	}
	return -1; //bitmap is full
}

void freeBlock(int num) {
	unsigned int BM[16];
	read_block(0, (char*)BM);
	int bitNum = num / 32;
	int bitPos = num % 32;

	BM[bitNum] = BM[bitNum] & ~MASK[bitPos];
	write_block(0, (char*)BM);
}

int open(const char *name) {
	//read directory
	directoryEntry entries[24];
	lseek(0, 0);
	int size = read(0, (char*)entries, 3 * blockLength);
	size /= sizeof(directoryEntry);
	for (int j = 0; j < size; j++) {
		if (strcmp(entries[j].name, name) == 0) { 
			descriptor d[4];
			int descriptorIndex = entries[j].fileDescriptorIndex % 4;
			read_block(entries[j].fileDescriptorIndex / 4 + 1, (char*)d);
			for (int i = 0; i < OFTsize; i++) {
				if (OFT[i].position < 0) {
					OFT[i].position = 0;
					OFT[i].index = entries[j].fileDescriptorIndex;
					OFT[i].length = d[descriptorIndex].length;
					if (OFT[i].length > 0)
						read_block(d[descriptorIndex].blocks[0], OFT[i].block);
					return i;
				}
				else if (OFT[i].index == entries[j].fileDescriptorIndex)
					return -1; //file already opened
			}
		}
	}

	return -1; //no free entry in OFT
}

bool close(int index) {
	if (OFT[index].position < 0)
		return false;
	descriptor descriptors[4];
	int descriptorNum = OFT[index].index % 4;
	read_block(OFT[index].index / 4 + 1, (char*)descriptors);
	int blockNum = OFT[index].position / blockLength;

	if (OFT[index].position % blockLength == 0)
		blockNum--;

	descriptors[descriptorNum].length = OFT[index].length;
	if (descriptors[descriptorNum].blocks[blockNum] < 0)
		descriptors[descriptorNum].blocks[blockNum] = findBlock();
	//write fileData from OFT to disk
	write_block(descriptors[descriptorNum].blocks[blockNum], OFT[index].block);
	//update descriptor info
	write_block(OFT[index].index / 4 + 1, (char*)descriptors);
	//mark OFT as free
	
	OFT[index].position = -1;

	return true;
}

bool isDuplicate(const char* name, directoryEntry entries[], int numEntries) {
	for (int i = 0; i < numEntries; i++) {
		if (strcmp(entries[i].name, name) == 0)
			return true;
	}
	return false;
}

int lseek(int index, int position) {
	if (OFT[index].position < 0 && OFT[index].length < position)
		return -1; //file closed or out of range
	int currentBlock = OFT[index].position / blockLength;
	if (OFT[index].position != 0 && OFT[index].position % blockLength == 0) {
		currentBlock--;
	}
	int posBlock = position / blockLength;

	if (currentBlock != posBlock) {
		descriptor descriptors[4];
		int descriptorNum = OFT[index].index % 4;
		read_block(OFT[index].index / 4 + 1, (char*)descriptors);
				
		if (descriptors[descriptorNum].blocks[currentBlock] < 0)
			descriptors[descriptorNum].blocks[currentBlock] = findBlock();
		descriptors[descriptorNum].length = OFT[index].length;
		write_block(OFT[index].index / 4 + 1, (char*)descriptors);
		write_block(descriptors[descriptorNum].blocks[currentBlock], OFT[index].block);
		read_block(descriptors[descriptorNum].blocks[position / blockLength], OFT[index].block);
	}
	OFT[index].position = position;
	return OFT[index].position;
}

int read(int index, char *mem, int count) {
	if (OFT[index].position < 0)
		return -1;
	int initPos = OFT[index].position;
	int position;
	for (position = OFT[index].position; position < OFT[index].length && position < count + initPos; position++) {
		if (position != 0 && position % 64 == 0) {
			descriptor d[4];
			int dNum = OFT[index].index % 4;
			read_block(OFT[index].index / 4 + 1, (char*)d);
			int blockNum = OFT[index].position / blockLength - 1;
			write_block(d[dNum].blocks[blockNum], OFT[index].block);
			read_block(d[dNum].blocks[blockNum + 1], OFT[index].block);
		}
		int i = position % 64;
		*mem = OFT[index].block[i];
		mem++;
		OFT[index].position++;
	}
	return position - initPos;
}

int write(int index, char *mem, int count) {
	if (OFT[index].position < 0)
		return -1;

	int initPos = OFT[index].position; 
	int position;
	for (position = OFT[index].position; OFT[index].position < count + initPos && OFT[index].position < 3 * blockLength; position++) { //position is position within 3 blocks
		if (position % 64 == 0 && position != 0) { //allocate block
			descriptor d[4];
			int dNum = OFT[index].index % 4;
			read_block(OFT[index].index / 4 + 1, (char*)d);
			int blockNum = OFT[index].position / blockLength - 1;
			if (d[dNum].length <= OFT[index].position)
				d[dNum].blocks[blockNum] = findBlock();

			write_block(d[dNum].blocks[blockNum], OFT[index].block);

			if (d[dNum].length < OFT[index].length)
				d[dNum].length = OFT[index].length;
			write_block(OFT[index].index / 4 + 1, (char*)d);

			if (d[dNum].length > OFT[index].position)
				read_block(d[dNum].blocks[blockNum + 1], OFT[index].block);
		}
		int i = position % 64; //i is position within block
		OFT[index].block[i] = *mem;
		OFT[index].position++;
		mem++;
		if (OFT[index].length < OFT[index].position)
			OFT[index].length = OFT[index].position;
	}
	return position - initPos;
}

bool create(const char* name) {
	directoryEntry entries[24];
	lseek(0, 0);
	int sizeRead = read(0, (char*)entries, 192);
	int numEntries = sizeRead / sizeof(directoryEntry);

	if (isDuplicate(name, entries, numEntries))
		return false;

	directoryEntry file;
	strcpy(file.name, name);
	file.fileDescriptorIndex = findFreeDescriptor();

	bool entryFound = false;
	for (int i = 0; i < numEntries; i++) {
		if (entries[i].fileDescriptorIndex < 0) {
			entries[i] = file;
			entryFound = true;
			break;
		}
	}
	if (!entryFound) {
		if (sizeRead == 184)
			return false;
		sizeRead += sizeof(directoryEntry);
		entries[numEntries] = file;
	}
	
	
	lseek(0, 0);
	write(0, (char*)entries, sizeRead);

	return true;
}

bool destroy(const char* name) {
	directoryEntry entries[24];
	int fileDescriptorIndex = -1;
	lseek(0, 0);
	int sizeRead = read(0, (char*)entries, 192);
	for (int i = 0; i < sizeRead / sizeof(directoryEntry); i++) {
		if (entries[i].fileDescriptorIndex > 0) {
			if (strcmp(entries[i].name, name) == 0) {
				fileDescriptorIndex = entries[i].fileDescriptorIndex;
				entries[i].fileDescriptorIndex = -1;
				strcpy(entries[i].name, "");
				lseek(0, 0);
				write(0, (char*)entries, sizeRead);
				goto outer;
			}
		}
	}
outer:;
	if (fileDescriptorIndex < 0)
		return false;

	for (int i = OFTsize - 1; i >= 0; i--) {
		if (OFT[i].index == fileDescriptorIndex)
			close(i);
	}

	descriptor descriptors[4];
	read_block(fileDescriptorIndex / 4 + 1, (char*)descriptors);

	for (int i = 0; i < 3; i++) {
		if (descriptors[fileDescriptorIndex % 4].blocks[i] >= 0)
			freeBlock(descriptors[fileDescriptorIndex % 4].blocks[i]);
	}

	descriptors[fileDescriptorIndex % 4].length = -1;
	descriptors[fileDescriptorIndex % 4].blocks[0] = -1;
	descriptors[fileDescriptorIndex % 4].blocks[1] = -1;
	descriptors[fileDescriptorIndex % 4].blocks[2] = -1;

	write_block(fileDescriptorIndex / 4 + 1, (char*)descriptors);
	return true;
}



void directory() {
	directoryEntry entries[24];
	lseek(0, 0);
	int size = read(0, (char*)entries, 192);
	for (int i = 0; i < size / sizeof(directoryEntry); i++) {
		if (entries[i].fileDescriptorIndex > 0) {
			cout << entries[i].name << " ";
		}
	}
	cout << endl;
}