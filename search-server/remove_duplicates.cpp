#include "remove_duplicates.h"
#include<iostream>

void RemoveDuplicates(SearchServer& search_server) {
	std::vector<std::vector<std::string>>  helper;
	std::set<int> duplicates;

	for (auto it_f = search_server.begin(); it_f != search_server.end(); ++it_f) {
		std::vector<std::string> help_vector;
		for (const auto& word : search_server.GetWordFrequencies(*it_f)) {

			help_vector.push_back(word.first);
		}
		if (std::count(helper.begin(), helper.end(), help_vector)) {
			duplicates.insert(*it_f);
		}
		else helper.push_back(help_vector);


	}

	for (const int& document_id : duplicates)
	{
		std::cout << "Found duplicate document id " << document_id << '\n';
		search_server.RemoveDocument(document_id);
	}
}


