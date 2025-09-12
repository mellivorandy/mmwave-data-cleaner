#include "DataCleaner.hpp"

int main() {
    PipelineConfig cfg;

    // File paths
    cfg.inputPath = "data/down-to-up.csv";
    cfg.outputCleanPath = "data/output_clean.csv";
    cfg.outputDroppedPath = "data/output_dropped.csv";
    
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
    cfg.excludeFromClean = {"gesturePresence"};
    
    // For debugging: print dropped rows to console
    cfg.printDroppedToStderr = true;

    DataCleaningPipeline pipeline(cfg);
    return pipeline.run();
}
