#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>

// -----------------------------------------------------------------------------------------------------------------------------------------------
//                                                                      Helper functions
// -----------------------------------------------------------------------------------------------------------------------------------------------

std::string xorStrings(const std::string& str1, const std::string& str2) {
    std::string result;
    for (size_t i = 0; i < str1.size(); ++i) {
        result.push_back(str1[i] ^ str2[i]);
    }
    return result;
}

std::string decryptMessage(const std::string& encryptedMessage, const std::string& k1, const std::string& k2) {
    size_t blockSize = k2.length();
    size_t numBlocks = encryptedMessage.length() / blockSize;

    std::vector<std::string> blocks;
    for (size_t i = 0; i < numBlocks; ++i) {
        size_t startIdx = i * blockSize;
        blocks.push_back(encryptedMessage.substr(startIdx, blockSize));
    }

    std::string C1;
    for (const auto& block : blocks) {
        C1 += xorStrings(block, k2);
    }

    std::string decryptedHex = xorStrings(C1, k1);

    // Convert hex back to string
    std::string decryptedMessage;
    for (size_t i = 0; i < decryptedHex.length(); i += 2) {
        std::string byteString = decryptedHex.substr(i, 2);
        char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
        decryptedMessage.push_back(byte);
    }

    return decryptedMessage;
}

void decrypt_and_print_flag() {
    std::ifstream inFile("flag.txt");
    if (inFile.is_open()) {
        std::string k1, k2, encryptedFlag;
        
        // Read K1, K2, and the encrypted flag from the file
        std::getline(inFile, k1);
        std::getline(inFile, k2);
        std::getline(inFile, encryptedFlag);
        inFile.close();

        // Decrypt the flag
        std::string decryptedFlag = decryptMessage(encryptedFlag, k1, k2);
        std::cout << "Decrypted Flag: " << decryptedFlag << std::endl;
    } else {
        std::cerr << "Error opening file for reading" << std::endl;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------------------
//                                                                      Main 
// -----------------------------------------------------------------------------------------------------------------------------------------------

int main() {
    std::srand(std::time(nullptr));  // Seed the random number generator with the current time
    unsigned int correct_number = std::rand();  // Generate a random number
    unsigned long long int user_input;

    while (true) {
        std::cout << "Enter the number: ";
        std::cin >> user_input;

        if (user_input == correct_number) {
            decrypt_and_print_flag();
            break;
        } else {
            std::cout << "Wrong number, try again." << std::endl;
        }
    }

    return 0;
}
