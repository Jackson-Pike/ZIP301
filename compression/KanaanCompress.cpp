// Huffman.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

using namespace std;

struct Node {
	unsigned char ch;
	uint64_t freq; //unsigned 64-bit frequency count to handle large files
	Node* left;
	Node* right;
	Node(unsigned char c, uint64_t f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
	Node(uint64_t f, Node* l, Node* r) : ch(0), freq(f), left(l), right(r) {}
	bool isLeaf() const { return left == nullptr && right == nullptr; }
};

struct Cmp {
	bool operator()(const Node* a, const Node* b) const {
		return a->freq > b->freq; // min-heap
	}
};

void buildCodes(Node* node, const string& prefix, unordered_map<unsigned char, string>& codes) {
	if (!node) return;
	if (node->isLeaf()) {
		// If tree has only one symbol, give it code "0"
		codes[node->ch] = prefix.empty() ? string("0") : prefix;
		return;
	}
	if (node->left) buildCodes(node->left, prefix + "0", codes);
	if (node->right) buildCodes(node->right, prefix + "1", codes);
}

void freeTree(Node* node) {
	if (!node) return;
	freeTree(node->left);
	freeTree(node->right);
	delete node;
}

// Helper: return the character label used in the .zip301 header
// Special characters use keyword names; printable non-space chars are bare.
static string charLabel(int v) {
	unsigned char uc = static_cast<unsigned char>(v);
	if (uc == '\n') return string("newline");
	if (uc == '\r') return string("return");
	if (uc == '\t') return string("tab");
	if (uc == ' ') return string("space");
	if (isprint(uc)) {
		return string(1, static_cast<char>(uc));
	}
	std::ostringstream oss;
	oss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)uc;
	return oss.str();
}

// Entry point
int main(int argc, char** argv) {
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " <inputfile>" << endl;
		return 1;
	}

	string inpath = argv[1];
	ifstream ifs(inpath, ios::binary);
	if (!ifs) {
		cerr << "ERROR reading file: " << inpath << endl;
		return 1;
	}

	// Read entire input into memory in one syscall
	ifs.seekg(0, ios::end);
	size_t fileSize = static_cast<size_t>(ifs.tellg());
	ifs.seekg(0, ios::beg);
	vector<unsigned char> inputBuf(fileSize);
	ifs.read(reinterpret_cast<char*>(inputBuf.data()), fileSize);
	ifs.close();

    // Count frequencies of each byte value in the input file
	vector<uint64_t> freq(256, 0);
	for (unsigned char uc : inputBuf) freq[uc]++;

    // Build a min-heap (priority queue) of leaf nodes for each byte that appears
	priority_queue<Node*, vector<Node*>, Cmp> pq;
	for (int i = 0; i < 256; ++i) {
		if (freq[i] > 0) pq.push(new Node(static_cast<unsigned char>(i), freq[i]));
	}

	Node* root = nullptr;
	if (pq.empty()) {
		// Empty input file: produce an empty archive with no codes
		root = nullptr;
	} else if (pq.size() == 1) {
		// Single unique symbol: create a root with that single child
		Node* only = pq.top(); pq.pop();
		root = new Node(only->freq, only, nullptr);
	} else {
		while (pq.size() > 1) {
			Node* a = pq.top(); pq.pop();
			Node* b = pq.top(); pq.pop();
			Node* parent = new Node(a->freq + b->freq, a, b);
			pq.push(parent);
		}
		root = pq.top(); pq.pop();
	}

    // Generate Huffman codes by traversing the tree (left->'0', right->'1')
	unordered_map<unsigned char, string> codes;
	if (root) buildCodes(root, string(), codes);

	// Pre-pack codes into direct-indexed arrays for O(1) lookup during encoding
	uint32_t codeWord[256] = {};  // bits of each code, MSB-aligned in the uint32_t
	int      codeLen[256]  = {};  // number of valid bits

	uint64_t totalBits = 0;
	for (int i = 0; i < 256; ++i) {
		if (freq[i] == 0) continue;
		const string& s = codes[static_cast<unsigned char>(i)];
		int len = static_cast<int>(s.size());
		codeLen[i] = len;
		uint32_t word = 0;
		for (int k = 0; k < len; ++k)
			if (s[k] == '1') word |= (1u << (31 - k));
		codeWord[i] = word;
		totalBits += freq[i] * static_cast<uint64_t>(len);
	}

	// Build output filename: replace last extension with .zip301
	string outpath;
	size_t pos = inpath.find_last_of('.');
	if (pos == string::npos) outpath = inpath + ".zip301";
	else outpath = inpath.substr(0, pos) + ".zip301";

	ofstream ofs(outpath, ios::binary);
	if (!ofs) {
		cerr << "ERROR creating output file: " << outpath << endl;
		freeTree(root);
		return 1;
	}

    // Write the code table as text lines: <numeric-byte> <readable-char> <code>\n
	// Sorting by numeric byte value keeps the header deterministic
	vector<pair<int, string>> codeLines;
	for (auto &p : codes) codeLines.emplace_back(static_cast<int>(p.first), p.second);
	sort(codeLines.begin(), codeLines.end(), [](auto &a, auto &b){ return a.first < b.first; });
    for (auto &pr : codeLines) {
		ofs << pr.second << ' ' << charLabel(pr.first) << '\n';
	}

	// Separator line of five asterisks
	ofs << "*****\n";

	// Write total number of bits before the binary data
	ofs << totalBits << '\n';

	// Stream-encode inputBuf directly into packed bytes via a 64-bit accumulator
	const size_t OUT_FLUSH = 65536;
	vector<unsigned char> outBuf;
	outBuf.reserve(OUT_FLUSH + 8);

	uint64_t accumulator = 0;  // valid bits sit at the MSB side
	int      bitsInAcc   = 0;

	for (unsigned char uc : inputBuf) {
		accumulator |= (static_cast<uint64_t>(codeWord[uc]) << (32 - bitsInAcc));
		bitsInAcc   += codeLen[uc];

		while (bitsInAcc >= 8) {
			outBuf.push_back(static_cast<unsigned char>(accumulator >> 56));
			accumulator <<= 8;
			bitsInAcc   -= 8;
			if (outBuf.size() >= OUT_FLUSH) {
				ofs.write(reinterpret_cast<const char*>(outBuf.data()), outBuf.size());
				outBuf.clear();
			}
		}
	}

	// Flush partial last byte (zero-padded in low bits)
	if (bitsInAcc > 0)
		outBuf.push_back(static_cast<unsigned char>(accumulator >> 56));
	if (!outBuf.empty())
		ofs.write(reinterpret_cast<const char*>(outBuf.data()), outBuf.size());

	ofs.close();
	//freeTree(root);

	cout << "Wrote encoded output to: " << outpath << endl;
	return 0;
}

