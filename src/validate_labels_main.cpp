#include "evaluation.h"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void printUsage() {
    std::cerr << "Usage: chord_core_validate_labels --labels <path>\n";
}

std::string parseLabelPath(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--labels" && index + 1 < argc) {
            return argv[++index];
        }
    }

    throw std::invalid_argument("--labels is required");
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string labelPath = parseLabelPath(argc, argv);
        const std::vector<chord::ChordSpan> spans = chord::loadChordLabelCsv(labelPath);
        const chord::LabelCsvValidationSummary summary = chord::summarizeChordSpans(spans);

        std::cout << "Validated labels: " << labelPath << '\n';
        std::cout << "  spans=" << summary.spanCount << '\n';
        std::cout << "  labeled_duration=" << std::fixed << std::setprecision(3) << summary.labeledDurationSeconds
                  << "s\n";
        std::cout << "  first_start=" << summary.firstStartSeconds << "s\n";
        std::cout << "  last_end=" << summary.lastEndSeconds << "s\n";
        return 0;
    } catch (const std::exception& exception) {
        printUsage();
        std::cerr << "Label validation failed: " << exception.what() << '\n';
        return 1;
    }
}
