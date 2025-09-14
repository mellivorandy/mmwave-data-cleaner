#include "DataCleaner.hpp"

std::string getArg(int argc, char* argv[], int idx, const std::string& def) {
    return (argc > idx) ? argv[idx] : def;
}

int main(int argc, char* argv[]) {
    PipelineConfig cfg;

    // Default file paths
    std::string defaultInputPath = "data/down-to-up.csv";
    std::string defaultCleanPath = "data/output_clean.csv";
    std::string defaultDroppedPath = "data/output_dropped.csv";

    // Set file paths
    cfg.inputPath = getArg(argc, argv, 1, defaultInputPath);
    cfg.outputCleanPath = getArg(argc, argv, 2, defaultCleanPath);
    cfg.outputDroppedPath = getArg(argc, argv, 3, defaultDroppedPath);

    // Columns to keep
    cfg.keepColumns = {
        "timestamp", "frameNum", "error", "gesturePresence", "gesture",
        "gestureFeatures_0", "gestureFeatures_1", "gestureFeatures_2", "gestureFeatures_3",
        "gestureFeatures_4", "gestureFeatures_5", "gestureFeatures_6", "gestureFeatures_7",
        "gestureFeatures_8", "gestureFeatures_9", "gestureFeatures_10", "gestureFeatures_11",
        "gestureFeatures_12", "gestureFeatures_13", "gestureFeatures_14", "gestureFeatures_15"};
    
    // Columns for filtering
    cfg.gesturePresenceCol = "gesturePresence";
    cfg.frameNumCol = "frameNum";
    cfg.gestureCol = "gesture";
    cfg.excludeFromClean = {"gesturePresence"};
    
    // For debugging: print dropped rows to console
    cfg.printDroppedToStderr = true;

    if (argc == 1) {
        std::cout << "\nUsing default file paths.\n";
        std::cout << "\nTo customize: " << argv[0] << " [input.csv] [output_clean.csv] [output_dropped.csv]\n";
    }

    DataCleaningPipeline pipeline(cfg);
    return pipeline.run();
}
