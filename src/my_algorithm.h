/*
 * algorithm.h
 *
 *  Created on: 20/ott/2012
 *      Author: carlo
 */

#ifndef MY_ALGORITHM_H_
#define MY_ALGORITHM_H_

#include <algorithm>
#include <vector>
#include <cassert>
#include <utility>

using namespace std;

namespace cpputils{
/**
 * Returns true if ele is in vec. Ele has to be passed by value
 */
template<typename T>
bool canFind(vector<T>& vec, T ele){
	return find(vec.begin(), vec.end(), ele) != vec.end();
}

//returns the number of elements < val
template <class T>
int binarySearch(T& vec, double val){  //vec has to be ordered
    int min = 0;
    int max = vec.size() + 1;
    int mean;
    for(;;){
        mean = (min + max) / 2;
        if(max == min + 1) break;
        if(val > vec[mean-1]){
            min = mean;
        } else {
            max = mean;
        }
    }
    return mean;
}

/// gives back [0,...,n-1]
vector<int> range(int n){
	vector<int> r(n);
	for(auto i = 0; i < n ; i++){
		r[i] = i;
	}
	return r;
}

template<typename T>
vector<T> range(T i1, T i2, T di = 1){
	vector<T> r;

	assert((di > 0 && i1 <= i2) || (di < 0 && i1 >= i2));
	for(auto i = i1; i < i2 - 0.5*di; i += di){
		r.push_back(i);
	}

	return r;
}

//includes the extrema
template<typename T>
vector<T> irange(T i1, T i2, T di = 1){
	vector<T> r;

	assert((di > 0 && i1 <= i2) || (di < 0 && i1 >= i2));
	for(auto i = i1; i < i2 + 0.5*di; i += di){
		r.push_back(i);
	}

	return r;
}

//does  not include the right extrema, size is n
template<typename T>
vector<T> nrange(T x1, T x2, int n){
	vector<T> r;
    T dx = (x2 - x1 ) / n;
	for(T x = x1; x < x2 - 0.5*dx; x += dx){
		r.push_back(x);
	}
	return r;
}

//includes his extrema, size is n
template<typename T>
vector<T> nirange(T x1, T x2, int n){
	vector<T> r;
    T dx = (x2 - x1 ) / (n - 1);
	for(T x = x1; x < x2 + 0.5*dx; x += dx){
		r.push_back(x);
	}
	return r;
}



template <typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}


template<typename T>
T posPart(const T& a){
    if(a > 0){
        return a;
    } else{
        return 0;
    }
}

template<class T>
void erase(vector<T>& v, size_t pos){
    swap(v[pos], v.back());
    v.pop_back();
}

template<class T>
void erase(vector<T>& v, int pos){
    swap(v[pos], v.back());
    v.pop_back();
}

template<class T, class It>
void erase(vector<T>& v, It pos){
    swap(*pos, v.back());
    v.pop_back();
}

//no bound check
template<class T>
void eraseEle(vector<T>& v, const T& ele){
    auto it = find(v.begin(), v.end(), ele);
    erase(v, it);
}


#ifdef UNITTEST
#include  <gtest/gtest.h>

TEST(erase, pos1){
    vector<int> v = {1, 2, 3, 4};
    vector<int> v2 = {1, 4, 3};
    size_t pos = 1;
    erase(v, 1);
    EXPECT_TRUE(v == v2);
}

TEST(erase, pos2){
    vector<int> v = {1, 2, 3, 4};
    vector<int> v2 = {1, 4, 3};
    int pos = 1;
    erase(v, 1);
    EXPECT_TRUE(v == v2);
}

TEST(eraseEle, ele){
    vector<int> v = {1, 2, 3, 4};
    vector<int> v2 = {4, 2, 3};
    int ele = 1;
    eraseEle(v, 1);
    EXPECT_TRUE(v == v2);
}
#endif //UNITTEST

} //cpputils

#endif /* ALGORITHM_H_ */
