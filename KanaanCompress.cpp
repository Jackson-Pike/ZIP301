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

// Helper: return a readable description for a byte value
// Examples:
//   10 -> "'\\n'"  (newline)
//   32 -> "' '"     (space)
//   65 -> "'A'"     (printable ASCII)
//   27 -> "0x1B"    (non-printable shown as hex)
static string describeByte(int v) {
	unsigned char uc = static_cast<unsigned char>(v);
	if (uc == '\n') return string("'\\n'");
	if (uc == '\r') return string("'\\r'");
	if (uc == '\t') return string("'\\t'");
	if (uc == ' ') return string("' '");
	if (isprint(uc)) {
		string s;
		s.push_back('\'' );
		s.push_back(static_cast<char>(uc));
		s.push_back('\'' );
		return s;
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

    // Count frequencies of each byte value in the input file
	vector<uint64_t> freq(256, 0);
	char c;
	while (ifs.get(c)) {
		unsigned char uc = static_cast<unsigned char>(c);
		freq[uc]++;
	}

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

	// Re-open input to encode (seek back to beginning)
	ifs.clear();
	ifs.seekg(0, ios::beg);

	string bitstr;
	// Reserve some space if possible
	{
		uint64_t totalBytes = 0;
		for (int i = 0; i < 256; ++i) totalBytes += freq[i];
		bitstr.reserve(static_cast<size_t>(min<uint64_t>(totalBytes * 2, 1ULL << 26)));
	}

    // Convert the input file bytes to a long string of '0'/'1' using the codes
	if (root) {
		while (ifs.get(c)) {
			unsigned char uc = static_cast<unsigned char>(c);
			auto it = codes.find(uc);
			if (it != codes.end()) bitstr += it->second; // append the prefix code
		}
	}

	uint64_t totalBits = bitstr.size();

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
		// write: numeric value, printable/escaped representation, then the prefix code
		ofs << pr.first << ' ' << describeByte(pr.first) << ' ' << pr.second << '\n';
	}

	// Separator line of five asterisks
	ofs << "*****\n";

	// Write binary string as text 0/1 for debugging/verification
	ofs << bitstr << '\n';

	// Write total number of bits
	ofs << totalBits << '\n';

	ofs.close();
	freeTree(root);

	cout << "Wrote encoded output to: " << outpath << endl;
	return 0;
}

