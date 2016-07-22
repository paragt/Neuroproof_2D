#ifndef FEATURECACHE_H
#define FEATURECACHE_H

#include <iosfwd>
#include <vector>
#include <set>

namespace NeuroProof {

struct CountCache {
    CountCache() : count(0) {}
    signed long long count; 
    ~CountCache(){};	
};

struct MomentCache {
    MomentCache(unsigned int num_moments) : count(0), vals(num_moments, 0) {}
    unsigned long long count; 
    std::vector<double> vals;
    ~MomentCache(){
	if (vals.size()>0)  	
	    vals.clear();
    };	
};

struct HistCache {
    HistCache(unsigned num_bins) : count(0), hist(num_bins, 0) {}
    unsigned long long count;
    std::vector<unsigned long long> hist;
    ~HistCache(){
	if (hist.size()>0)  
	    hist.clear();
    };	
};

}

#endif


