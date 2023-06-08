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
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        auto result = server_.FindTopDocuments(raw_query, document_predicate);
        if (result.empty()) {
            ++minute_in_query; // кол-во пустых запросов
        }
        requests_.push_back({ raw_query,result }); // requests_.size()-кол-во всех запросов
        while (requests_.size() > min_in_day_) {
            requests_.pop_front();
            --minute_in_query;
        }
        return result;
        // напишите реализацию
    }

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status) {
        auto result = server_.FindTopDocuments(raw_query, status);
        if (result.empty()) {
            ++minute_in_query; // кол-во пустых запросов
        }
        requests_.push_back({ raw_query,result }); // requests_.size()-кол-во всех запросов
        while (requests_.size() > min_in_day_) {
            requests_.pop_front();
            --minute_in_query;
        }
        return result;        // напишите реализацию
    }

    vector<Document> AddFindRequest(const string& raw_query) {
        auto result = server_.FindTopDocuments(raw_query);
        if (result.empty()) {
            ++minute_in_query; // кол-во пустых запросов
        }
        requests_.push_back({ raw_query,result }); // requests_.size()-кол-во всех запросов
        while (requests_.size() > min_in_day_) {
            requests_.pop_front();
            --minute_in_query;
        }
        return result;
        // напишите реализацию
    }

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