#include "json_reader.h"

#include <iostream>
#include <fstream>


using namespace transportcatalogue;

using namespace std;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

//int main(int argc, char* argv[]) {
//    if (argc != 2) {
//        PrintUsage();
//        return 1;
//    }
//
//    const std::string_view mode(argv[1]);
//
//    if (mode == "make_base"sv) {
//        std::ifstream ifile("make_base.json");
//        if (!ifile) {
//            std::cout << "Input file opened incorrectly"s << std::endl;
//            return 2;
//        }
//
//        creator::Creator creator;
//        creator.InitializeCatalogue(ifile);
//    }
//    else if (mode == "process_requests"sv) {
//        std::ifstream ifile("input.json");
//        if (!ifile) {
//            std::cout << "Input file opened incorrectly"s << std::endl;
//            return 2;
//        }
//
//        std::ofstream ofile("output.json");
//        if (!ifile) {
//            std::cout << "Input file opened incorrectly"s << std::endl;
//            return 2;
//        }
//
//
//        creator::Creator creator;
//        creator.ExecutingRequests(ifile, ofile);
//    }
//    else {
//        PrintUsage();
//        return 1;
//    }
//}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        creator::Creator creator;
        creator.InitializeCatalogue(std::cin);

    }
    else if (mode == "process_requests"sv) {
        creator::Creator creator;
        creator.ExecutingRequests(std::cin, std::cout);
    }
    else {
        PrintUsage();
        return 1;
    }
}