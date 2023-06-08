#pragma once
#include <string>
#include <vector>
#include <set>

using namespace std;

vector<string> SplitIntoWords(const string& text);



template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings);

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator it_begin, Iterator it_end) :
        it_begin_(it_begin), it_end_(it_end) {}


    Iterator begin() {
        return it_begin_;
    }

    Iterator end() {
        return it_end_;
    }

    int size() {
        return distance(it_begin_, it_end_);
    }

private:
    Iterator it_begin_;
    Iterator it_end_;

};