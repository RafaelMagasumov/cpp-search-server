#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include<cassert>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MAX_DIFFERENCE_RELEVANCE = 1e-6;


string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result;
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
	int rating;
};

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer {
public:
	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status,
		const vector<int>& ratings) {
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {
		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}

	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {

		return FindTopDocuments(raw_query, [status](auto id, auto statuss, auto rating) {
			return statuss == status;
			});
	}



	template <typename Predicat>
	vector<Document> FindTopDocuments(const string& raw_query,
		Predicat predicat) const {
		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, predicat);

		sort(matched_documents.begin(), matched_documents.end(),
			[](const auto& lhs, const auto& rhs) {
				if (abs(lhs.relevance - rhs.relevance) < MAX_DIFFERENCE_RELEVANCE) {
					return lhs.rating > rhs.rating;
				}
				else {
					return lhs.relevance > rhs.relevance;
				}
			});
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

	int GetDocumentCount() const {
		return documents_.size();
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
		int document_id) const {
		const Query query = ParseQuery(raw_query);
		vector<string> matched_words;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.push_back(word);
			}
		}
		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.clear();
				break;
			}
		}
		return { matched_words, documents_.at(document_id).status };
	}

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;

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

	static int ComputeAverageRating(const vector<int>& ratings) {
		if (ratings.empty()) {
			return 0;
		}
		int rating_sum = 0;
		for (const int rating : ratings) {
			rating_sum += rating;
		}
		return rating_sum / static_cast<int>(ratings.size());
	}

	struct QueryWord {
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(string text) const {
		bool is_minus = false;
		// Word shouldn't be empty
		if (text[0] == '-') {
			is_minus = true;
			text = text.substr(1);
		}
		return { text, is_minus, IsStopWord(text) };
	}

	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const {
		Query query;
		for (const string& word : SplitIntoWords(text)) {
			const QueryWord query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					query.minus_words.insert(query_word.data);
				}
				else {
					query.plus_words.insert(query_word.data);
				}
			}
		}
		return query;
	}

	// Existence required
	double ComputeWordInverseDocumentFreq(const string& word) const {
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	template <typename Predicat>
	vector<Document> FindAllDocuments(const Query& query, const Predicat predicat) const {
		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word)) {
				if (predicat(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}

			}
		}

		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto[document_id, relevance] : document_to_relevance) {
			matched_documents.push_back(
				{ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}
};


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
	const string& func, unsigned line, const string& hint) {
	if (t != u) {
		cout << boolalpha;
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cout << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b)  AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
	const string& hint) {
	if (!value) {
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))



// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
			"Stop words must be excluded from documents"s);
	}
}

void TestToSplitIntoWords() {
	const vector<string> words = { "I", "go", "dog" };
	const string text = "I go dog";
	ASSERT_HINT(SplitIntoWords(text) == words, "ERROR IN THE ToSplitIntoWords function");
}

void TestFindTopDocument() {
	const int document_id = 5;
	const int document_id_second = 6;
	const string content = "i go to find my dog"s;
	const string content_second = "fly to cat"s;
	const vector<int> ratings = { 1, 2, 3, 5 };
	const vector<int> ratings_second = { 2, 3, 5 };
	{
		SearchServer server;
		server.AddDocument(document_id, content, DocumentStatus::ACTUAL, ratings);
		server.AddDocument(document_id_second, content_second, DocumentStatus::BANNED, ratings_second);
		const auto& test = server.FindTopDocuments("find"s);
		const auto& test_second = server.FindTopDocuments("cat"s);
		ASSERT_EQUAL_HINT(test.size(), 1, "FindTopDocument must be must  \"ACTUAL\" ");
		ASSERT_HINT(test_second.empty(), "FindTopDocument doens't must be must  \"BANNED\" ");
	}
}

void TestMinusWord() {
	const int document_id = 1;
	const string content = "белый кот и модный ошейник"s;
	const vector<int> rating = { 1, 2, 3 };
	const int document_id_sec = 2;
	const string content_sec = "белый кот и модный"s;
	const vector<int> rating_sec = { 1, 2, 5 };
	{
		SearchServer server;
		server.AddDocument(document_id, content, DocumentStatus::ACTUAL, rating);
		server.AddDocument(document_id_sec, content_sec, DocumentStatus::ACTUAL, rating_sec);
		ASSERT(server.FindTopDocuments("-белый").empty());
		ASSERT_EQUAL(server.FindTopDocuments("кот").size(), 2);
		ASSERT(server.FindTopDocuments("кот -модный"s).empty());
	}
}

void TestComputerating() {
	const int id_document_first = 0;
	const int id_document_second = 1;
	const int id_document_third = 2;
	const string content_first = "кошка бежит домой"s;
	const string content_second = "кошка в будке"s;
	const string content_third = "кошка на дереве"s;
	const vector<int> rating_first = { 1, -5,8 }; // rating 1;
	const vector<int> rating_second = { 2, 10, -5, 3 }; // rating 2 
	const vector<int> rating_third = { 1, 8, 10, -6 }; //rating 3
	{
		SearchServer server;
		server.AddDocument(id_document_first, content_first, DocumentStatus::ACTUAL, rating_first);
		server.AddDocument(id_document_second, content_second, DocumentStatus::ACTUAL, rating_second);
		server.AddDocument(id_document_third, content_third, DocumentStatus::ACTUAL, rating_third);
		const auto document_ = server.FindTopDocuments("кошка"s);
		const Document& doc_first = document_[0];
		const Document& doc_second = document_[1];
		const Document& doc_third = document_[2];
		ASSERT_EQUAL(doc_first.rating, (1 + 8 + 10 + (-6)) / 4);
		ASSERT_EQUAL(doc_second.rating, (2 + 10 + (-5) + 3) / 4);
		ASSERT_EQUAL(doc_third.rating, (1 + (-5) + 8) / 3);
	}
}

void TestRelevance() { // тест на подсчет релевантности 
	SearchServer server;
	const int id_document_first = 0;
	const int id_document_second = 1;
	const int id_document_third = 2;
	const string content_first = "кошка бежит домой"s;
	const string content_second = "кошка в будке"s;
	const string content_third = "кошка на дереве"s;
	const vector<int> rating_first = { 1, -5,8 }; // rating 1;
	const vector<int> rating_second = { 2, 10, -5, 3 }; // rating 2 
	const vector<int> rating_third = { 1, 8, 10, -6 }; //rating 3
	server.AddDocument(id_document_first, content_first, DocumentStatus::ACTUAL, rating_first);
	server.AddDocument(id_document_second, content_second, DocumentStatus::ACTUAL, rating_second);
	server.AddDocument(id_document_third, content_third, DocumentStatus::ACTUAL, rating_third);
	const auto document_ = server.FindTopDocuments("бежит"s);
	double IDF = log(3 / document_.size());
	double TF = 1 * 1.0 / 3;
	double relevance = IDF * TF;
	const Document& doc_first = document_[0];
	ASSERT(abs(doc_first.relevance - relevance) < MAX_DIFFERENCE_RELEVANCE);
}

void TestRelevanceTop() { //  тест на вывод релеваотности по убыванию
	SearchServer server;
	const int id_document_first = 0;
	const int id_document_second = 1;
	const int id_document_third = 2;
	const string content_first = "кошка бежит в будке"s;
	const string content_second = "кошка бежит домой"s;
	const string content_third = "кошка на дереве"s;
	const vector<int> rating_first = { 1, -5,8 }; // rating 1;
	const vector<int> rating_second = { 2, 10, -5, 3 }; // rating 2 
	const vector<int> rating_third = { 1, 8, 10, -6 }; //rating 3
	server.AddDocument(id_document_first, content_first, DocumentStatus::ACTUAL, rating_first);
	server.AddDocument(id_document_second, content_second, DocumentStatus::ACTUAL, rating_second);
	server.AddDocument(id_document_third, content_third, DocumentStatus::ACTUAL, rating_third);
	server.AddDocument(4, "муха кошка и собака бегут домой", DocumentStatus::ACTUAL, { 10,4 });
	const auto document_ = server.FindTopDocuments("кошка бежит домой"s);
	const Document &  doc_first = document_[0];
	const Document& doc_second = document_[1];
	const Document& doc_third = document_[2];
	const Document& doc_ford = document_[3];
	ASSERT_HINT(doc_first.relevance > doc_second.relevance, "error in sort relevance"s);
	ASSERT_HINT(doc_second.relevance > doc_third.relevance, "error int sort relevance"s);
	ASSERT_HINT(doc_third.relevance > doc_ford.relevance, "error in sort relevance"s);
}


void TestSearchStatus() { // поиск документов имеющих заданный статус
	SearchServer server;
	const int id_document_first = 0;
	const int id_document_second = 1;
	const int id_document_third = 2;
	const int id_document_forth = 3;
	const string content_first = "кошка бежит в будке"s;
	const string content_second = "кошка бежит домой"s;
	const string content_third = "кошка на дереве"s;
	const string content_forth = "кошка бежит от собаки";
	const vector<int> rating_first = { 1, -5,8 }; // rating 1;
	const vector<int> rating_second = { 2, 10, -5, 3 }; // rating 2 
	const vector<int> rating_third = { 1, 8, 10, -6 }; //rating 3
	const vector<int> rating_forth = { 9 };
	server.AddDocument(id_document_first, content_first, DocumentStatus::ACTUAL, rating_first);
	server.AddDocument(id_document_second, content_second, DocumentStatus::BANNED, rating_second);
	server.AddDocument(id_document_third, content_third, DocumentStatus::IRRELEVANT, rating_third);
	server.AddDocument(id_document_forth, content_forth, DocumentStatus::REMOVED, rating_forth);
	const auto document_ = server.FindTopDocuments("бежит"s, DocumentStatus::ACTUAL);
	const auto document_second = server.FindTopDocuments("бежит"s, DocumentStatus::BANNED);
	const auto document_third = server.FindTopDocuments("кошка"s, DocumentStatus::IRRELEVANT);
	const auto document_forth = server.FindTopDocuments("кошка"s, DocumentStatus::REMOVED);
	ASSERT_EQUAL(document_.size(), 1); // соовтетвие по статусу
	ASSERT_EQUAL(document_second.size(), 1);
	ASSERT_EQUAL(document_third.size(), 1);
	ASSERT_EQUAL(document_forth.size(), 1);

	ASSERT_EQUAL(document_[0].id, 0);  // соответсвие документво по айди показывает что нашелся именно тот документ
	ASSERT_EQUAL(document_second[0].id, 1);
	ASSERT_EQUAL(document_third[0].id, 2);
	ASSERT_EQUAL(document_forth[0].id, 3);
}

void FilterResultPlusPredicat() { // фильтр резултатов поиска с использованием предиката задаваемого пользователем
	SearchServer server;
	const int id_document_first = 0;
	const int id_document_second = 1;
	const int id_document_third = 2;
	const int id_document_forth = 3;
	const string content_first = "кошка бежит в будке"s;
	const string content_second = "кошка бежит домой"s;
	const string content_third = "кошка на дереве"s;
	const string content_forth = "кошка бежит от собаки";
	const vector<int> rating_first = { 1, -5,8 }; // rating 1;
	const vector<int> rating_second = { 2, 10, -5, 3 }; // rating 2 
	const vector<int> rating_third = { 1, 8, 10, -6 }; //rating 3
	const vector<int> rating_forth = { 9 };
	server.AddDocument(id_document_first, content_first, DocumentStatus::ACTUAL, rating_first);
	server.AddDocument(id_document_second, content_second, DocumentStatus::ACTUAL, rating_second);
	server.AddDocument(id_document_third, content_third, DocumentStatus::ACTUAL, rating_third);
	server.AddDocument(id_document_forth, content_forth, DocumentStatus::ACTUAL, rating_forth);
	const auto document_ = server.FindTopDocuments("кошка"s, [](int document_id, DocumentStatus status, int rating) { return rating > 2; }); // например рейтинг больше двух
	ASSERT_EQUAL(document_.size(), 2);
	const auto document_first_test = server.FindTopDocuments("кошка"s, [](int document_id, DocumentStatus status, int rating) { return document_id < 2; });
	ASSERT_EQUAL(document_first_test.size(), 2); // id меньше 2
	const auto document_second_test = server.FindTopDocuments("кошка"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::ACTUAL;});
	ASSERT_EQUAL(document_second_test.size(), 4);
}

void TestMatchDocument() {
	SearchServer server;
	const int id_document_first = 0;
	const int id_document_second = 1;
	server.AddDocument(id_document_first, "кошка собака попугай"s, DocumentStatus::ACTUAL, { 1 ,3,5 });
	server.AddDocument(id_document_second, "пес черепаха осел "s, DocumentStatus::ACTUAL, { 10, 5, 3 });
	const auto doc_first = server.MatchDocument("-кошка собака попугай"s, id_document_first);
	const auto doc_second = server.MatchDocument("пес черепаха -кошка"s, id_document_second);

	const auto[words, status] = doc_second; // проверка что только соответсвующие слова 
	const vector<string> word_second = { "пес"s, "черепаха" };
	ASSERT_EQUAL(words[0], word_second[0]); // пес
	ASSERT_EQUAL(words[1], word_second[1]);  // черепаха

	ASSERT(get<vector<string>>(doc_first).empty()); // проверка где минус слово там пусто! 
}



template <typename T>
void RunTestImpl(const T& func_name, const string& name) {
	func_name();
	cerr << name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)


void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestToSplitIntoWords);
	RUN_TEST(TestFindTopDocument);
	RUN_TEST(TestMinusWord);
	RUN_TEST(TestComputerating);
	RUN_TEST(TestRelevance);
	RUN_TEST(TestRelevanceTop);
	RUN_TEST(TestSearchStatus);
	RUN_TEST(FilterResultPlusPredicat);
	RUN_TEST(TestMatchDocument);
}
// ==================== для примера =========================

// ==================== для примера =========================

void PrintDocument(const Document& document) {
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << endl;
}

int main() {
	TestSearchServer();
	// Если вы видите эту строку, значит все тесты прошли успешно
	cout << "Search server testing finished"s << endl;
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
	cout << "ACTUAL by default:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
		PrintDocument(document);
	}
	cout << "BANNED:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
		PrintDocument(document);
	}
	cout << "Even ids:"s << endl;
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
		PrintDocument(document);
	}
	return 0;
}