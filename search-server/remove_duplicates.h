#pragma once
#include "search_server.h"



bool CompasionOfTwoMaps(const std::map<std::string, double>&,
	const std::map<std::string, double>&);

void RemoveDuplicates(SearchServer&);