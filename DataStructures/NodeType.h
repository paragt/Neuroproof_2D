#ifndef _NODE_TYPE
#define _NODE_TYPE

#define MEAN_PROB 1
#define COUNT_TYPE 2

#include <vector>
#include <map>


template <typename Region>
class NodeType {

    Region node_type;
    std::multimap< Region, unsigned long long > type_list;
    double sum_mitop;	
    double sum_cytop;	
//     double MITO_PCT_THD;
    double npixels;	
    int method;    
 
public:
    NodeType() : sum_mitop(0), sum_cytop(0), npixels(0), node_type(0), method(MEAN_PROB){ 
      type_list.clear();
//       MITO_PCT_THD = 0.2;
    };	

    void update(std::vector<double>& predictions);	
    void update(Region plbl);	
    void set_type(double );	
//     void set_mito_pct_thd(double pthd){MITO_PCT_THD = pthd;};	
    void set_node_type(Region ptype){node_type=ptype;};	
    Region get_node_type() {return node_type;};
    //void add_mitop(double mp){};	

};


template<typename Region> void NodeType<Region>::update(std::vector<double>& predictions) {

    double mitop = predictions[2];
    double cytop = predictions[1];
    double bdryp = predictions[0];	 	

    if (method == MEAN_PROB){
	sum_mitop += mitop; 
	sum_cytop += cytop; 
	npixels++;
    }
    else if (method == COUNT_TYPE){
	Region ntype = 0;
        if ( mitop > bdryp && mitop > 2*cytop) // p(mito) > 2*p(cyto)
	    ntype = 2;
	else //if ( cytop > bdryp)	
	    ntype = 1;

    	typename std::multimap< Region, unsigned long long >::iterator type_itr = type_list.find(ntype);

    //node_itr = type_list.find(ntype);	

    	if(type_itr == type_list.end()){
	    type_list.insert(make_pair(ntype,1));
    	} 	
    	else{
	    (type_itr->second) += 1;	
    	}	
    }
}

template<typename Region> void NodeType<Region>::update(Region plbl) {

    sum_mitop += ((plbl == 2)? 1: 0);  
    npixels++;
}

template<typename Region> void  NodeType<Region>::set_type(double mito_thd) {
 
    if (method == MEAN_PROB){	
   	double mito_pct = sum_mitop/npixels;	
   	double cyto_pct = sum_cytop/npixels;	
	if(mito_pct > 1.0){
           printf("somephing wrong with mito_ratio %d\n", node_type);
	    node_type = 0;	
    	}
			
//    	if (mito_pct >= 0.55)	
   	if (mito_pct >= mito_thd)//0.175 test, 0.2 trn
	    node_type = 2;
   	else
	    node_type = 1;	

    }
    else if (method == COUNT_TYPE){
    	typename std::multimap< Region, unsigned long long >::iterator type_itr;
    	Region maxidx=0;
	unsigned long long maxt=0;	  	
    	for(type_itr=type_list.begin(); type_itr!=type_list.end(); type_itr++){
	    if (type_itr->second > maxt){	
	    	maxt = type_itr->second;
	    	maxidx = type_itr->first;	
	    }
    	}
    	if (maxidx==0) 	
	    printf("type not found, node %d\n",node_type);
    	node_type = maxidx;
    }
}
 
#endif
