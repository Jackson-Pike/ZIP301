//
// Created by Jackson Pike on 4/2/26.
//

#include "Decompress.h"
#include <string>
#include <iostream>
#include <fstream>
#include <map>

using namespace std;
fstream ifs;
map<string, string> huffmanMap;
int bitCount;
int longestLen = 0;
fstream ofs;

void openFile(int argc, char** argv) {
    /* =====================FILE OPENING========================== */

    if (argc < 2) {
        cout << "Ooops! You forgot to provide a filename on the command line." << endl;
        exit(1);
    }

    // Proceed to open ifs with commandline argument [1] (the filename)
    ifs.open(argv[1], ios::in | ios::binary);


    string filename = argv[1];
    size_t period = filename.find('.');
    string outputFile = filename.substr(0, period);
    outputFile += "2.txt";
    ofs.open(outputFile, ios::out);

    // If there was an issue or the file did not exist, print error and exit.
    if (!ifs) {
        cout << "ERROR occurred when reading file: " << argv[1] << endl;
        exit(1);
    }
}

void readHeader() {
    while (true) {
        string line;
        getline(ifs, line);

        if (line.find("*****") != string::npos) {
            break;
        }

        // Parse the line: split on space
        size_t space = line.find(' ');
        string encoding = line.substr(0, space);
        if (encoding.size() > longestLen) {
            longestLen = encoding.size();
        }
        string mapping = line.substr(space + 1);
        huffmanMap[encoding] = mapping;
    }

    for (auto huffman_map : huffmanMap) {
        cout << huffman_map.first << " -> " << huffman_map.second << endl;
    }


}

void readBinaryAndWriteOutput() {

    string bitString = "";
    char byte;
    while (ifs.get(byte)) {
        // Convert byte to 8 bits
        for (int i = 7; i >= 0; i--) {
            bitString += ((byte >> i) & 1) ? '1' : '0';
        }
    }

    // Now trim to the exact bit count
    bitString = bitString.substr(0, bitCount);

    string bitBuffer = "";
    for (char bit : bitString) {
        bitBuffer += bit;

        if (huffmanMap.find(bitBuffer) != huffmanMap.end()) {
            string decodedChar = huffmanMap[bitBuffer];
            if (decodedChar == "space") {
                ofs << ' ';
            } else if (decodedChar == "newline") {
                ofs << endl;
            } else {
                ofs << decodedChar;
            }

            bitBuffer = "";
        }
    }
    ifs.close();
    ofs.close();
}

int main(int argc, char** argv) {

    openFile(argc, argv);
    readHeader();

    string rawCount;
    getline(ifs, rawCount);
    bitCount = stoi(rawCount);

    readBinaryAndWriteOutput();

}
