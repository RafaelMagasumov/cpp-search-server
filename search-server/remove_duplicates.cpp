#include "remove_duplicates.h"
#include<iostream>

void RemoveDuplicates(SearchServer& search_server) {
	std::set<std::set<std::string>>  helper;
	std::set<int> duplicates;

	for (auto it_f = search_server.begin(); it_f != search_server.end(); ++it_f) {
		std::set<std::string> help_vector;
		for (const auto& word : search_server.GetWordFrequencies(*it_f)) {

			help_vector.insert(word.first);
		}
		if (std::count(helper.begin(), helper.end(), help_vector)) {
			duplicates.insert(*it_f);
		}
		else helper.insert(help_vector);


	}

	for (const int& document_id : duplicates)
	{
		std::cout << "Found duplicate document id " << document_id << '\n';
		search_server.RemoveDocument(document_id);
	}
}


