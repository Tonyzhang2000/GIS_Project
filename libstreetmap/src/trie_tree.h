#pragma once 

#include <unordered_map>
#include "m1.h"

#define MAX_CHILD 26

class TrieNode{
    public:
        TrieNode();
        ~TrieNode();
        void tt_insert(std::string key,StreetIdx idd,bool np);
        std::vector<StreetIdx> tt_search(std::string key, bool np);
    private:
        bool isEnd;
        std::vector<StreetIdx> ids;
        std::unordered_map<char,TrieNode*> next;
};
