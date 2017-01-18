#include "shell.h"
using namespace std;

void error() {
	cout << "error" << endl;
}

void shell() {
	string input, cmd;
	vector<string> tokens;
	while (getline(cin, input)) {
		tokens = tokenize(input);
		if (tokens.size() > 0) {
			cmd = toLower(tokens[0]);
		}
		if (cmd == "cr") {
			if (tokens.size() == 2) {
				if (tokens[1].length() <= 3) {
					if (!create(tokens[1].c_str()))
						error();
					else
						cout << tokens[1] << " created" << endl;
				}
				else {
					error();
				}
			}
		}

		else if (cmd == "dr") {
			directory();
		}

		else if (cmd == "op") {
			if (tokens.size() == 2) {
				if (tokens[1].length() <= 3) {
					int index = open(tokens[1].c_str());
					if (index > 0)
						cout << tokens[1] << " opened " << index << endl;
					else
						error();
				}
				else {
					error();
				}
			}
		}
		
		else if (cmd == "rd") {
			if (tokens.size() == 3) {
				try {
					int index = stoi(tokens[1]);
					int count = stoi(tokens[2]);
					char *p = new char[count];
					int size = read(index, p, count);
					if (size < 0)
						error();
					else {
						for (int i = 0; i < size; i++) {
							cout << p[i];
						}
						delete p;
						cout << endl;
					}
				}
				catch (invalid_argument) {
					error();
				}
			}
			else {
				error();
			}
		}

		else if (cmd == "wr") {
			if (tokens.size() == 4 && tokens[2].length() == 1) {
				try {
					int index = stoi(tokens[1]);
					int count = stoi(tokens[3]);
					char *p = new char[count];
					for (int i = 0; i < count; i++) {
						p[i] = tokens[2][0];
					}
					int written = write(index, p, count);
					if (written < 0)
						error();
					else
						cout << written << " bytes written" << endl;
					delete p;
					
				}
				catch (invalid_argument) {
					error();
				}
			}
			else
				error();
		}

		else if (cmd == "sk") {
			if (tokens.size() == 3) {
				try {
					int index = stoi(tokens[1]);
					int position = stoi(tokens[2]);
					int num = lseek(index, position);
					if (num >= 0)
						cout << "position is " << num << endl;
				}
				catch (invalid_argument) {
					error();
				}
			}
			else
				error();
		}

		else if (cmd == "cl") {
			if (tokens.size() == 2) {
				try {
					int index = stoi(tokens[1]);
					if (close(index))
						cout << index << " closed" << endl;
					else
						error();
				}
				catch (invalid_argument) {
					error();
				}
			}
			else
				error();
		}

		else if (cmd == "in") {
			string name = "";
			if (tokens.size() == 2)
				name = tokens[1];
			if (init(name))
				cout << "disk restored" << endl;
			else
				cout << "disk initialized" << endl;
		}

		else if (cmd == "sv") {
			if (tokens.size() == 2) {
				save(tokens[1]);
				cout << "disk saved" << endl;
			}
			else
				error();
		}

		else if (cmd == "de") {
			if (tokens.size() == 2) {
				if (destroy(tokens[1].c_str())) {
					cout << tokens[1] << " destroyed" << endl;
				}
				else
					error();
			}
			else
				error();
		}
		else if (cmd == "")
			cout << endl;

		else if (cmd == "quit")
			exit(0);



		cmd = "";
	}
}

