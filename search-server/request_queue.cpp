#include "request_queue.h"

int RequestQueue::GetNoResultRequests() const {
	return minute_in_query;
	// �������� ����������
}


template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
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
	// �������� ���������� 

}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
	auto result = server_.FindTopDocuments(raw_query, status);
	if (result.empty()) {
		++minute_in_query; // ���-�� ������ �������� 
	}
	requests_.push_back({ raw_query,result }); // requests_.size()-���-�� ���� �������� 
	while (requests_.size() > min_in_day_) {
		requests_.pop_front();
		--minute_in_query;
	}
	return result;        // �������� ���������� 
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
	auto result = server_.FindTopDocuments(raw_query);
	if (result.empty()) {
		++minute_in_query; // ���-�� ������ �������� 
	}
	requests_.push_back({ raw_query,result }); // requests_.size()-���-�� ���� �������� 
	while (requests_.size() > min_in_day_) {
		requests_.pop_front();
		--minute_in_query;
	}
	return result;
	// �������� ���������� 

}