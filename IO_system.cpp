const int blockLength = 64; //16 integers
static char ldisk[64][blockLength];

void read_block(int i, char *p) {
	for (int b = 0; b < blockLength; b++) {
		*p = ldisk[i][b];
		p++;
	}
}

void write_block(int i, char *p) {
	for (int b = 0; b < blockLength; b++) {
		ldisk[i][b] = *p;
		p++;
	}
}