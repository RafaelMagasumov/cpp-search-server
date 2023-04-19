#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

// -------- Начало модульных тестов поисковой системы ----------

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
		ASSERT_EQUAL(server.FindTopDocuments("-белый").size(), 0);
		ASSERT_EQUAL(server.FindTopDocuments("кот").size(), 2);
		ASSERT_EQUAL(server.FindTopDocuments("кот -модный"s).size(), 0);
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
		ASSERT_EQUAL(doc_first.rating, 3);
		ASSERT_EQUAL(doc_second.rating, 2);
		ASSERT_EQUAL(doc_third.rating, 1);
	}
}





/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestToSplitIntoWords);
	RUN_TEST(TestFindTopDocument);
	RUN_TEST(TestMinusWord);
	RUN_TEST(TestComputerating);
	// Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
	TestSearchServer();
	// Если вы видите эту строку, значит все тесты прошли успешно
	cout << "Search server testing finished"s << endl;
}