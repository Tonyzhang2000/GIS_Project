#include <string>
#include <cstring>
#include <algorithm>

#include "m1.h"
#include "trie_tree.h"

TrieNode::TrieNode(){
    this->isEnd = false;
}
TrieNode::~TrieNode(){
    for(auto i : next){
        delete i.second;
    }
    next.clear();
}
void TrieNode::tt_insert(std::string key,StreetIdx idd,bool np ){
    // std::cout << "key: "<< key << std::endl;
    if (np){
        transform(key.begin(), key.end(), key.begin(), ::tolower);
        for (auto ch = key.begin(); ch != key.end() ;ch++){
            if (*ch == ' ')
               key.erase(ch);
            if (ch == key.end())
                break;
        }
    }
    char ch = key[0];
    if (key.length()>0){
        if (next.find(key[0]) == next.end()){
            next[ch] = new TrieNode();
        }
        key.erase(key.begin());
        ids.push_back(idd);
        next[ch]->tt_insert(key,idd,false);
    } else {
        isEnd = true; 
        ids.push_back(idd);
        }
}

std::vector<StreetIdx> TrieNode::tt_search(std::string key, bool np){
    std::vector<StreetIdx> ret;
    ret.clear(); 
    if (!key.length()) return ret; 
    if (np){
        transform(key.begin(), key.end(), key.begin(), ::tolower);
        for (auto ch = key.begin(); ch != key.end() ;ch++){
            if (*ch == ' ')
               key.erase(ch);
            if (ch == key.end())
                break;
        }
    }
    char ch = key[0];
    if (key.length()>1){
        if (next.find(ch) == next.end()){
            return ret;
        }else{
            key.erase(key.begin());
            return next[ch]->tt_search(key,false);
        }
    } else {
        if (next.find(ch) == next.end()){
            return ret;
        }else{
            return next[ch]->ids;
        }
    }
    std::cout << "key: "<< key << std::endl;
    return ret; 

}
