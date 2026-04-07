// Last edited by Jared W

#ifndef COMPRESS_H
#define COMPRESS_H

#include <string>
#include <map>

// A node in our Huffman tree
struct HuffNode {
    char ch;           // The character (only meaningful in leaf nodes)
    int freq;          // Frequency of this character (or sum of children)
    HuffNode* left;
    HuffNode* right;

    HuffNode(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
    HuffNode(int f, HuffNode* l, HuffNode* r) : ch('\0'), freq(f), left(l), right(r) {}
};

// Comparator for the priority queue (min-heap: lowest frequency = highest priority)
struct Compare {
    bool operator()(HuffNode* a, HuffNode* b) {
        return a->freq > b->freq;  // > makes it a min-heap
    }
};

#endif