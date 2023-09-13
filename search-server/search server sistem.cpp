#include "process_queries.h"
#include "search_server.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

using namespace std;

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



void TestFindTopDocument() {
	const int document_id = 5;

	const int document_id_second = 6;

	const string& content = "i go to find my dog"s;

	const string& content_second = "fly to cat"s;

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


void TestFindTopDocument_Par() {
	const int document_id = 5;

	const int document_id_second = 6;

	const string& content = "i go to find my dog"s;

	const string& content_second = "fly to cat"s;

	const vector<int> ratings = { 1, 2, 3, 5 };

	const vector<int> ratings_second = { 2, 3, 5 };

	{

		SearchServer server;

		server.AddDocument(document_id, content, DocumentStatus::ACTUAL, ratings);

		server.AddDocument(document_id_second, content_second, DocumentStatus::BANNED, ratings_second);

		const auto& test = server.FindTopDocuments(execution::par, "find"s);

		const auto& test_second = server.FindTopDocuments(execution::par, "cat"s);

		ASSERT_EQUAL_HINT(test.size(), 1, "FindTopDocument must be must  \"ACTUAL\" ");

		ASSERT_HINT(test_second.empty(), "FindTopDocument doens't must be must  \"BANNED\" ");

	}

}


void TestFindTopDocument_Seq() {
	const int document_id = 5;

	const int document_id_second = 6;

	const string& content = "i go to find my dog"s;

	const string& content_second = "fly to cat"s;

	const vector<int> ratings = { 1, 2, 3, 5 };

	const vector<int> ratings_second = { 2, 3, 5 };

	{

		SearchServer server;

		server.AddDocument(document_id, content, DocumentStatus::ACTUAL, ratings);

		server.AddDocument(document_id_second, content_second, DocumentStatus::BANNED, ratings_second);

		const auto& test = server.FindTopDocuments(execution::seq, "find"s);

		const auto& test_second = server.FindTopDocuments(execution::seq, "cat"s);

		ASSERT_EQUAL_HINT(test.size(), 1, "FindTopDocument must be must  \"ACTUAL\" ");

		ASSERT_HINT(test_second.empty(), "FindTopDocument doens't must be must  \"BANNED\" ");

	}

}

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


}

void TestToSplitIntoWords() {

	const vector<string_view> words = { "I", "go", "dog" };

	const string_view text = "I go dog";

	ASSERT_HINT(SplitIntoWords(text) == words, "ERROR IN THE ToSplitIntoWords function");

}


void TestMinusWord() {

	const int document_id = 1;

	const string& content = "белый кот и модный ошейник"s;

	const vector<int> rating = { 1, 2, 3 };

	const int document_id_sec = 2;

	const string& content_sec = "белый кот и модный"s;

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

	const string& content_first = "кошка бежит домой"s;

	const string& content_second = "кошка в будке"s;

	const string& content_third = "кошка на дереве"s;

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

	const string& content_first = "кошка бежит домой"s;

	const string& content_second = "кошка в будке"s;

	const string& content_third = "кошка на дереве"s;

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

	ASSERT(abs(doc_first.relevance - relevance) < MAX_RELEVANCE_DIFFERENCE); // error

}

void TestRelevanceTop() { //  тест на вывод релеваотности по убыванию 

	SearchServer server;

	const int id_document_first = 0;

	const int id_document_second = 1;

	const int id_document_third = 2;

	const string& content_first = "кошка бежит в будке"s;

	const string& content_second = "кошка бежит домой"s;

	const string& content_third = "кошка на дереве"s;

	const vector<int> rating_first = { 1, -5,8 }; // rating 1; 

	const vector<int> rating_second = { 2, 10, -5, 3 }; // rating 2  

	const vector<int> rating_third = { 1, 8, 10, -6 }; //rating 3 

	server.AddDocument(id_document_first, content_first, DocumentStatus::ACTUAL, rating_first);

	server.AddDocument(id_document_second, content_second, DocumentStatus::ACTUAL, rating_second);

	server.AddDocument(id_document_third, content_third, DocumentStatus::ACTUAL, rating_third);

	server.AddDocument(4, "муха кошка и собака бегут домой", DocumentStatus::ACTUAL, { 10,4 });

	const auto document_ = server.FindTopDocuments("кошка бежит домой"s);

	const Document& doc_first = document_[0];

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

	const string& content_first = "кошка бежит в будке"s;

	const string& content_second = "кошка бежит домой"s;

	const string& content_third = "кошка на дереве"s;

	const string& content_forth = "кошка бежит от собаки";

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

	const string& content_first = "кошка бежит в будке"s;

	const string& content_second = "кошка бежит домой"s;

	const string& content_third = "кошка на дереве"s;

	const string& content_forth = "кошка бежит от собаки";

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

void PrintDocument(const Document& document) {
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << endl;
}


int main() {
	// TEST
	{
		TestFindTopDocument_Seq();
		TestFindTopDocument_Par();
		TestFindTopDocument();
		TestExcludeStopWordsFromAddedDocumentContent();
		TestToSplitIntoWords();
		TestMinusWord();
		TestComputerating();
		TestRelevance();
		TestRelevanceTop();
		TestSearchStatus();
		FilterResultPlusPredicat();

	}

	SearchServer search_server("and with"s);
	int id = 0;
	for (
		const string& text : {
			"white cat and yellow hat"s,
			"curly cat curly tail"s,
			"nasty dog with big eyes"s,
			"nasty pigeon john"s,
		}
		) {
		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	}
	cout << "ACTUAL by default:"s << endl;
	// последовательная версия
	for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
		PrintDocument(document);
	}
	cout << "BANNED:"s << endl;
	// последовательная версия
	for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
		PrintDocument(document);
	}
	cout << "Even ids:"s << endl;
	// параллельная версия
	for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
		PrintDocument(document);
	}
	return 0;

}


