#include "search_server.h" 

#include "document.h" 

#include "request_queue.h"  

#include "paginator.h"  

#include <string> 

#include <vector> 

#include <stdexcept> 

#include <iostream> 

#include <numeric> // for std::accumulate() 

#include <algorithm> // std::transform // std::unique // std::copy_if



SearchServer::SearchServer(const std::string& stop_words_text) : SearchServer(
	SplitIntoWords(std::string_view(stop_words_text))) {
}

SearchServer::SearchServer(std::string_view stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}




void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,

	const std::vector<int>& ratings) {

	if ((document_id < 0) || (documents_.count(document_id) > 0)) {

		throw std::invalid_argument("Invalid document_id"); // проверка на id < 0 и на повторяющийся id 

	}

	storage.emplace_back(document);



	const auto words = SplitIntoWordsNoStop(storage.back()); // Добавляет пустоту 


	const double inv_word_count = 1.0 / words.size();

	for (const std::string_view word : words) {

		word_to_document_freqs_[word][document_id] += inv_word_count;

		get_document_freqs[document_id][word] += inv_word_count;

	}

	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

	document_ids_.insert(document_id);

}


// Обычная версия 


std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {

	return FindTopDocuments(

		raw_query, [status](int document_id, DocumentStatus document_status, int rating) {

			return document_status == status;

		});

}



std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {

	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);

}



// последовательная версия

std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, std::string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {

		return document_status == status;

		});
}


std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, std::string_view raw_query) const {

	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}



// параллельеая версия
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, std::string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {

		return document_status == status;

		});
}


std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, std::string_view raw_query) const {
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}



std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,

	int document_id) const {

	const auto query = ParseQuery(raw_query, true);

	std::vector<std::string_view> matched_words;

	if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view word) {
		return	word_to_document_freqs_.at(word).count(document_id);
		})) {
		return { matched_words,documents_.at(document_id).status };
	}

	for (const std::string_view word : query.plus_words) {

		if (!word_to_document_freqs_.count(word))  continue;

		if (word_to_document_freqs_.at(word).count(document_id)) {

			matched_words.push_back(word);

		}

	}
	return { matched_words, documents_.at(document_id).status };

}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy polity, std::string_view raw_query,

	int document_id) const {

	const auto query = ParseQuery(raw_query, false);

	std::vector<std::string_view> matched_words(query.plus_words.size());

	if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view word) {
		return	word_to_document_freqs_.at(word).count(document_id);
		})) {
		matched_words.clear();
		return { matched_words, documents_.at(document_id).status };
	}


	const  auto it = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](const std::string_view word) {
		return word_to_document_freqs_.at(word).count(document_id);
		});

	matched_words.erase(it, matched_words.end());

	std::sort(matched_words.begin(), matched_words.end());

	//std::unique(matched_words.begin(), matched_words.end());

	matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());


	return { matched_words, documents_.at(document_id).status };

}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy polity, std::string_view raw_query,

	int document_id) const {
	return MatchDocument(raw_query, document_id);
}



int SearchServer::GetDocumentCount() const {

	return documents_.size();

}



bool SearchServer::IsStopWord(const std::string_view word) const {

	return stop_words_.count(word) > 0;

}



bool SearchServer::IsValidWord(const std::string_view word) {

	// A valid word must not contain special characters 

	return std::none_of(word.begin(), word.end(), [](char c) {

		return c >= '\0' && c < ' ';

		});

}





std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const { // должны храниться как строки

	std::vector<std::string_view> temp = SplitIntoWords(text);

	std::vector<std::string_view> words;

	for (const std::string_view word : temp) {

		if (!IsValidWord(word)) {

			throw std::invalid_argument("Word " + std::string(word) + " is invalid");

		}

		if (!IsStopWord(word)) {

			words.push_back(word);

		}

	}

	return words;

}



int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {

	if (ratings.empty()) {

		return 0;

	}

	int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

	return rating_sum / static_cast<int>(ratings.size());

}



SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {

	if (text.empty()) {

		throw std::invalid_argument("Query word is empty");

	}

	std::string_view word = text;

	bool is_minus = false;

	if (word[0] == '-') {

		is_minus = true;

		word = word.substr(1);

	}

	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {

		throw std::invalid_argument("Query word " + std::string(text) + " is invalid");

	}



	return { word, is_minus, IsStopWord(word) };

}



double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {

	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());

}







std::set<int>::iterator SearchServer::begin() const {

	return document_ids_.begin();

}



std::set<int>::iterator SearchServer::end() const {

	return document_ids_.end();

}



const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {

	return get_document_freqs.at(document_id);

}



void SearchServer::RemoveDocument(int document_id) {

	documents_.erase(document_id);

	document_ids_.erase(document_id);

	auto it = get_document_freqs.find(document_id); // it - итератор на int который надо удалить 

	for (auto& k : (*it).second) word_to_document_freqs_.at(k.first).erase(document_id); // удаление из word_to_document_freqs_ (поиск по string) 

	this->get_document_freqs.erase(it); // удаление из get_document_freqs (поиск по id) 
}


void SearchServer::RemoveDocument(const std::execution::parallel_policy policy, int document_id) {
	std::vector<const std::string_view*> helper(get_document_freqs.at(document_id).size()); // храним указатели на слова
	std::transform(policy, get_document_freqs.at(document_id).begin(), get_document_freqs.at(document_id).end(), helper.begin(), [](const auto& m) {
		return &m.first;
		});

	std::for_each(policy, helper.begin(), helper.end(), [&](const auto& help) {

		word_to_document_freqs_.at(*help).erase(document_id);
		});

	get_document_freqs.erase(document_id);

	documents_.erase(document_id);

	document_ids_.erase(document_id);

}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy policy, int document_id) {
	RemoveDocument(document_id);
}





SearchServer::Query SearchServer::ParseQuery(const std::string_view text, const bool is_sequenced) const {

	Query result;

	for (const std::string_view word : SplitIntoWords(text)) {

		const auto query_word = ParseQueryWord(word);


		if (!query_word.is_stop) {

			if (query_word.is_minus) {

				result.minus_words.push_back(query_word.data);

			}

			else {

				result.plus_words.push_back(query_word.data);

			}

		}

	}


	if (is_sequenced) {
		std::sort(std::execution::par, result.plus_words.begin(), result.plus_words.end());

		std::sort(std::execution::par, result.minus_words.begin(), result.minus_words.end());

		//std::unique(result.plus_words.begin(), result.plus_words.end()); // Возвращает итератор на элемент за последним нового массива
		//std::unique(result.minus_words.begin(), result.minus_words.end()); // Возвращает итератор на элемент за последним нового массива

		result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());

		result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
	}

	return result;

}