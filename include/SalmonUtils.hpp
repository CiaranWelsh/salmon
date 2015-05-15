#ifndef __SALMON_UTILS_HPP__
#define __SALMON_UTILS_HPP__

extern "C" {
#include "io_lib/scram.h"
#include "io_lib/os.h"
#undef min
#undef max
}

#include <algorithm>
#include <iostream>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "format.h"

#include "SalmonOpts.hpp"

#include "LibraryFormat.hpp"
#include "ReadLibrary.hpp"
#include "TranscriptGeneMap.hpp"
#include "GenomicFeature.hpp"

class ReadExperiment;
class LibraryFormat;

namespace salmon{
namespace utils {

using std::string;
using NameVector = std::vector<string>;
using IndexVector = std::vector<size_t>;
using KmerVector = std::vector<uint64_t>;

// Returns a uint64_t where the upper 32-bits
// contain tid and the lower 32-bits contain offset
uint64_t encode(uint64_t tid, uint64_t offset);

// Given a uin64_t generated by encode(), return the
// transcript id --- upper 32-bits
uint32_t transcript(uint64_t enc);

// Given a uin64_t generated by encode(), return the
// offset --- lower 32-bits
uint32_t offset(uint64_t enc);


LibraryFormat parseLibraryFormatStringNew(std::string& fmt);

std::vector<ReadLibrary> extractReadLibraries(boost::program_options::parsed_options& orderedOptions);

LibraryFormat parseLibraryFormatString(std::string& fmt);

size_t numberOfReadsInFastaFile(const std::string& fname);

bool readKmerOrder( const std::string& fname, std::vector<uint64_t>& kmers );

template <template<typename> class S, typename T>
bool overlap( const S<T> &a, const S<T> &b );

template< typename T >
TranscriptGeneMap transcriptToGeneMapFromFeatures( std::vector<GenomicFeature<T>> &feats ) {
    using std::unordered_set;
    using std::unordered_map;
    using std::vector;
    using std::tuple;
    using std::string;
    using std::get;

    using NameID = tuple<string, size_t>;

    IndexVector t2g;
    NameVector transcriptNames;
    NameVector geneNames;

    // holds the mapping from transcript ID to gene ID
    IndexVector t2gUnordered;
    // holds the set of gene IDs
    unordered_map<string, size_t> geneNameToID;

    // To read the input and assign ids
    size_t geneCounter = 0;
    string transcript;
    string gene;

    std::sort( feats.begin(), feats.end(),
    []( const GenomicFeature<T> & a, const GenomicFeature<T> & b) -> bool {
        return a.sattr.transcript_id < b.sattr.transcript_id;
    } );

    std::string currentTranscript = "";
    for ( auto & feat : feats ) {

        auto &gene = feat.sattr.gene_id;
        auto &transcript = feat.sattr.transcript_id;

        if ( transcript != currentTranscript ) {
            auto geneIt = geneNameToID.find(gene);
            size_t geneID = 0;

            if ( geneIt == geneNameToID.end() ) {
                // If we haven't seen this gene yet, give it a new ID
                geneNameToID[gene] = geneCounter;
                geneID = geneCounter;
                geneNames.push_back(gene);
                ++geneCounter;
            } else {
                // Otherwise lookup the ID
                geneID = geneIt->second;
            }

            transcriptNames.push_back(transcript);
            t2g.push_back(geneID);

            //++transcriptID;
            currentTranscript = transcript;
        }

    }

    return TranscriptGeneMap(transcriptNames, geneNames, t2g);
}

TranscriptGeneMap transcriptGeneMapFromGTF(const std::string& fname, std::string key="gene_id");

TranscriptGeneMap readTranscriptToGeneMap( std::ifstream &ifile );

TranscriptGeneMap transcriptToGeneMapFromFasta( const std::string& transcriptsFile );

void aggregateEstimatesToGeneLevel(TranscriptGeneMap& tgm, boost::filesystem::path& inputPath);

// NOTE: Throws an invalid_argument exception of the quant or quant_bias_corrected files do
// not exist!
void generateGeneLevelEstimates(boost::filesystem::path& geneMapPath,
                                boost::filesystem::path& estDir,
                                bool haveBiasCorrectedFile = false);

    enum class OrphanStatus: uint8_t { LeftOrphan = 0, RightOrphan = 1, Paired = 2 };

    bool headersAreConsistent(SAM_hdr* h1, SAM_hdr* h2);

    bool headersAreConsistent(std::vector<SAM_hdr*>&& headers);

    template <typename AlnLibT>
    void writeAbundances(const SalmonOpts& sopt,
                         AlnLibT& alnLib,
                         boost::filesystem::path& fname,
                         std::string headerComments="");

    double logAlignFormatProb(const LibraryFormat observed, const LibraryFormat expected, double incompatPrior);

    std::ostream& operator<<(std::ostream& os, OrphanStatus s);
    /**
    *  Given the information about the position and strand from which a paired-end
    *  read originated, return the library format with which it is compatible.
    */
    LibraryFormat hitType(int32_t end1Start, bool end1Fwd,
                          int32_t end2Start, bool end2Fwd);
    LibraryFormat hitType(int32_t end1Start, bool end1Fwd, uint32_t len1,
                          int32_t end2Start, bool end2Fwd, uint32_t len2, bool canDovetail);
    /**
    *  Given the information about the position and strand from which the
    *  single-end read originated, return the library format with which it
    *  is compatible.
    */
    LibraryFormat hitType(int32_t readStart, bool isForward);

}
}

#endif // __SALMON_UTILS_HPP__
