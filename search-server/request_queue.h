#pragma once
#include "search_server.h"
#include<deque>
#include"document.h"

class RequestQueue {
public:
	explicit RequestQueue(const SearchServer& search_server)
		: server_(search_server)
	{}


	template <typename DocumentPredicate>
	vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);

	vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);

	vector<Document> AddFindRequest(const string& raw_query);

	int GetNoResultRequests() const;

private:
	struct QueryResult {
		string query;
		vector<Document> result;
	};
	deque<QueryResult> requests_;
	const static int min_in_day_ = 1440;
	const  SearchServer& server_;
	int64_t minute_in_query = 0; // кол-во пустых запросов
};