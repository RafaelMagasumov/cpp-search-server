#include "remove_duplicates.h"
#include<iostream>
bool CompasionOfTwoMaps(const std::map<std::string, double>& map1,
	const std::map<std::string, double>& map2)
{
	if (map1.size() != map2.size()) return false;

	std::map<std::string, double>::const_iterator pos1 = map1.begin();
	std::map<std::string, double>::const_iterator pos2 = map2.begin();

	while (pos1 != map1.end())
	{
		if ((*pos1).first != (*pos2).first) return false;
		++pos1;
		++pos2;
	}

	return true;
}

void RemoveDuplicates(SearchServer& search_server) {
	std::set<int> duplicates;
	// auto it_f = search_server.begin();
	// auto it_n = next(it_f);
	for (auto it_f = search_server.begin(); it_f != search_server.end(); ++it_f) {
		auto it_n = std::next(it_f);
		while (it_n != search_server.end()) {
			if (CompasionOfTwoMaps(search_server.GetWordFrequencies(*it_f), search_server.GetWordFrequencies(*it_n)))
			{
				duplicates.insert(*it_n);
			}
			++it_n;
		}
	}
	for (const int& document_id : duplicates)
	{
		std::cout << "Found duplicate document id " << document_id << '\n';
		search_server.RemoveDocument(document_id);
	}
}


