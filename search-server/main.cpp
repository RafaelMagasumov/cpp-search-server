#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result = 0;
	cin >> result;
	ReadLine();
	return result;
}

vector<string> SplitIntoWords(const string& text) {
	vector<string> words;
	string word;
	for (const char c : text) {
		if (c == ' ') {
			if (!word.empty()) {
				words.push_back(word);
				word.clear();
			}
		}
		else {
			word += c;
		}
	}
	if (!word.empty()) {
		words.push_back(word);
	}

	return words;
}

struct Document {
	int id;
	double relevance;
};

struct Query {
	set <string> plus_word;
	set <string> minus_word;
};

class SearchServer {
public:
	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document) {
		++document_count_;
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double TF = 1. / words.size();
		for (const string& s : words) {
			word_to_documents_freqs_[s][document_id] += TF; // обращение сначала к map<int,double> потом к double 
		}
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {
		const Query query_words = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query_words);

		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				return lhs.relevance > rhs.relevance;
			});
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

private:
	int document_count_ = 0;
	map<string, map<int, double>> word_to_documents_freqs_;

	set<string> stop_words_;

	bool IsStopWord(const string& word) const {
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (!IsStopWord(word)) {
				words.push_back(word);
			}
		}
		return words;
	}

	Query ParseQuery(const string& text) const {
		set<string> zapros;
		Query query_words;
		for (const string& word : SplitIntoWordsNoStop(text)) {
			zapros.insert(word);
		}
		for (string s : zapros) {
			//char c = s[0];
			if (s[0] == '-') {
				s = s.substr(1);
				query_words.minus_word.insert(s);
			}
			else {
				query_words.plus_word.insert(s);
			}
		}
		return query_words;
	}
	double IDFschet(const string& word) const {
		return log(document_count_ * 1. / word_to_documents_freqs_.at(word).size());
	}

	vector<Document> FindAllDocuments(const Query& query_words) const {
		map<int, double> document_to_relevance;
		vector<Document> matched_documents;
		for (const string& word : query_words.plus_word) {
			if (word_to_documents_freqs_.count(word)) {

				const double IDF = IDFschet(word);
				for (const auto&[id, tf] : word_to_documents_freqs_.at(word)) {
					document_to_relevance[id] += tf * IDF;
				}
			}
		}

		for (const string& word : query_words.minus_word) {
			if (word_to_documents_freqs_.count(word)) {
				for (auto&[id, tf] : word_to_documents_freqs_.at(word)) {
					document_to_relevance.erase(id);
				}
			}
		}


		for (auto& s : document_to_relevance) {
			matched_documents.push_back({ s.first, s.second });
		}
		return matched_documents;
	}

};

SearchServer CreateSearchServer() {
	SearchServer search_server;
	search_server.SetStopWords(ReadLine());

	const int document_count = ReadLineWithNumber();
	for (int document_id = 0; document_id < document_count; ++document_id) {
		search_server.AddDocument(document_id, ReadLine());
	}

	return search_server;
}

int main() {
	const SearchServer search_server = CreateSearchServer();

	const string query = ReadLine();
	for (const auto&[document_id, relevance] : search_server.FindTopDocuments(query)) {
		cout << "{ document_id = "s << document_id << ", "
			<< "relevance = "s << relevance << " }"s << endl;
	}
}