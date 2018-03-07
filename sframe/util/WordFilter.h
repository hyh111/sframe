
#ifndef SFRAME_WORD_FILTER_H
#define SFRAME_WORD_FILTER_H

#include <inttypes.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace sframe {

// 单词查找树节点
class TrieNode
{
public:

	TrieNode() : _is_end_node(false) {}

	~TrieNode();

	bool IsEndNode() const
	{
		return _is_end_node;
	}

	void SetIsEndNode(bool b)
	{
		_is_end_node = b;
	}

	TrieNode * GetChild(uint8_t c, bool create_when_null);

private:
	std::unordered_map<uint8_t, TrieNode*> _children;
	bool _is_end_node;
};

// 单词查找树
class Trie
{
public:

	static const int kMaxCharCount = 256;

	Trie(bool ignore_case);

	~Trie();

	void AddIgnoreCharacters(const char * str);

	void AddWord(const char * str, size_t len);

	size_t FindWord(const char * str, size_t len);

private:
	TrieNode * _nodes[kMaxCharCount];
	bool _ignore_case;
	bool _is_ignore[kMaxCharCount];
};


// 敏感词过滤
class WordFilter
{
public:

	WordFilter(bool ignore_case = true) : _trie(ignore_case) {}

	~WordFilter() {}

	void AddIgnoreCharacters(const char * str)
	{
		_trie.AddIgnoreCharacters(str);
	}

	void AddWord(const std::string & word)
	{
		_trie.AddWord(word.c_str(), word.length());
	}

	bool HaveBadWord(const std::string & text);

	std::string ReplaceBadWord(const std::string & text, char replace_char = '*', size_t replace_char_count = 0);

private:
	Trie _trie;

};

}

#endif