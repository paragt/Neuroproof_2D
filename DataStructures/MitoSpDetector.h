#ifndef _MITO_SP_DETECTOR
#define _MITO_SP_DETECTOR

#define MEAN_PROB 1
#define COUNT_TYPE 2

#include <vector>
#include <map>

#include "../DataStructures/Stack.h"
#include "../Utilities/h5write.h"
#include "../Utilities/string_functions.h"


template <typename Region>
class MitoSpDetector {

    Rag<Region> *rag;
    FeatureMgr *feature_mgr;
//     UniqueRowFeature_Label  all_features;  
    
    EdgeClassifier* mclfr;	 
    std::set<Region> mito_sp_list;
    
//     Region node_type;
//     std::multimap< Region, unsigned long long > type_list;
//     double sum_mitop;	
//     double sum_cytop;	
// //     double MITO_PCT_THD;
//     double npixels;	
//     int method;    
 
public:
    MitoSpDetector(){
    }
    
    void add_features(UniqueRowFeature_Label &all_features);
    double predict(std::string classifier_filename, double thd);
    void set_var(Rag<Region> *prag, FeatureMgr* pfmgr){
	rag = prag;
	feature_mgr = pfmgr;
      
    }
    double save(Region* tmp_label_volume1D, size_t depth, size_t height, size_t width, std::string imagename);
    
    
//     NodeType() : sum_mitop(0), sum_cytop(0), npixels(0), node_type(0), method(MEAN_PROB){ 
//       type_list.clear();
// //       MITO_PCT_THD = 0.2;
//     };	
// 
//     void update(std::vector<double>& predictions);	
//     void update(Region plbl);	
//     void set_type(double );	
// //     void set_mito_pct_thd(double pthd){MITO_PCT_THD = pthd;};	
//     void set_type(Region ptype){node_type=ptype;};	
//     Region get_node_type() {return node_type;};
//     //void add_mitop(double mp){};	

};

template<typename Region> void MitoSpDetector<Region>::add_features(UniqueRowFeature_Label &all_features) 
{
  
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Region id = (*iter)->get_node_id();
	
	std::vector<double> feature;
	feature_mgr->compute_node_features((*iter),feature);

	feature.erase(feature.begin());
	std::vector<double> feature_nbr(feature.size(),0);
	double nnbrs=0;
	RagNode<Region>* rag_node1=(*iter);
	for(typename RagNode<Region>::edge_iterator eiter = rag_node1->edge_begin(); eiter != rag_node1->edge_end(); ++eiter) {
	    RagNode<Region>* other_node = (*eiter)->get_other_node(rag_node1);
	    
	    std::vector<double> feature2;
	    feature_mgr->compute_node_features(other_node,feature2);
	    feature2.erase(feature2.begin());
	    for(size_t i=0; i<feature_nbr.size(); i++)
		feature_nbr[i] = feature_nbr[i] + feature2[i];
	    nnbrs++;
	}
	for(size_t i=0; i<feature_nbr.size(); i++)
	    feature_nbr[i] /= (nnbrs);
	feature.insert(feature.end(), feature_nbr.begin(),feature_nbr.end());
	
	int mito_label = ((*iter)->get_node_type()==2)? 1:-1;

	feature.push_back(mito_label);
	all_features.insert(feature);
    }
    printf("number of features added: %u\n", all_features.nrows());

}
template<typename Region> double MitoSpDetector<Region>::save(Region* temp_label_volume1D, size_t depth, size_t height, size_t width,
							      std::string output_filename)
{
  
	hsize_t dims_out[3];
	for(size_t ii=0; ii < depth*height*width; ii++){
	    if (mito_sp_list.find(temp_label_volume1D[ii]) == mito_sp_list.end())
		temp_label_volume1D[ii] = 0;
	}
// 	string output_filename = "";
// 	output_filename += imagename;
// 	output_filename += "_mitolbl.h5";
	
	dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;   
	H5Write(output_filename.c_str(),"stack",3,dims_out, temp_label_volume1D);
	printf("Output-nomito written to %s, dataset %s\n",output_filename.c_str(), "stack");	
	
}


template<typename Region> double MitoSpDetector<Region>::predict(std::string classifier_filename, double thd) {
    
    if (endswith(classifier_filename, "h5"))
	mclfr = new VigraRFclassifier(classifier_filename.c_str());	
    else if (endswith(classifier_filename, "xml")) 	
	mclfr = new OpencvRFclassifier(classifier_filename.c_str());	
//     mclfr = new VigraRFclassifier(classifier_filename.c_str());
    
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Region id = (*iter)->get_node_id();
	
	std::vector<double> feature;
	feature_mgr->compute_node_features((*iter),feature);
	
	feature.erase(feature.begin());
	std::vector<double> feature_nbr(feature.size(),0);
	double nnbrs=0;
	RagNode<Region>* rag_node1=(*iter);
	for(typename RagNode<Region>::edge_iterator eiter = rag_node1->edge_begin(); eiter != rag_node1->edge_end(); ++eiter) {
	    RagNode<Region>* other_node = (*eiter)->get_other_node(rag_node1);
	    
	    std::vector<double> feature2;
	    feature_mgr->compute_node_features(other_node,feature2);
	    feature2.erase(feature2.begin());
	    for(size_t i=0; i<feature_nbr.size(); i++)
		feature_nbr[i] = feature_nbr[i] + feature2[i];
	    nnbrs++;
	}
	for(size_t i=0; i<feature_nbr.size(); i++)
	    feature_nbr[i] /= (nnbrs);
	feature.insert(feature.end(), feature_nbr.begin(),feature_nbr.end());
	
	
	double prob = mclfr->predict(feature);
	
	if (prob>thd){
	  (*iter)->get_type_decider()->set_node_type(2);
	  mito_sp_list.insert(id);
	}
	else
	  (*iter)->get_type_decider()->set_node_type(1);
	  
    }
    
}

#endif
