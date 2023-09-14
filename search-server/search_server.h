#pragma once 
#include "document.h" 
#include "string_processing.h" 
#include "concurrent_map.h"
#include "log_duration.h"
#include <map> 
#include <set> 
#include <algorithm> 
#include<cmath> 
#include <stdexcept> 
#include <execution>
#include <compare>
#include <deque>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MAX_RELEVANCE_DIFFERENCE = 1e-6;

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);
	explicit SearchServer(const std::string& stop_words_text);
	explicit SearchServer(std::string_view stop_words_text);
	explicit SearchServer() = default;

	void AddDocument(int document_id, std::string_view document, DocumentStatus status,
		const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(std::string_view raw_query,
		DocumentPredicate document_predicate) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

	std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy, std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy, std::string_view raw_query) const;

	template <typename DocumentPredicate, typename Polity>
	std::vector<Document> FindTopDocuments(Polity polity, std::string_view raw_query, DocumentPredicate document_predicate) const;

	std::vector<Document> FindTopDocuments(std::execution::parallel_policy polity, std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(std::execution::parallel_policy polity, std::string_view raw_query) const;

	int GetDocumentCount() const;

	std::set<int>::iterator begin() const;

	std::set<int>::iterator end() const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	void RemoveDocument(int documents_id);

	void RemoveDocument(const std::execution::parallel_policy, int documents_id);

	void RemoveDocument(const std::execution::sequenced_policy, int document_id);

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy polity, std::string_view raw_query, 	int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy polity, std::string_view raw_query, int document_id) const;



private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	std::deque<std::string> storage;
	std::map<int, std::map<std::string_view, double>> get_document_freqs; // {id, {document, freqs}}. 
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; // {document, {id, freqs}}. 

	const std::set<std::string, std::less<>> stop_words_;

	std::map<int, DocumentData> documents_;

	std::set<int> document_ids_; 

	bool IsStopWord(const std::string_view word) const;

	static bool IsValidWord(const std::string_view word);

	std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(const std::string_view text) const;

	struct Query {
		std::vector<std::string_view> plus_words;
		std::vector<std::string_view> minus_words;
	};

	Query ParseQuery(const std::string_view text, const bool is_sequenced) const;

	double ComputeWordInverseDocumentFreq(const std::string_view word) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(std::execution::sequenced_policy policy, const Query& query, DocumentPredicate document_predicate) const;


	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(std::execution::parallel_policy policy, const Query& query, DocumentPredicate document_predicate) const;
};


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
	return FindTopDocuments(std::execution::par, raw_query, document_predicate);
}


template <typename DocumentPredicate, typename Polity>
std::vector<Document> SearchServer::FindTopDocuments(Polity polity, std::string_view raw_query,
	DocumentPredicate document_predicate) const {
	const auto query = ParseQuery(raw_query, true);
	auto matched_documents = SearchServer::FindAllDocuments(polity, query, document_predicate);
	sort(polity, matched_documents.begin(), matched_documents.end(),
		[](const Document& lhs, const Document& rhs) {

			if (std::abs(lhs.relevance - rhs.relevance) < MAX_RELEVANCE_DIFFERENCE) {
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


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
	DocumentPredicate document_predicate) const {
	std::map<int, double> document_to_relevance;
	for (const std::string_view word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {

			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

		for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word)) {

			const auto& document_data = documents_.at(document_id);

			if (document_predicate(document_id, document_data.status, document_data.rating)) {

				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}

		}
	}

	for (const std::string_view word : query.minus_words) {

		if (word_to_document_freqs_.count(word) == 0) {

			continue;

		}

		for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {

			document_to_relevance.erase(document_id);
		}
	}
	std::vector<Document> matched_documents;

	for (const auto[document_id, relevance] : document_to_relevance) {
		matched_documents.push_back(
			{ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::sequenced_policy policy, const Query& query,
	DocumentPredicate document_predicate) const {
	return FindAllDocuments(query, document_predicate);
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy policy, const Query& query,
	DocumentPredicate document_predicate) const {
	ConcurrentMap<int, double> document_to_relevance_two(100);
	std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](std::string_view word) {
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
		for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word)) {

			const auto& document_data = documents_.at(document_id);

			if (document_predicate(document_id, document_data.status, document_data.rating)) {

				//	document_to_relevance[document_id] += term_freq * inverse_document_freq;

				document_to_relevance_two[document_id].ref_to_value += term_freq * inverse_document_freq;
			}
		}
		});
	std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](std::string_view word) {

		for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {

			document_to_relevance_two.Erase(document_id);
		}
		});

	auto result = document_to_relevance_two.BuildOrdinaryMap();

	std::vector<Document> matched_documents;

	for (const auto[document_id, relevance] : result) {

		matched_documents.push_back(

			{ document_id, relevance, documents_.at(document_id).rating });

	}
	return matched_documents;
}

template <typename StringContainer>

SearchServer::SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
	if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {

		throw std::invalid_argument("Some of stop words are invalid");
	}
}