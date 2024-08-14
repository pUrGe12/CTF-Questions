#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <bitset>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std;

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                                        Huffman encoding scheme  
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* 
Code for Huffman taken from lots of online resources 
1. https://www.programiz.com/dsa/huffman-coding
2. chatGPT helped 
3. https://www.geeksforgeeks.org/huffman-coding-using-priority-queue 
*/

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                                        Helper functions
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

std::string binaryStringToInt(const std::string& binaryString) {
    std::string result = "0";

    for (char bit : binaryString) {

        int carry = 0;
        for (size_t i = 0; i < result.size(); ++i) {
            int num = (result[i] - '0') * 2 + carry;
            result[i] = (num % 10) + '0';
            carry = num / 10;
        }
        if (carry > 0) {
            result += (carry + '0');
        }

        if (bit == '1') {
            carry = 1;
            for (size_t i = 0; i < result.size(); ++i) {
                int num = (result[i] - '0') + carry;
                result[i] = (num % 10) + '0';
                carry = num / 10;
            }
            if (carry > 0) {
                result += (carry + '0');
            }
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}


std::vector<std::vector<int>> binaryStringToMatrix(const std::string& binaryString, int rows, int cols) {
    std::vector<std::vector<int>> matrix(rows, std::vector<int>(cols));
    int index = 0;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            matrix[r][c] = binaryString[index++] - '0';
        }
    }
    
    return matrix;
}

long long largestPrimeFactor(long long n) {

    long long largest = -1;

    while (n % 2 == 0) {
        largest = 2;
        n /= 2;
    }
    for (long long i = 3; i <= std::sqrt(n); i += 2) {
        while (n % i == 0) {
            largest = i;
            n /= i;
        }
    }
    if (n > 2) {
        largest = n;
    }

    return largest;
}

// ------------------------------------------------------------------------------------ MAIN ---------------------------------------------------------------------------------------

int main() {

    std::string encodedFlagNums = "1773 1166 1693 1110 795 1561 115 1879"; 
    std::cout << "This is the encoded flag: " << encodedFlagNums << std::endl;
    
    std::string data;
    std::cout << "What do you want to encode? ";
    std::getline(std::cin, data);

    std::unordered_map<char, unsigned> mp;
    mp['5'] = '1';
    mp['R'] = '1';
    mp['n'] = '1';
    mp['K'] = '1';
    mp['4'] = '1';
    mp['C'] = '1';
    mp['T'] = '2';
    mp['I'] = '2';
    mp['0'] = '1';
    mp['u'] = '1';
    mp['F'] = '1';
    mp['M'] = '1';
    mp['{'] = '1';
    mp['_'] = '1';
    mp['H'] = '1';
    mp['}'] = '1';
    mp['f'] = '2';
    mp['m'] = '1';

    std::unordered_map<char, unsigned> freqMap;
    for (char c : data) {
        freqMap[c]++;
    }
  
    try{
        HuffmanNode* root = buildHuffmanTree(mp);
        std::unordered_map<char, std::string> codes;
        buildCodes(root, "", codes);

        std::string encoded = encode(data, codes);

        int cols = largestPrimeFactor(encoded.size());
        int rows = encoded.size() / cols; 

        std::vector<std::vector<int>> Y = binaryStringToMatrix(encoded.substr(0, rows * cols), rows, cols);
        std::vector<std::string> integerRepresentations(rows);
        for (int i = 0; i < rows; ++i) {
            std::string binaryString = "";
            for (int j = 0; j < cols; ++j) {
                binaryString += std::to_string(Y[i][j]);
            }
            integerRepresentations[i] = binaryStringToInt(binaryString);
        }

        std::cout << "Encoded Input is: " << std::endl;
        for (const std::string& str : integerRepresentations) {
            std::cout << str << " ";
        }
        
        delete root;
        return 0;

    } catch(const std::out_of_range){
      
        HuffmanNode* root = buildHuffmanTree(freqMap);
        std::unordered_map<char, std::string> codes;
        buildCodes(root, "", codes);

        std::string encoded = encode(data, codes);

        int cols = largestPrimeFactor(encoded.size());
        int rows = encoded.size() / cols; 

        std::vector<std::vector<int>> Y = binaryStringToMatrix(encoded.substr(0, rows * cols), rows, cols);
        
        std::vector<std::string> integerRepresentations(rows);
        for (int i = 0; i < rows; ++i) {
            std::string binaryString = "";
            for (int j = 0; j < cols; ++j) {
                binaryString += std::to_string(Y[i][j]);
            }
            integerRepresentations[i] = binaryStringToInt(binaryString);
        }

        std::cout << "Encoded Input is: " << std::endl;
        for (const std::string& str : integerRepresentations) {
            std::cout << str << " ";
        }
        
        delete root;
        return 0;
    }
}
