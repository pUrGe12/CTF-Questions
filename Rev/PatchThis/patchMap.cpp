#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>

// -----------------------------------------------------------------------------------------------------------------------------------------------
//                                                                      decrpytion function
// -----------------------------------------------------------------------------------------------------------------------------------------------

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
