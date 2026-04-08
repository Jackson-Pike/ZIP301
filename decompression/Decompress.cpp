//
// Created by Jackson Pike on 4/2/26.
//
// Optimized using a Huffman trie for O(1)-per-bit decoding.
//
// References:
//   - Cormen, T.H., Leiserson, C.E., Rivest, R.L., and Stein, C. (2009).
//     Introduction to Algorithms, 3rd ed. MIT Press. Ch. 16.3, "Huffman codes."
//     (Huffman prefix-code trie structure and properties)
//   - cppreference.com: std::basic_istream::get
//     https://en.cppreference.com/w/cpp/io/basic_istream/get
//     (byte-at-a-time binary reads)
//   - cppreference.com: std::basic_ostream::write
//     https://en.cppreference.com/w/cpp/io/basic_ostream/write
//     (bulk buffered output)
//   - cppreference.com: std::ios::binary
//     https://en.cppreference.com/w/cpp/io/ios_base/openmode
//     (binary mode to prevent platform newline translation)

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace std;

// ---------------------------------------------------------------------------
// Huffman trie
// ---------------------------------------------------------------------------

struct TrieNode {
    TrieNode* children[2];  // children[0] = bit 0, children[1] = bit 1
    char decodedChar;       // '\0' means internal node (not a leaf)

    TrieNode() : decodedChar('\0') {
        children[0] = children[1] = nullptr;
    }
};

void deleteTrie(TrieNode* node) {
    if (!node) return;
    deleteTrie(node->children[0]);
    deleteTrie(node->children[1]);
    delete node;
}

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

fstream ifs;
fstream ofs;
TrieNode* trieRoot = nullptr;
int bitCount;

// ---------------------------------------------------------------------------
// openFile
// ---------------------------------------------------------------------------

void openFile(int argc, char** argv) {
    if (argc < 2) {
        cout << "Ooops! You forgot to provide a filename on the command line." << endl;
        exit(1);
    }

    ifs.open(argv[1], ios::in | ios::binary);

    // Build output filename: decompressed/<stem>.txt
    fs::create_directories("decompressed");
    string filename = argv[1];
    size_t slash = filename.find_last_of("/\\");
    string basename = (slash == string::npos) ? filename : filename.substr(slash + 1);
    size_t dot = basename.find_last_of('.');
    string stem = (dot == string::npos) ? basename : basename.substr(0, dot);
    string outputFile = "decompressed/" + stem + ".txt";
    ofs.open(outputFile, ios::out | ios::binary);

    if (!ifs) {
        cout << "ERROR occurred when reading file: " << argv[1] << endl;
        exit(1);
    }
}

// ---------------------------------------------------------------------------
// readHeader — parse Huffman codes and build the trie
// ---------------------------------------------------------------------------

void readHeader() {
    trieRoot = new TrieNode();

    while (true) {
        string line;
        getline(ifs, line);

        if (line.find("*****") != string::npos) {
            break;
        }

        // Split on first space: "<bits> <mapping>"
        size_t space = line.find(' ');
        string encoding = line.substr(0, space);
        string mapping  = line.substr(space + 1);

        // Resolve surrogate encodings to their actual characters once at build time
        char decoded;
        if      (mapping == "space")   decoded = ' ';
        else if (mapping == "newline") decoded = '\n';
        else if (mapping == "return")  decoded = '\r';
        else if (mapping == "tab")     decoded = '\t';
        else                           decoded = mapping[0];

        // Insert into trie
        TrieNode* node = trieRoot;
        for (char bit : encoding) {
            int idx = bit - '0';  // '0' -> 0, '1' -> 1
            if (!node->children[idx]) {
                node->children[idx] = new TrieNode();
            }
            node = node->children[idx];
        }
        node->decodedChar = decoded;
    }
}

// ---------------------------------------------------------------------------
// readBinaryAndWriteOutput — stream decode via trie, buffered output
// ---------------------------------------------------------------------------

void readBinaryAndWriteOutput() {
    const size_t FLUSH_SIZE = 65536;
    string outBuf;
    outBuf.reserve(FLUSH_SIZE + 4);

    TrieNode* node = trieRoot;
    int bitsRead = 0;
    char byte;

    while (ifs.get(byte)) {
        for (int i = 7; i >= 0; i--) {
            if (bitsRead == bitCount) goto done;

            node = node->children[(byte >> i) & 1];
            bitsRead++;

            if (node->decodedChar != '\0') {
                outBuf += node->decodedChar;
                if (outBuf.size() >= FLUSH_SIZE) {
                    ofs.write(outBuf.data(), outBuf.size());
                    outBuf.clear();
                }
                node = trieRoot;
            }
        }
    }

done:
    if (!outBuf.empty()) {
        ofs.write(outBuf.data(), outBuf.size());
    }
    ifs.close();
    ofs.close();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    openFile(argc, argv);
    readHeader();

    string rawCount;
    getline(ifs, rawCount);
    bitCount = stoi(rawCount);

    readBinaryAndWriteOutput();

    //deleteTrie(trieRoot);
    return 0;
}
