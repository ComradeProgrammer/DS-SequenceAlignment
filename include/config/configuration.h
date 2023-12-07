#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__
#include <string>

class Configuration {
   public:

    // "masterWaitTime", required for master,
    // master will wait for connections from slaves for this period of time
    // unit: second
    int master_wait_time = 0;

    // "columnBlockSize", required for the master
    int column_block_size_ = 0;
    // "rowBlockSize" required for the master
    int row_block_size_ = 0;

    // "sequenceRowType", required for the master, must be in
    // "fasta","text","string"
    std::string sequence_row_type_;
    // "sequenceRowDataSource", required
    // if "sequenceRowType" is fasta, then this is the location of fasta file
    // if "sequenceRowType" is text, then this is the location of text file
    // containing the sequence
    // if "sequenceRowType" is string, then this is the sequence string
    std::string sequence_row_data_source_;

    // "sequenceColumnType",required by the master, usage is the same with
    // "sequenceRowType"
    std::string sequence_column_type_;
    // "sequenceColumnDataSource",required by the master. usage is the same with
    // "sequenceRowDataSource"
    std::string sequence_column_data_source_;

    // "matchScore"
    int match_score_;
    // "mismatchPenalty"
    int mismatch_penalty_;
    // "gapOpen"
    int gap_open_;

   public:
    bool loadFromFile(std::string file_path);
};
#endif