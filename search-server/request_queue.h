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
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;

private:
	struct QueryResult {
		std::string query;
		std::vector<Document> result;
	};

	std::deque<QueryResult> requests_;

	const static int min_in_day_ = 1440;

	const  SearchServer& server_;

	int64_t minute_in_query = 0; // ���-�� ������ �������� 
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	auto result = server_.FindTopDocuments(raw_query, document_predicate);

	if (result.empty()) {
		++minute_in_query; // ���-�� ������ ��������  
	}

	requests_.push_back({ raw_query,result }); // requests_.size()-���-�� ���� ��������  

	while (requests_.size() > min_in_day_) {

		requests_.pop_front();

		--minute_in_query;
	}

	return result;
}