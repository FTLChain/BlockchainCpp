#pragma once

/*
MIT License

Copyright (c) 2017 ZetaChain_Native

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "platform.hpp" // Platform Specific Stuff NOTE: Must Always be the first include in a file
#include <string> // std::string
#include <vector>
#include <map>

namespace ZetaChain_Native {
	namespace Conversions {
		std::vector<unsigned char> toBytes(double* d);

		std::vector<unsigned char> toBytes(float* f);

		std::vector<unsigned char> toBytes(long long* ll);

		std::vector<unsigned char> toBytes(long* l);

		std::vector<unsigned char> toBytes(int* i);

		std::vector<unsigned char> toBytes(short* s);

		std::vector<unsigned char> toBytes(unsigned long long* ll);

		std::vector<unsigned char> toBytes(unsigned long* l);

		std::vector<unsigned char> toBytes(unsigned int* i);

		std::vector<unsigned char> toBytes(unsigned short* s);

		std::vector<unsigned char> toBytes(bool* b);

		std::vector<unsigned char> toBytes(std::string str);

		template <class T>
		std::vector<T> mapToValues(std::map<std::string, T> values) {
			std::vector<T> data = std::vector<T>();
			for (auto it = values.begin(); it != values.end(); it++) {
				data.push_back(it->second);
			}
			return data;
		}

		template <class T>
		std::vector<std::string> mapToValuesString(std::map<std::string, T> values, int size) {
			std::vector<std::string> valueList = std::vector<std::string>(size);
			for (auto it = values.begin(); it != values.end(); it++) {
				valueList.push_back(it->second.toString());
			}
			return valueList;
		}

		template <class T>
		std::vector<std::string> mapToValuesStringPointer(std::map<std::string, T> values, int size) {
			std::vector<std::string> valueList = std::vector<std::string>(size);
			for (auto it = values.begin(); it != values.end(); it++) {
				valueList.push_back(it->second->toString());
			}
			return valueList;
		}

		template <class T>
		std::vector<std::string> mapToKeys(std::map<std::string, T> values, int size) {
			std::vector<std::string> keys = std::vector<std::string>(size);
			for (auto it = values.begin(); it != values.end(); it++) {
				keys.push_back(it->first);
			}
			return keys;
		}
	}
}
