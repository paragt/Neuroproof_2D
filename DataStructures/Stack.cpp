
#include "Stack.h"

using namespace NeuroProof;
using namespace std;





Label find_max(Label* data, const size_t* dims){
    Label max=0;  	
    size_t plane_size = dims[1]*dims[2];
    size_t width = dims[2];	 	
    for(size_t i=0;i<dims[0];i++){
	for (size_t j=0;j<dims[1];j++){
	    for (size_t k=0;k<dims[2];k++){
		size_t curr_spot = i*plane_size + j * width + k;
		if (max<data[curr_spot])
		    max = data[curr_spot];	
	    }
	}
    }	
    return max;	
}

void Stack::compute_contingency_table(){

    if(!gtruth)
	return;		

    size_t i,j,k;
    const size_t dimn[]={depth-2*padding,height-2*padding,width-2*padding};
    Label *watershed_data = get_label_volume(); 		
	
    Label ws_max = find_max(watershed_data,dimn);  	

    contingency.clear();	
    //contingency.resize(ws_max+1);
    
	    /*C* debug
	    string debug_wshedname = "wshed_bytes.txt";
	    string debug_gtruthname = "gt_bytes.txt";
	    FILE* wfp= fopen(debug_wshedname.c_str(),"wt");
	    FILE* gfp= fopen(debug_gtruthname.c_str(),"wt");
	    // */
    unsigned long ws_size = dimn[0]*dimn[1]*dimn[2];	
    for(unsigned long itr=0; itr<ws_size ; itr++){
        Label wlabel = watershed_data[itr];
	Label glabel = gtruth[itr];
	
	    /*C* debug
		    fprintf(wfp, "%u ", wlabel);
		    fprintf(gfp, "%u ", glabel);
	    // */
	multimap<Label, vector<LabelCount> >::iterator mit;
 	mit = contingency.find(wlabel);
	if (mit != contingency.end()){
	    vector<LabelCount>& gt_vec = mit->second;
	    for (j=0; j< gt_vec.size();j++)
	        if (gt_vec[j].lbl == glabel){
		    (gt_vec[j].count)++;
		    break;
	  	}
	    if (j==gt_vec.size()){
	        LabelCount lc(glabel,1);
	        gt_vec.push_back(lc);	
	    }
	}
	else{
	    vector<LabelCount> gt_vec;	
	    gt_vec.push_back(LabelCount(glabel,1));	
	    contingency.insert(make_pair(wlabel, gt_vec));	
	}
    }		

	    /*C* debug
	    fclose(wfp);
	    fclose(gfp);
// 	    // */
    delete[] watershed_data;	
}
void Stack::compute_vi(){

    if(!gtruth)
	return;		

    int j, k;

    compute_contingency_table();

    double sum_all=0;

    int nn = contingency.size();
	
    multimap<Label, double>  wp;	 		
    multimap<Label, double>  gp;	 		

    for(multimap<Label, vector<LabelCount> >::iterator mit = contingency.begin(); mit != contingency.end(); ++mit){
	Label i = mit->first;
	vector<LabelCount>& gt_vec = mit->second; 	
	wp.insert(make_pair(i,0.0));
	for (j=0; j< gt_vec.size();j++){
	    unsigned int count = gt_vec[j].count;
	    Label gtlabel = gt_vec[j].lbl;
	
	    (wp.find(i)->second) += count;	

	    if(gp.find(gtlabel) != gp.end())
		(gp.find(gtlabel)->second) += count;
	    else
		gp.insert(make_pair(gtlabel,count));	

	    sum_all += count;
	}
	int tt=1;
    }

    double HgivenW=0;
    double HgivenG=0;
     				
    for(multimap<Label, vector<LabelCount> >::iterator mit = contingency.begin(); mit != contingency.end(); ++mit){
	Label i = mit->first;
	vector<LabelCount>& gt_vec = mit->second; 	
	for (j=0; j< gt_vec.size();j++){
	    unsigned int count = gt_vec[j].count;
	    Label gtlabel = gt_vec[j].lbl;
	    
 	    double p_wg = count/sum_all;
	    double p_w = wp.find(i)->second/sum_all; 	
	    
            HgivenW += p_wg* log(p_w/p_wg);

	    double p_g = gp.find(gtlabel)->second/sum_all;	

	    HgivenG += p_wg * log(p_g/p_wg);	

	}
    }

    printf("MergeSplit: (%f, %f)\n",HgivenW, HgivenG); 		
}


void Stack::compute_groundtruth_assignment(){

    if(!gtruth)
	return;		

    size_t j,k;
    const size_t dimn[]={depth-2*padding,height-2*padding,width-2*padding};

    Label gt_max = find_max(gtruth,dimn);  	

    compute_contingency_table();
		
    assignment.clear();  	
    for(multimap<Label, vector<LabelCount> >::iterator mit = contingency.begin(); mit != contingency.end(); ++mit){
	Label i = mit->first;
	vector<LabelCount>& gt_vec = mit->second; 	
	size_t max_count=0;
	Label max_label=0;
	for (j=0; j< gt_vec.size();j++){
	    if (gt_vec[j].count > max_count){
		max_count = gt_vec[j].count;
		max_label = gt_vec[j].lbl;
	    }	
	}
	assignment.insert(make_pair(i,max_label));
    }	


    printf("gt label determined for %d nodes\n", assignment.size());
}
void Stack::modify_assignment_after_merge(Label node_keep, Label node_remove){

    if(!gtruth)
	return;		
  	
    Label node_remove_gtlbl = assignment.find(node_remove)->second;
    size_t node_remove_gtoverlap = 0;
    size_t j;

    multimap<Label, vector<LabelCount> >::iterator mit; 
    mit = contingency.find(node_remove);	
    vector<LabelCount>& gt_vecr = mit->second; 	
    for (j=0; j < gt_vecr.size();j++){
	if (gt_vecr[j].lbl == node_remove_gtlbl){
	    node_remove_gtoverlap = gt_vecr[j].count;
	    break;
	}	
    }
    if (j == gt_vecr.size())
	printf("Something wrong with gt assignment of node remove %d\n",node_remove); 	


    mit = contingency.find(node_keep);	
    vector<LabelCount>& gt_veck = mit->second; 	

    for (j=0; j< gt_veck.size();j++){
	if (gt_veck[j].lbl == node_remove_gtlbl){
	    (gt_veck[j].count) += node_remove_gtoverlap;
	    break;
	}	
    }
    if (j == gt_veck.size()){ // i.e.,  a false merge
	//printf("Node keep %d does not overlap with node remove %d gt \n",node_keep, node_remove); 	
	
	LabelCount lc(node_remove_gtlbl,node_remove_gtoverlap);
	gt_veck.push_back(lc);
    }

    size_t max_count=0;
    Label max_label=0;
    for (j=0; j< gt_veck.size();j++){
	if (gt_veck[j].count > max_count){
	    max_count = gt_veck[j].count;
	    max_label = gt_veck[j].lbl;
	}	
    }
    assignment.find(node_keep)->second = max_label;


}





/*********************************************************************************/



void Stack::build_rag(std::map<Label, int> *mito_list)
{
    if (feature_mgr && (feature_mgr->get_num_features() == 0)) {
        feature_mgr->add_median_feature();
        median_mode = true; 
    } 
    
    unsigned int plane_size = width * height;
    std::vector<double> predictions(prediction_array.size(), 0.0);
    std::tr1::unordered_set<Label> labels;
    unsigned long long curr_spot;

    for (unsigned int z = padding; z < (depth-padding); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = padding; y < (height-padding); ++y) {
            int y_spot = y * width;
            for (unsigned int x = padding; x < (width-padding); ++x) {
                curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed[curr_spot];
		if (spot0==0)
		  continue;
		
                unsigned int spot1 = watershed[curr_spot-1];
                unsigned int spot2 = watershed[curr_spot+1];
                unsigned int spot3 = watershed[curr_spot-width];
                unsigned int spot4 = watershed[curr_spot+width];
                unsigned int spot5 = watershed[curr_spot-plane_size];
                unsigned int spot6 = watershed[curr_spot+plane_size];

                for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                    predictions[i] = prediction_array[i][curr_spot];
                }
                
                RagNode<Label> * node = rag->find_rag_node(spot0);
                if (!node) {
                    node = rag->insert_rag_node(spot0);
                }


                if (feature_mgr && !median_mode) {
                    feature_mgr->add_val(predictions, node);
                }
		   // if(gtruth) 
			//node->add_member_idx(gtruth, curr_spot);
		if (px_gtruth){
		    Label pxlbl = px_gtruth[curr_spot];
		    node->get_type_decider()->update(pxlbl); 
		}
		else
		    node->get_type_decider()->update(predictions); 


		// *C* point on the border with other superpixel
                if (spot1 && (spot0 != spot1)) {
                    rag_add_edge(rag, spot0, spot1, predictions, feature_mgr);
                    labels.insert(spot1);
                }
                if (spot2 && (spot0 != spot2) && (labels.find(spot2) == labels.end())) {
                    rag_add_edge(rag, spot0, spot2, predictions, feature_mgr);
                    labels.insert(spot2);
                }
                if (spot3 && (spot0 != spot3) && (labels.find(spot3) == labels.end())) {
                    rag_add_edge(rag, spot0, spot3, predictions, feature_mgr);
                    labels.insert(spot3);
                }
                if (spot4 && (spot0 != spot4) && (labels.find(spot4) == labels.end())) {
                    rag_add_edge(rag, spot0, spot4, predictions, feature_mgr);
                    labels.insert(spot4);
                }
//                 if (spot5 && (spot0 != spot5) && (labels.find(spot5) == labels.end())) {
//                     rag_add_edge(rag, spot0, spot5, predictions, feature_mgr);
//                     labels.insert(spot5);
//                 }
//                 if (spot6 && (spot0 != spot6) && (labels.find(spot6) == labels.end())) {
//                     rag_add_edge(rag, spot0, spot6, predictions, feature_mgr);
//                 }
           
                // point on volume border
//                 if (!spot1 || !spot2 || !spot3 || !spot4 || !spot5 || !spot6) {
//                     node->incr_border_size();
//                 }

                if (!spot1 || !spot2 || !spot3 || !spot4) {
                    node->incr_border_size();
                }



                labels.clear();

            }
        }
    } 

//     string debug_gtruthname = "mito_node_ids.txt";
//     FILE* gfp= fopen(debug_gtruthname.c_str(),"wt");
    if (merge_mito){
// 	bool return_mito_list = (mito_list != NULL)? true : false;
// 	if (return_mito_list)
// 	    mito_list->clear();
	
	printf("Labeling mito sps with thd= %lf\n", mito_thd);
	
    }
    watershed_to_body[0] = 0;
    unsigned int nmito_sp=0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label id = (*iter)->get_node_id();
        watershed_to_body[id] = id;
	if (merge_mito){
    // 	(*iter)->get_type_decider()->set_mito_pct_thd(mito_pct_thd);
	    (*iter)->get_type_decider()->set_type(mito_thd);
	    if ((*iter)->get_node_type()==2)
		nmito_sp++;
// 	    if (return_mito_list)
// 		mito_list->insert(std::make_pair(id, (*iter)->get_node_type()));
	}
// 	if ((*iter)->get_node_type() == 2)
// 	    fprintf(gfp, "%u\n", id);
	  
    }
    printf("total mito sps: %u\n", nmito_sp);
    
//     fclose(gfp);

//     unsigned int ecount = 0;
//     for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
// 	(*iter)->set_edge_id(ecount);
// 	ecount++;     
//     }
}

void Stack::create_nodes()
{
    unsigned int plane_size = width * height;
    std::vector<double> predictions(prediction_array.size(), 0.0);
    std::tr1::unordered_set<Label> labels;
    unsigned long long curr_spot;

    for (unsigned int z = padding; z < (depth-padding); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = padding; y < (height-padding); ++y) {
            int y_spot = y * width;
            for (unsigned int x = padding; x < (width-padding); ++x) {
                curr_spot = x + y_spot + z_spot;
//                 unsigned int spot0 = watershed[curr_spot];
//                 unsigned int spot1 = watershed[curr_spot-1];
//                 unsigned int spot2 = watershed[curr_spot+1];
//                 unsigned int spot3 = watershed[curr_spot-width];
//                 unsigned int spot4 = watershed[curr_spot+width];
//                 unsigned int spot5 = watershed[curr_spot-plane_size];
//                 unsigned int spot6 = watershed[curr_spot+plane_size];

//                 for (unsigned int i = 0; i < prediction_array.size(); ++i) {
//                     predictions[i] = prediction_array[i][curr_spot];
//                 }
                unsigned int spot0_next = watershed2[curr_spot];
                
//                 RagNode<Label> * node = rag->find_rag_node(spot0);
//                 if (!node) {
//                     node = rag->insert_rag_node(spot0);
// 		    node->set_size(1);
//                 }
// 		else
// 		    node->incr_size();
                RagNode<Label> * node_next = rag->find_rag_node(spot0_next);
                if (!node_next) {
                    node_next = rag->insert_rag_node(spot0_next);
		    node_next->set_size(1);
                }
                else 
		    node_next->incr_size();
//                 
//                 RagEdge<Label> *edge1 = rag->find_rag_edge(node, node_next); 
// 		if (!edge1)
// 		    rag->insert_rag_edge(node,node_next);
// 		else
// 		    edge1->incr_size();
// 



//                 labels.clear();
		

            }
        }
    } 

}
void Stack::link()
{
//     if (feature_mgr && (feature_mgr->get_num_features() == 0)) {
//         feature_mgr->add_median_feature();
//         median_mode = true; 
//     } 
    
    unsigned int plane_size = width * height;
    std::vector<double> predictions(prediction_array.size(), 0.0);
    std::tr1::unordered_set<Label> labels;
    unsigned long long curr_spot;

    for (unsigned int z = padding; z < (depth-padding); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = padding; y < (height-padding); ++y) {
            int y_spot = y * width;
            for (unsigned int x = padding; x < (width-padding); ++x) {
                curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed[curr_spot];
                unsigned int spot1 = watershed[curr_spot-1];
                unsigned int spot2 = watershed[curr_spot+1];
                unsigned int spot3 = watershed[curr_spot-width];
                unsigned int spot4 = watershed[curr_spot+width];
//                 unsigned int spot5 = watershed[curr_spot-plane_size];
//                 unsigned int spot6 = watershed[curr_spot+plane_size];

//                 for (unsigned int i = 0; i < prediction_array.size(); ++i) {
//                     predictions[i] = prediction_array[i][curr_spot];
//                 }
                unsigned int spot0_next = watershed2[curr_spot];
                
                RagNode<Label> * node = rag->find_rag_node(spot0);
                if (!node) {
		    printf("node does not exist, should not happen\n");
                }
                RagNode<Label> * node_next = rag->find_rag_node(spot0_next);
                if (!node_next) {
		    printf("node_next does not exist, should not happen\n");
                }
                
                RagEdge<Label> *edge1 = rag->find_rag_edge(node, node_next); 
		if (!edge1)
		    rag->insert_rag_edge(node,node_next);
		else
		    edge1->incr_size();

		

            }
        }
    } 

}



int Stack::remove_inclusions()
{
    int num_removed = 0;

    visited.clear();
    node_depth.clear();
    low_count.clear();
    prev_id.clear();
    biconnected_components.clear();
    stack.clear();

    RagNode<Label>* rag_node = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        bool border = (*iter)->is_border();

        if (border) {
            rag_node = *iter;
            break;
        }
    }
    assert(rag_node);

    DFSStack temp;
    temp.previous = 0;
    temp.rag_node = rag_node;
    temp.count = 1;
    temp.start_pos = 0;

    std::vector<DFSStack> dfs_stack;
    dfs_stack.push_back(temp);
    biconnected_dfs(dfs_stack);
    stack.clear();
    std::tr1::unordered_map<Label, Label> body_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history2; 

    // merge nodes in biconnected_components (ignore components with '0' node)
    for (int i = 0; i < biconnected_components.size(); ++i) {
        bool found_zero = false;
        std::tr1::unordered_set<Label> merge_nodes;
        for (int j = 0; j < biconnected_components[i].size()-1; ++j) {
            Label region1 = biconnected_components[i][j].region1;     
            Label region2 = biconnected_components[i][j].region2;
            if (!region1 || !region2) {
                found_zero = true;
            }
            
            if (body_to_body.find(region1) != body_to_body.end()) {
                region1 = body_to_body[region1];
            }
            if (body_to_body.find(region2) != body_to_body.end()) {
                region2 = body_to_body[region2];
            }

            assert(region1 != region2);

            merge_nodes.insert(region1);
            merge_nodes.insert(region2);
        }
        if (!found_zero) {
            Label articulation_region = biconnected_components[i][biconnected_components[i].size()-1].region1;
            unsigned long long total_size = 0;
            RagNode<Label>* articulation_node = rag->find_rag_node(articulation_region);
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region1 = *iter;
                RagNode<Label>* rag_node = rag->find_rag_node(region1);
                total_size += rag_node->get_size();

            }
          
            bool found_preserve = false; 
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region2 = *iter;
                if (body_to_body.find(region2) != body_to_body.end()) {
                    region2 = body_to_body[region2];
                }

                RagNode<Label>* rag_node = rag->find_rag_node(region2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    for (RagNode<Label>::edge_iterator edge_iter = rag_node->edge_begin();
                        edge_iter != rag_node->edge_end(); ++edge_iter) {
                        if ((*edge_iter)->is_preserve()) {
                            found_preserve = true;
                            break;
                        }
                    } 
                }

                if (found_preserve) {
                    break;
                }
            }

            if (found_preserve) {
                continue;
            }
            
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region2 = *iter;
                if (body_to_body.find(region2) != body_to_body.end()) {
                    region2 = body_to_body[region2];
                }

                RagNode<Label>* rag_node = rag->find_rag_node(region2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    feature_mgr->merge_features(articulation_node, rag_node); 
                    rag->remove_rag_node(rag_node); 

                    watershed_to_body[region2] = articulation_region;
                    for (std::vector<Label>::iterator iter2 = merge_history[region2].begin(); iter2 != merge_history[region2].end(); ++iter2) {
                        watershed_to_body[*iter2] = articulation_region;
                    }

                    body_to_body[region2] = articulation_region;
                    for (std::vector<Label>::iterator iter2 = merge_history2[region2].begin(); iter2 != merge_history2[region2].end(); ++iter2) {
                        body_to_body[*iter2] = articulation_region;
                    }

                    merge_history[articulation_region].push_back(region2);
                    merge_history[articulation_region].insert(merge_history[articulation_region].end(), merge_history[region2].begin(), merge_history[region2].end());
                    merge_history.erase(region2);
                   
                    merge_history2[articulation_region].push_back(region2); 
                    merge_history2[articulation_region].insert(merge_history2[articulation_region].end(), merge_history2[region2].begin(), merge_history2[region2].end());
                    merge_history2.erase(region2);
                }
            }
        }
    }

    
    visited.clear();
    node_depth.clear();
    low_count.clear();
    prev_id.clear();
    biconnected_components.clear();
    stack.clear();
    return num_removed;
}

// return low point (0 is default edge)
void Stack::biconnected_dfs(std::vector<DFSStack>& dfs_stack)
{
    while (!dfs_stack.empty()) {
        DFSStack entry = dfs_stack.back();
        RagNode<Label>* rag_node = entry.rag_node;
        Label previous = entry.previous;
        int count = entry.count;
        dfs_stack.pop_back();

        if (visited.find(rag_node->get_node_id()) == visited.end()) {
            visited.insert(rag_node->get_node_id());
            node_depth[rag_node->get_node_id()] = count;
            low_count[rag_node->get_node_id()] = count;
            prev_id[rag_node->get_node_id()] = previous;
        }

        bool skip = false;
        int curr_pos = 0;
        for (RagNode<Label>::node_iterator iter = rag_node->node_begin(); iter != rag_node->node_end(); ++iter) {
            RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node, *iter);
            if (rag_edge->is_false_edge()) {
                continue;
            }
            
            if (curr_pos < entry.start_pos) {
                ++curr_pos;
                continue;
            }
            if (prev_id[(*iter)->get_node_id()] == rag_node->get_node_id()) {
                OrderedPair<Label> current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                int temp_low = low_count[(*iter)->get_node_id()];
                low_count[rag_node->get_node_id()] = std::min(low_count[rag_node->get_node_id()], temp_low);

                if (temp_low >= count) {
                    OrderedPair<Label> popped_edge;
                    biconnected_components.push_back(std::vector<OrderedPair<Label> >());
                    do {
                        popped_edge = stack.back();
                        stack.pop_back();
                        biconnected_components[biconnected_components.size()-1].push_back(popped_edge);
                    } while (!(popped_edge == current_edge));
                    OrderedPair<Label> articulation_pair(rag_node->get_node_id(), rag_node->get_node_id());
                    biconnected_components[biconnected_components.size()-1].push_back(articulation_pair);
                } 
            } else if (visited.find((*iter)->get_node_id()) == visited.end()) {
                OrderedPair<Label> current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                stack.push_back(current_edge);

                DFSStack temp;
                temp.previous = rag_node->get_node_id();
                temp.rag_node = (*iter);
                temp.count = count+1;
                temp.start_pos = 0;
                entry.start_pos = curr_pos;
                dfs_stack.push_back(entry);
                dfs_stack.push_back(temp);
                skip = true;
                break;
            } else if ((*iter)->get_node_id() != previous) {
                low_count[rag_node->get_node_id()] = std::min(low_count[rag_node->get_node_id()], node_depth[(*iter)->get_node_id()]);
                if (count > node_depth[(*iter)->get_node_id()]) {
                    stack.push_back(OrderedPair<Label>(rag_node->get_node_id(), (*iter)->get_node_id()));
                }
            }
            ++curr_pos;
        }

        if (skip) {
            continue;
        }

        bool border = rag_node->is_border();
        if (previous && border) {
            low_count[rag_node->get_node_id()] = 0;
            stack.push_back(OrderedPair<Label>(0, rag_node->get_node_id()));
        }
    }
}

Label* Stack::get_label_volume(){

        Label * temp_labels = new Label[(width-(2*padding))*(height-(2*padding))*(depth-(2*padding))];
        Label * temp_labels_iter = temp_labels;
        unsigned int plane_size = width * height;

        for (int z = padding; z < (depth - padding); ++z) {
            int z_spot = z * plane_size;
            for (int y = padding; y < (height - padding); ++y) {
                int y_spot = y * width;
                for (int x = padding; x < (width - padding); ++x) {
                    unsigned long long curr_spot = x+y_spot+z_spot;
                    Label sv_id = watershed[curr_spot];
                    Label body_id;
                    if (!watershed_to_body.empty()) {
                        body_id = watershed_to_body[sv_id];
                    } else {
                        body_id = sv_id;
                    }
                    *temp_labels_iter = body_id;
                    ++temp_labels_iter;
                }
            }
        }
    
        return temp_labels;
}



/*




void Stack::determine_edge_locations(){
        best_edge_z.clear();
        best_edge_loc.clear();

        unsigned int plane_size = width * height;

        for (unsigned int z = 1; z < (depth-1); ++z) {
            int z_spot = z * plane_size;

            EdgeCount curr_edge_z;
            EdgeLoc curr_edge_loc;
             

            for (unsigned int y = 1; y < (height-1); ++y) {
                int y_spot = y * width;
                for (unsigned int x = 1; x < (width-1); ++x) {
                    unsigned long long curr_spot = x + y_spot + z_spot;
                    unsigned int spot0 = watershed_to_body[(watershed[curr_spot])];
                    unsigned int spot1 = watershed_to_body[(watershed[curr_spot-1])];
                    unsigned int spot2 = watershed_to_body[(watershed[curr_spot+1])];
                    unsigned int spot3 = watershed_to_body[(watershed[curr_spot-width])];
                    unsigned int spot4 = watershed_to_body[(watershed[curr_spot+width])];
                    unsigned int spot5 = watershed_to_body[(watershed[curr_spot-plane_size])];
                    unsigned int spot6 = watershed_to_body[(watershed[curr_spot+plane_size])];

                    if (spot1 && (spot0 != spot1)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot1);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot2 && (spot0 != spot2)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot2);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot3 && (spot0 != spot3)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot3);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot4 && (spot0 != spot4)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot4);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot5 && (spot0 != spot5)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot5);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot6 && (spot0 != spot6)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot6);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                }
            }
        
            for (EdgeCount::iterator iter = curr_edge_z.begin(); iter != curr_edge_z.end(); ++iter) {
                if (iter->second > best_edge_z[iter->first]) {
                    best_edge_z[iter->first] = iter->second;
                    best_edge_loc[iter->first] = curr_edge_loc[iter->first];
                }
            }
        } 
        
        
        return;
}

bool Stack::add_edge_constraint(boost::python::tuple loc1, boost::python::tuple loc2) {
        Label body1 = get_body_id(boost::python::extract<int>(loc1[0]), boost::python::extract<int>(loc1[1]),
                                boost::python::extract<int>(loc1[2]));    
        Label body2 = get_body_id(boost::python::extract<int>(loc2[0]), boost::python::extract<int>(loc2[1]),
                                boost::python::extract<int>(loc2[2]));    

        if (body1 == body2) {
            return false;
        }

        RagEdge<Label>* edge = rag->find_rag_edge(body1, body2);

        if (!edge) {
            RagNode<Label>* node1 = rag->find_rag_node(body1);
            RagNode<Label>* node2 = rag->find_rag_node(body2);
            edge = rag->insert_rag_edge(node1, node2);
            edge->set_false_edge(true);
        }
        edge->set_preserve(true);

        return true;
}
*/
Label Stack::get_body_id(unsigned int x, unsigned int y, unsigned int z) {
        x += padding;
        //y += padding;
        y = height - y - 1 - padding;
        z += padding;
        unsigned int plane_size = width * height;
        unsigned long long curr_spot = x + y * width + z * plane_size; 
        Label body_id = watershed[curr_spot];
        if (!watershed_to_body.empty()) {
            body_id = watershed_to_body[body_id];
        }
        return body_id;
}

/*boost::python::tuple Stack::get_edge_loc(RagEdge<Label>* edge) {
        if (best_edge_loc.find(edge) == best_edge_loc.end()) {
            throw ErrMsg("Edge location was not loaded!");
        }
        Location loc = best_edge_loc[edge];
        unsigned int x = boost::get<0>(loc) - padding;
        unsigned int y = boost::get<1>(loc) - padding; //height - boost::get<1>(loc) - 1 - padding;
        unsigned int z = boost::get<2>(loc) - padding;

        return boost::python::make_tuple(x, y, z); 
}

void Stack::get_edge_loc(RagEdge<Label>* edge, Label&x, Label& y, Label& z) {
        if (best_edge_loc.find(edge) == best_edge_loc.end()) {
            throw ErrMsg("Edge location was not loaded!");
        }
        Location loc = best_edge_loc[edge];
        x = boost::get<0>(loc) - padding;
        y = boost::get<1>(loc) - padding; //height - boost::get<1>(loc) - 1 - padding;
        z = boost::get<2>(loc) - padding;

}

void Stack::write_graph(string output_path){
  
    unsigned found = output_path.find_last_of(".");
    string dirname = output_path.substr(0,found);
    string filename = dirname + string("_edge_list.txt");
    
    FILE *fp = fopen(filename.c_str(), "wt");
    if(!fp){
        printf("%s could not be opened \n", filename.c_str());
	return;
    }
    fprintf(fp,"Nodes:\n");
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
	fprintf(fp,"%u %lu %u\n", (*iter)->get_node_id(), (*iter)->get_size(), 
		  (is_orphan(*iter)? 1 : 0));
    }
    fprintf(fp,"\nEdges:\n");
    
    Label x, y, z;
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        get_edge_loc(*iter, x, y, z); 
        fprintf(fp,"%u %u %lu %lu %u %u %lf %u %u %u\n", (*iter)->get_node1()->get_node_id(), (*iter)->get_node2()->get_node_id(),
	      (*iter)->get_node1()->get_size(), (*iter)->get_node2()->get_size(),
	      (*iter)->is_preserve()? 1:0, (*iter)->is_false_edge()? 1: 0,
	      (*iter)->get_weight(), x, y, z);
    }
    fclose(fp);
    
}*/





int Stack::decide_edge_label(RagNode<Label>* rag_node1, RagNode<Label>* rag_node2){
    int edge_label = 0;
    Label node1 = rag_node1->get_node_id();	
    Label node2 = rag_node2->get_node_id();	

    Label node1gt = assignment.find(node1)->second;
    Label node2gt = assignment.find(node2)->second;

    if (!node1gt){
	printf("no grountruth node %d\n",node1);
	return 0;
    }  	
    if (!node2gt){
	printf("no grountruth node %d\n",node2);
	return 0;
    }  	

    edge_label = (node1gt == node2gt)? -1 : 1;

    if(merge_mito){
	if ( rag_node1->get_node_type() == 2 && rag_node2->get_node_type() == 2 ){
	    return 0;
	}
	else if ( rag_node1->get_node_type() == 2 || rag_node2->get_node_type() == 2 )
	    edge_label = 1;				
	
// 	if ( rag_node1->get_node_type() == 2 || rag_node2->get_node_type() == 2 ){
// 	    if (rag_node1->get_node_type() == 2 && rag_node2->get_node_type() == 2)
// 		edge_label = 0;
// 	    else
// 		edge_label = 1;				
// 	}
    }
    return edge_label;	
}





void Stack::set_basic_features(){

    double hist_percentiles[]={0.1, 0.3, 0.5, 0.7, 0.9};
    std::vector<double> percentiles(hist_percentiles, hist_percentiles+sizeof(hist_percentiles)/sizeof(double));		

    // ** for Toufiq's version ** feature_mgr->add_inclusiveness_feature(true);  	
    feature_mgr->add_moment_feature(4,true);	
    feature_mgr->add_hist_feature(25,percentiles,false); 	

    //cout << "Number of features, channels:" << feature_mgr->get_num_features()<< ","<<feature_mgr->get_num_channels()<<"\n";	

}









