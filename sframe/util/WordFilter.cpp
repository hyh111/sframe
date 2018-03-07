
#include <assert.h>
#include <sstream>
#include <memory.h>
#include <algorithm>
#include "WordFilter.h"

using namespace sframe;

TrieNode::~TrieNode()
{
	for (auto & pr : _children)
	{
		if (pr.second)
		{
			delete pr.second;
		}
	}
}

TrieNode * TrieNode::GetChild(uint8_t c, bool create_when_null)
{
	auto it = _children.find(c);
	if (it == _children.end())
	{
		if (create_when_null)
		{
			TrieNode * node = new TrieNode();
			if (!_children.insert(std::make_pair(c, node)).second)
			{
				assert(false);
				return nullptr;
			}
			return node;
		}

		return nullptr;
	}

	return it->second;
}



Trie::Trie(bool ignore_case)
{
	memset(_nodes, 0, sizeof(_nodes));
	_ignore_case = ignore_case;
	memset(_is_ignore, 0, sizeof(_is_ignore));
}

Trie::~Trie()
{
	for (int i = 0; i < kMaxCharCount; i++)
	{
		if (_nodes[i])
		{
			delete _nodes[i];
		}
	}
}

void Trie::AddIgnoreCharacters(const char * str)
{
	if (str == nullptr)
	{
		return;
	}

	while ((*str) != '\0')
	{
		uint8_t c = (uint8_t)(*str);
		if (_ignore_case && c >= (uint8_t)'A' && c <= (uint8_t)'Z')
		{
			c += (uint8_t)32;
		}

		_is_ignore[c] = true;
		str++;
	}
}

void Trie::AddWord(const char * str, size_t len)
{
	if (str == nullptr || len == 0)
	{
		return;
	}

	uint8_t c = (uint8_t)str[0];
	if (_ignore_case && c >= (uint8_t)'A' && c <= (uint8_t)'Z')
	{
		c += (uint8_t)32;
	}

	auto node = _nodes[c];
	if (!node)
	{
		node = new TrieNode();
		_nodes[c] = node;
	}

	for (size_t i = 1; i < len; i++)
	{
		c = (uint8_t)str[i];
		if (_ignore_case && c >= (uint8_t)'A' && c <= (uint8_t)'Z')
		{
			c += (uint8_t)32;
		}

		if (_is_ignore[c])
		{
			continue;
		}

		TrieNode * cur_node = node->GetChild(c, true);
		assert(cur_node);
		node = cur_node;
	}

	node->SetIsEndNode(true);
}

size_t Trie::FindWord(const char * str, size_t len)
{
	if (str == nullptr || len == 0)
	{
		return 0;
	}

	uint8_t c = (uint8_t)str[0];
	if (_ignore_case && c >= (uint8_t)'A' && c <= (uint8_t)'Z')
	{
		c += (uint8_t)32;
	}

	TrieNode * node = _nodes[c];
	if (node == nullptr)
	{
		return 0;
	}

	size_t ret = node->IsEndNode() ? 1 : 0;

	for (size_t i = 1; i < len; i++)
	{
		c = (uint8_t)str[i];
		if (_ignore_case && c >= (uint8_t)'A' && c <= (uint8_t)'Z')
		{
			c += (uint8_t)32;
		}

		if (_is_ignore[c])
		{
			continue;
		}

		TrieNode * cur_node = node->GetChild(c, false);
		if (cur_node == nullptr)
		{
			break;
		}

		if (cur_node->IsEndNode())
		{
			ret = i + 1;
		}

		node = cur_node;
	}

	return ret;
}




bool WordFilter::HaveBadWord(const std::string & text)
{
	if (text.empty())
	{
		return false;
	}

	const char * str = text.c_str();
	size_t len = text.length();

	for (size_t i = 0; i < len; i++)
	{
		if (_trie.FindWord(str + i, len - i) > 0)
		{
			return true;
		}
	}

	return false;
}

std::string WordFilter::ReplaceBadWord(const std::string & text, char replace_char, size_t replace_char_count)
{
	if (text.empty())
	{
		return "";
	}

	std::ostringstream oss;
	const char * str = text.c_str();
	size_t len = text.length();
	char replace_str[33] = { 0 };
	if (replace_char_count > 0)
	{
		memset(replace_str, (int)replace_char, std::min(replace_char_count, (size_t)32));
	}

	while (len > 0)
	{
		size_t bad_word_len = _trie.FindWord(str, len);
		assert(bad_word_len <= len);
		if (bad_word_len == 0)
		{
			oss << *str;
			str++;
			len--;
		}
		else
		{
			if (replace_char_count > 0)
			{
				oss << replace_str;
			}
			else
			{
				for (size_t i = 0; i < bad_word_len; i++)
				{
					oss << replace_char;
				}
			}
			str += bad_word_len;
			len -= bad_word_len;
		}
	}

	return oss.str();
}
