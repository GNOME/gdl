// SciTE - Scintilla based Text Editor
/** @file PropSet.cxx
 ** A Java style properties file module.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// Maintain a dictionary of properties

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "Platform.h"

#include "PropSet.h"

// The comparison and case changing functions here assume ASCII
// or extended ASCII such as the normal Windows code page.

inline char MakeUpperCase(char ch) {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return static_cast<char>(ch - 'a' + 'A');
}

int CompareCaseInsensitive(const char *a, const char *b) {
	while (*a && *b) {
		if (*a != *b) {
			char upperA = MakeUpperCase(*a);
			char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
	}
	// Either *a or *b is nul
	return *a - *b;
}

int CompareNCaseInsensitive(const char *a, const char *b, int len) {
	while (*a && *b && len) {
		if (*a != *b) {
			char upperA = MakeUpperCase(*a);
			char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
		len--;
	}
	if (len == 0)
		return 0;
	else
		// Either *a or *b is nul
		return *a - *b;
}

bool EqualCaseInsensitive(const char *a, const char *b) {
	return 0 == CompareCaseInsensitive(a, b);
}

inline unsigned int HashString(const char *s) {
	unsigned int ret = 0;
	while (*s) {
		ret <<= 4;
		ret ^= *s;
		s++;
	}
	return ret;
}

PropSet::PropSet() {
	superPS = 0;
	for (int root = 0; root < hashRoots; root++)
		props[root] = 0;
}

PropSet::~PropSet() {
	superPS = 0;
	Clear();
}

void PropSet::Set(const char *key, const char *val) {
	unsigned int hash = HashString(key);
	for (Property *p = props[hash % hashRoots]; p; p = p->next) {
		if ((hash == p->hash) && (0 == strcmp(p->key, key))) {
			// Replace current value
			delete [](p->val);
			p->val = StringDup(val);
			return ;
		}
	}
	// Not found
	Property *pNew = new Property;
	if (pNew) {
		pNew->hash = HashString(key);
		pNew->key = StringDup(key);
		pNew->val = StringDup(val);
		pNew->next = props[hash % hashRoots];
		props[hash % hashRoots] = pNew;
	}
}

void PropSet::Set(char *keyval) {
	while (isspace(*keyval))
		keyval++;
	char *eqat = strchr(keyval, '=');
	if (eqat) {
		*eqat = '\0';
		Set(keyval, eqat + 1);
		*eqat = '=';
	} else {	// No '=' so assume '=1'
		Set(keyval, "1");
	}
}

SString PropSet::Get(const char *key) {
	unsigned int hash = HashString(key);
	for (Property *p = props[hash % hashRoots]; p; p = p->next) {
		if ((hash == p->hash) && (0 == strcmp(p->key, key))) {
			return p->val;
		}
	}
	if (superPS) {
		// Failed here, so try in base property set
		return superPS->Get(key);
	} else {
		return "";
	}
}

static bool IncludesVar(const char *value, const char *key) {
	const char *var = strstr(value, "$(");
	while (var) {
		if (isprefix(var+2, key) && (var[2 + strlen(key)] == ')')) {
			// Found $(key) which would lead to an infinite loop so exit
			return true;
		}
		var = strstr(var + 2, ")");
		if (var)
			var = strstr(var + 1, "$(");
	}
	return false;
}

SString PropSet::GetExpanded(const char *key) {
	SString val = Get(key);
	if (IncludesVar(val.c_str(), key)) 
		return val;
	else
		return Expand(val.c_str());
}

SString PropSet::Expand(const char *withvars) {
	char *base = StringDup(withvars);
	char *cpvar = strstr(base, "$(");
	while (cpvar) {
		char *cpendvar = strchr(cpvar, ')');
		if (cpendvar) {
			int lenvar = cpendvar - cpvar - 2;  	// Subtract the $()
			char *var = StringDup(cpvar + 2, lenvar);
			SString val = GetExpanded(var);
			int newlenbase = strlen(base) + val.length() - lenvar;
			char *newbase = new char[newlenbase];
			strncpy(newbase, base, cpvar - base);
			strcpy(newbase + (cpvar - base), val.c_str());
			strcpy(newbase + (cpvar - base) + val.length(), cpendvar + 1);
			delete []var;
			delete []base;
			base = newbase;
		}
		cpvar = strstr(base, "$(");
	}
	SString sret = base;
	delete []base;
	return sret;
}

int PropSet::GetInt(const char *key, int defaultValue) {
	SString val = GetExpanded(key);
	if (val.length())
		return val.value();
	else
		return defaultValue;
}

bool isprefix(const char *target, const char *prefix) {
	while (*target && *prefix) {
		if (*target != *prefix)
			return false;
		target++;
		prefix++;
	}
	if (*prefix)
		return false;
	else
		return true;
}

static bool IsSuffixCaseInsensitive(const char *target, const char *suffix) {
	int lentarget = strlen(target);
	int lensuffix = strlen(suffix);
	if (lensuffix > lentarget)
		return false;
	for (int i = lensuffix - 1; i >= 0; i--) {
		if (MakeUpperCase(target[i + lentarget - lensuffix]) != 
			MakeUpperCase(suffix[i]))
			return false;
	}
	return true;
}

SString PropSet::GetWild(const char *keybase, const char *filename) {
	for (int root = 0; root < hashRoots; root++) {
		for (Property *p = props[root]; p; p = p->next) {
			if (isprefix(p->key, keybase)) {
				char * orgkeyfile = p->key + strlen(keybase);
				char *keyfile = NULL;

				if (strstr(orgkeyfile, "$(") == orgkeyfile) {
					char *cpendvar = strchr(orgkeyfile, ')');
					if (cpendvar) {
						*cpendvar = '\0';
						SString s = GetExpanded(orgkeyfile + 2);
						*cpendvar = ')';
						keyfile = StringDup(s.c_str());
					}
				}
				char *keyptr = keyfile;

				if (keyfile == NULL)
					keyfile = orgkeyfile;

				for (; ; ) {
					char *del = strchr(keyfile, ';');
					if (del == NULL)
						del = keyfile + strlen(keyfile);
					char delchr = *del;
					*del = '\0';
					if (*keyfile == '*') {
						if (IsSuffixCaseInsensitive(filename, keyfile + 1)) {
							*del = delchr;
							delete []keyptr;
							return p->val;
						}
					} else if (0 == strcmp(keyfile, filename)) {
						*del = delchr;
						delete []keyptr;
						return p->val;
					}
					if (delchr == '\0')
						break;
					*del = delchr;
					keyfile = del + 1;
				}
				delete []keyptr;

				if (0 == strcmp(p->key, keybase)) {
					return p->val;
				}
			}
		}
	}
	if (superPS) {
		// Failed here, so try in base property set
		return superPS->GetWild(keybase, filename);
	} else {
		return "";
	}
}

// GetNewExpand does not use Expand as it has to use GetWild with the filename for each 
// variable reference found.
SString PropSet::GetNewExpand(const char *keybase, const char *filename) {
	char *base = StringDup(GetWild(keybase, filename).c_str());
	char *cpvar = strstr(base, "$(");
	while (cpvar) {
		char *cpendvar = strchr(cpvar, ')');
		if (cpendvar) {
			int lenvar = cpendvar - cpvar - 2;  	// Subtract the $()
			char *var = StringDup(cpvar + 2, lenvar);
			SString val = GetWild(var, filename);
			int newlenbase = strlen(base) + val.length() - lenvar;
			char *newbase = new char[newlenbase];
			strncpy(newbase, base, cpvar - base);
			strcpy(newbase + (cpvar - base), val.c_str());
			strcpy(newbase + (cpvar - base) + val.length(), cpendvar + 1);
			delete []var;
			delete []base;
			base = newbase;
		}
		cpvar = strstr(base, "$(");
	}
	SString sret = base;
	delete []base;
	return sret;
}

void PropSet::Clear() {
	for (int root = 0; root < hashRoots; root++) {
		Property *p = props[root];
		while (p) {
			Property *pNext = p->next;
			p->hash = 0;
			delete p->key;
			p->key = 0;
			delete p->val;
			p->val = 0;
			delete p;
			p = pNext;
		}
		props[root] = 0;
	}
}

static bool iswordsep(char ch, bool onlyLineEnds) {
	if (!isspace(ch))
		return false;
	if (!onlyLineEnds)
		return true;
	return ch == '\r' || ch == '\n';
}

// Creates an array that points into each word in the string and puts \0 terminators
// after each word.
static char **ArrayFromWordList(char *wordlist, int *len, bool onlyLineEnds = false) {
	char prev = '\n';
	int words = 0;
	for (int j = 0; wordlist[j]; j++) {
		if (!iswordsep(wordlist[j], onlyLineEnds) && iswordsep(prev, onlyLineEnds))
			words++;
		prev = wordlist[j];
	}
	char **keywords = new char * [words + 1];
	if (keywords) {
		words = 0;
		prev = '\0';
		int slen = strlen(wordlist);
		for (int k = 0; k < slen; k++) {
			if (!iswordsep(wordlist[k], onlyLineEnds)) {
				if (!prev) {
					keywords[words] = &wordlist[k];
					words++;
				}
			} else {
				wordlist[k] = '\0';
			}
			prev = wordlist[k];
		}
		keywords[words] = &wordlist[slen];
		*len = words;
	} else {
		*len = 0;
	}
	return keywords;
}

void WordList::Clear() {
	if (words) {
		delete []list;
		delete []words;
		delete []wordsNoCase;
	}
	words = 0;
	wordsNoCase = 0;
	list = 0;
	len = 0;
	sorted = false;
}

void WordList::Set(const char *s) {
	list = StringDup(s);
	sorted = false;
	words = ArrayFromWordList(list, &len, onlyLineEnds);
	wordsNoCase = new char * [len + 1];
	memcpy(wordsNoCase, words, (len + 1) * sizeof (*words));
}

char *WordList::Allocate(int size) {
	list = new char[size + 1];
	list[size] = '\0';
	return list;
}

void WordList::SetFromAllocated() {
	sorted = false;
	words = ArrayFromWordList(list, &len, onlyLineEnds);
	wordsNoCase = new char * [len + 1];
	memcpy(wordsNoCase, words, (len + 1) * sizeof (*words));
}

int cmpString(const void *a1, const void *a2) {
	// Can't work out the correct incantation to use modern casts here
	return strcmp(*(char**)(a1), *(char**)(a2));
}

int cmpStringNoCase(const void *a1, const void *a2) {
	// Can't work out the correct incantation to use modern casts here
	return CompareCaseInsensitive(*(char**)(a1), *(char**)(a2));
}

static void SortWordList(char **words, char **wordsNoCase, unsigned int len) {
	qsort(reinterpret_cast<void*>(words), len, sizeof(*words),
	      cmpString);
	qsort(reinterpret_cast<void*>(wordsNoCase), len, sizeof(*wordsNoCase),
	      cmpStringNoCase);
}

bool WordList::InList(const char *s) {
	if (0 == words)
		return false;
	if (!sorted) {
		sorted = true;
		SortWordList(words, wordsNoCase, len);
		for (unsigned int k = 0; k < (sizeof(starts) / sizeof(starts[0])); k++)
			starts[k] = -1;
		for (int l = len - 1; l >= 0; l--) {
			unsigned char indexChar = words[l][0];
			starts[indexChar] = l;
		}
	}
	unsigned char firstChar = s[0];
	int j = starts[firstChar];
	if (j >= 0) {
		while (words[j][0] == firstChar) {
			if (s[1] == words[j][1]) {
				const char *a = words[j] + 1;
				const char *b = s + 1;
				while (*a && *a == *b) {
					a++;
					b++;
				}
				if (!*a && !*b)
					return true;
			}
			j++;
		}
	}
	return false;
}

/**
 * Returns an element (complete) of the wordlist array which has
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved (default).
 */
const char *WordList::GetNearestWord(const char *wordStart, int searchLen /*= -1*/, bool ignoreCase /*= false*/) {
	int start = 0; // lower bound of the api array block to search
	int end = len - 1; // upper bound of the api array block to search
	int pivot; // index of api array element just being compared
	int cond; // comparison result (in the sense of strcmp() result)
	const char *word; // api array element just being compared

	if (0 == words)
		return NULL;
	if (!sorted) {
		sorted = true;
		SortWordList(words, wordsNoCase, len);
	}
	if (ignoreCase) {
		while (start <= end) { // binary searching loop
			pivot = (start + end) >> 1;
			word = wordsNoCase[pivot];
			cond = CompareNCaseInsensitive(wordStart, word, searchLen);
			if (!cond && nonFuncChar(word[searchLen])) // maybe there should be a "non-word character" test here?
				return word; // result must not be freed with free()
			else if (cond > 0)
				start = pivot + 1;
			else if (cond <= 0)
				end = pivot - 1;
		}
	} else { // preserve the letter case
		while (start <= end) { // binary searching loop
			pivot = (start + end) >> 1;
			word = words[pivot];
			cond = strncmp(wordStart, word, searchLen);
			if (!cond && nonFuncChar(word[searchLen])) // maybe there should be a "non-word character" test here?
				return word; // result must not be freed with free()
			else if (cond > 0)
				start = pivot + 1;
			else if (cond <= 0)
				end = pivot - 1;
		}
	}
	return NULL;
}

/** 
 * Find the length of a 'word' which is actually an identifier in a string 
 * which looks like "identifier(..." or "identifier:" or "identifier" and where
 * there may be extra spaces after the identifier that should not be 
 * counted in the length.
 */
static unsigned int LengthWord(const char *word, char otherSeparator) {
	// Find a '(', or ':'. If that fails go to the end of the string.
 	const char *endWord = strchr(word, '(');
	if (!endWord)
		endWord = strchr(word, ':');
	if (!endWord && otherSeparator)
		endWord = strchr(word, otherSeparator);
	if (!endWord)
		endWord = word + strlen(word);
	// Last case always succeeds so endWord != 0
	
	// Drop any space characters.
	if (endWord > word) {
		endWord--;	// Back from the '(', ':', or '\0'
		// Move backwards over any spaces
		while ((endWord > word) && (isspace(*endWord))) {
			endWord--;
		}
	}
	return endWord - word;
}

/**
 * Accumulate words in a space separated string
 */
class WordAccumulator {
	/// How many characters will be pre-allocated (to avoid buffer reallocation on each new word)
	enum {wordChunk = 1000};
	/// Length of the returned buffer of words (string)
	unsigned int length; 
	/// Allocated size of the buffer
	unsigned int size;
public:
	/// Buffer for the words returned - this must be freed by client using delete[].
	char *buffer; 
	WordAccumulator() : length(0), size(wordChunk), buffer(0) {
		buffer = new char[size];
		if (buffer)
			buffer[0] = '\0';
	}
	void Append(const char *word, unsigned int wordLen) {
		if (!buffer)
			return;
		unsigned int newLength = length + wordLen; // stretch the buffer
		if (length)
			newLength++;
		if (newLength >= size) {
			unsigned int newSize = (((newLength+1) / wordChunk) + 1) * wordChunk;
			char *newBuffer = new char[newSize];
			if (!newBuffer)
				return;
			memcpy(newBuffer, buffer, length);
			delete []buffer;
			buffer = newBuffer;
			size = newSize;
		}
		if (length) // append a new entry
			buffer[length++] = ' ';
		memcpy(buffer + length, word, wordLen);
		length = newLength;
		buffer[length] = '\0';
	}
};

/**
 * Returns elements (first words of them) of the wordlist array which have
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved (default).
 * If there are more words meeting the condition they are returned all of
 * them in the ascending order separated with spaces.
 *
 * NOTE: returned buffer has to be freed with delete[].
 */
char *WordList::GetNearestWords(
	const char *wordStart, 
	int searchLen /*= -1*/, 
	bool ignoreCase /*= false*/, 
	char otherSeparator /*= '\0'*/) {
	int wordlen; // length of the word part (before the '(' brace) of the api array element
	WordAccumulator wordsNear;
	int start = 0; // lower bound of the api array block to search
	int end = len - 1; // upper bound of the api array block to search
	int pivot; // index of api array element just being compared
	int cond; // comparison result (in the sense of strcmp() result)
	int oldpivot; // pivot storage to be able to browse the api array upwards and then downwards
	const char *word; // api array element just being compared

	if (0 == words)
		return NULL;
	if (!sorted) {
		sorted = true;
		SortWordList(words, wordsNoCase, len);
	}
	if (ignoreCase) {
		while (start <= end) { // binary searching loop
			pivot = (start + end) >> 1;
			word = wordsNoCase[pivot];
			cond = CompareNCaseInsensitive(wordStart, word, searchLen);
			if (!cond) {
				oldpivot = pivot;
				do { // browse sequentially the rest after the hit
					wordlen = LengthWord(word, otherSeparator) + 1;
					wordsNear.Append(word, wordlen);
					if (++pivot > end)
						break;
					word = wordsNoCase[pivot];
				} while (!CompareNCaseInsensitive(wordStart, word, searchLen));

				pivot = oldpivot;
				for (;;) { // browse sequentially the rest before the hit
					if (--pivot < start)
						break;
					word = wordsNoCase[pivot];
					if (CompareNCaseInsensitive(wordStart, word, searchLen))
						break;
					wordlen = LengthWord(word, otherSeparator) + 1;
					wordsNear.Append(word, wordlen);
				}
				return wordsNear.buffer;
			}
			else if (cond < 0)
				end = pivot - 1;
			else if (cond > 0)
				start = pivot + 1;
		}
	} else {	// preserve the letter case
		while (start <= end) { // binary searching loop
			pivot = (start + end) >> 1;
			word = words[pivot];
			cond = strncmp(wordStart, word, searchLen);
			if (!cond) {
				oldpivot = pivot;
				do { // browse sequentially the rest after the hit
					wordlen = LengthWord(word, otherSeparator) + 1;
					wordsNear.Append(word, wordlen);
					if (++pivot > end)
						break;
					word = words[pivot];
				} while (!strncmp(wordStart, word, searchLen));

				pivot = oldpivot;
				for (;;) { // browse sequentially the rest before the hit
					if (--pivot < start)
						break;
					word = words[pivot];
					if (strncmp(wordStart, word, searchLen))
						break;
					wordlen = LengthWord(word, otherSeparator) + 1;
					wordsNear.Append(word, wordlen);
				}
				return wordsNear.buffer;
			}
			else if (cond < 0)
				end = pivot - 1;
			else if (cond > 0)
				start = pivot + 1;
		}
	}
	delete []wordsNear.buffer;
	return NULL;
}
