#include "DataStructures/Stack.h"
#include "DataStructures/MitoSpDetector.h"
// #include "Priority/GPR.h"
// #include "Priority/LocalEdgePriority.h"
// #include "Utilities/ScopeTime.h"
// #include "ImportsExports/ImportExportRagPriority.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <memory>
// #include <json/json.h>
// #include <json/value.h>

#include "Utilities/h5read.h"
#include "Utilities/h5write.h"
#include "Utilities/folder.h"

#include <ctime>
#include <cmath>
#include <cstring>



#define REMOVE_INCLUSION 0




using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using namespace NeuroProof;
using std::vector;

template <class T>
void padZero(T* data, const size_t* dims, int pad_length, T** ppadded_data){
     
     // implemented only for 3D arrays

     unsigned long int newsize = (dims[0]+2*pad_length)*(dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
     *ppadded_data = new T[newsize];  	
     T* padded_data = *ppadded_data;
	
     memset((void*) padded_data, 0, sizeof(T)*newsize);

     unsigned int width, plane_size, width0, plane_size0, i0,j0,k0, i,j,k;
     
     for (i=pad_length, i0=0; i<= dims[0] ; i++, i0++)
	 for (j=pad_length, j0=0; j<= dims[1]; j++, j0++)	
	     for(k=pad_length, k0=0; k<= dims[2]; k++, k0++){
		 plane_size = (dims[1]+2*pad_length)*(dims[2]+2*pad_length);	
		 width = dims[2]+2*pad_length;	

		 plane_size0 = dims[1]*dims[2];	
		 width0 = dims[2];	

		 padded_data[i*plane_size+ j*width + k] = data[i0*plane_size0 + j0*width0 + k0];	
	     }
			

}

// bool endswith(string filename, string extn){
//     unsigned found = filename.find_last_of(".");
//     string fextn = filename.substr(found);	
//     if (fextn.compare(extn) == 0 )
// 	return true;
//     else return false;	  
// }


int main(int argc, char** argv) 
{
    int          i, j, k;


    cout<< "Reading data ..." <<endl;



    if (argc<8){
	printf("format: NeuroProof_stack -image_dir image_directory\
					 -classifier classifier_file \
					 -threshold agglomeration_threshold \
					 -algorithm algorithm_type\n");
	return 0;
    }	

    int argc_itr=1;	
    string watershed_filename="";
    string watershed_dataset_name="";		
    string prediction_filename="";
    string prediction_dataset_name="";		
    string output_filename;
    string output_dataset_name="stack";		
    string groundtruth_filename="";
    string groundtruth_dataset_name="";		
    string classifier_filename;
    string mito_classifier_filename="";

    string output_filename_nomito, threshold_str;
    string image_dir;
    string mito_det_dir;

    	
    double threshold = 0.2;	
    int agglo_type = 1;		
    bool merge_mito = true;
    bool merge_mito_by_chull = false;
    bool read_off_rwts = false;
    double mito_thd=0.3;
    size_t min_region_sz=100;

    while(argc_itr<argc){
	if (!(strcmp(argv[argc_itr],"-image_dir"))){
	    image_dir = argv[++argc_itr];
	}

	if (!(strcmp(argv[argc_itr],"-classifier"))){
	    classifier_filename = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-mito_det_dir"))){
	    mito_det_dir = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-threshold"))){
	    threshold_str = argv[++argc_itr];
	    threshold = atof(threshold_str.c_str());
	}
	if (!(strcmp(argv[argc_itr],"-algorithm"))){
	    agglo_type = atoi(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-nomito"))){
	    merge_mito = false;
	}
	if (!(strcmp(argv[argc_itr],"-mito_classifier"))){
	    mito_classifier_filename = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-mito_thd"))){
	    mito_thd = atof(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-min_region_sz"))){
	    min_region_sz = atoi(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-read_off"))){
	    if (agglo_type==2)
		read_off_rwts = true;
	}
        ++argc_itr;
    } 	
    	

    EdgeClassifier* eclfr;
    if (endswith(classifier_filename, "h5"))
    	eclfr = new VigraRFclassifier(classifier_filename.c_str());	
    else if (endswith(classifier_filename, "xml")) 	
	eclfr = new OpencvRFclassifier(classifier_filename.c_str());	
    
    
    time_t start, end;
    time(&start);	
    
    size_t img_count=0;
    Folder datadir(image_dir);
    string imagepath, imagename, mitolist_name, global_output_filename;
    while (datadir.get_next(imagepath, prediction_filename, watershed_filename))
    {
	printf("image %d: %s\nwatershed: %s\nprediction :%s\n", ++img_count, imagepath.c_str(),
	  watershed_filename.c_str(), prediction_filename.c_str());
	
	
// 	output_filename = image_dir+"/output/";
// 	imagename = imagepath;
	size_t ofn = imagepath.find_last_of(".");
	size_t bfn = imagepath.find_last_of("/");
	size_t nchar = ofn-(bfn+1);
	imagename = imagepath.substr(bfn+1,nchar);
	output_filename = image_dir + "/output/"+imagename+"_result_thd"+threshold_str+"_m4.h5";
	global_output_filename = image_dir + "/output/"+imagename+"_global_result_thd"+threshold_str+"_m4.h5";
	mitolist_name = image_dir + "/output/"+imagename+"_mitolist.txt";
// 	output_filename += "_result_m4.h5";
	if (agglo_type ==4)
	    output_filename = global_output_filename;
	output_filename_nomito = output_filename;
	ofn = output_filename_nomito.find_last_of(".");
	output_filename_nomito.erase(ofn-1,1);			

	H5Read watershed(watershed_filename.c_str(), "stack");	
	Label* watershed_data=NULL;	
	watershed.readData(&watershed_data);	
	int depth, width, height; 
	if (watershed.total_dim()==3){
	    depth = watershed.dim()[0];
	    height = watershed.dim()[1];
	    width =	 watershed.dim()[2];
	}
	else if (watershed.total_dim() == 2){
	    depth = 1;
	    height = watershed.dim()[0];
	    width =	 watershed.dim()[1];
	}

	size_t data_dim[3];
	data_dim[0] = depth; data_dim[1] = height; data_dim[2] = width;

	    
	int pad_len=1;
	Label* px_groundtruth_data=NULL;
	Label *zp_px_groundtruth_data=NULL;

	if (mito_det_dir.size()>0){
	    string imgname_noextn = imagename;
// 	    imgname_noextn.erase(0, image_dir.size());
// 	    imgname_noextn.erase(imgname_noextn.size()-4, 4);
	    string px_groundtruth_filename= mito_det_dir;
	    px_groundtruth_filename += imgname_noextn;
	    px_groundtruth_filename += "_mito_pred.h5";
	    string px_groundtruth_dataset_name="stack";		
	    
	    printf("%s\n",px_groundtruth_filename.c_str());
	    
	    H5Read px_groundtruth(px_groundtruth_filename.c_str(), "stack", true);	
	    px_groundtruth.readData(&px_groundtruth_data);	
	    
	    padZero(px_groundtruth_data, data_dim,pad_len,&zp_px_groundtruth_data);	
	}
	
	Label *zp_watershed_data=NULL;
	padZero(watershed_data, data_dim,pad_len,&zp_watershed_data);	


	StackPredict* stackp = new StackPredict(zp_watershed_data, depth+2*pad_len, height+2*pad_len, width+2*pad_len, pad_len);
	stackp->set_feature_mgr(new FeatureMgr());
	stackp->set_merge_mito(merge_mito, mito_thd);
	    

	H5Read prediction(prediction_filename.c_str(), "stack");	
	float* prediction_data=NULL;
	prediction.readData(&prediction_data);	
    
	double* prediction_single_ch = new double[depth*height*width];
	double* prediction_ch0 = new double[depth*height*width];
	    

	size_t cube_size, plane_size, element_size=prediction.dim()[prediction.total_dim()-1];
	size_t position, count;		    	
	for (int ch=0; ch < element_size; ch++){
	    count = 0;
	    for(i=0; i<depth; i++){
		cube_size = height*width*element_size;	
		for(j=0; j<height; j++){
		    plane_size = width*element_size;
		    for(k=0; k<width; k++){
			position = i*cube_size + j*plane_size + k*element_size + ch;
			prediction_single_ch[count] = prediction_data[position];
			count++;					
		    }		
		}	
	    }
	    
	    /*C* debug
	    string debug_wshedname = "wshed_bytes.txt";
// 	    string debug_gtruthname = "gt_bytes.txt";
	    string debug_predname = "pred_vals.txt";
	    FILE* wfp= fopen(debug_wshedname.c_str(),"wt");
// 	    FILE* gfp= fopen(debug_gtruthname.c_str(),"wt");
	    FILE* pfp= fopen(debug_predname.c_str(),"wt");
	    for (size_t rr=0; rr< height; rr++){
		for (size_t cc=0; cc<width; cc++){
		    size_t tidx = rr*width+cc;
		    fprintf(wfp, "%u ", watershed_data[tidx]);
// 		    fprintf(gfp, "%u ", groundtruth_data[tidx]);
		    fprintf(pfp, "%lf ", prediction_single_ch[tidx]);
		}
		fprintf(wfp, "\n");
// 		fprintf(gfp, "\n");
		fprintf(pfp, "\n");
	    }
	    fclose(wfp);
// 	    fclose(gfp);
	    fclose(pfp);
	    // */
	    
	    double* zp_prediction_single_ch = NULL;
	    padZero(prediction_single_ch, data_dim, pad_len,&zp_prediction_single_ch);
	    if (ch == 0)
		memcpy(prediction_ch0, prediction_single_ch, depth*height*width*sizeof(double));	


	    stackp->add_prediction_channel(zp_prediction_single_ch);
	}

	stackp->set_basic_features();
	if (px_groundtruth_data)
	    stackp->set_pixel_groundtruth(zp_px_groundtruth_data);


	stackp->get_feature_mgr()->set_classifier(eclfr);   	 

	Label* groundtruth_data=NULL;
	if (groundtruth_filename.size()>0){  	
	    H5Read groundtruth(groundtruth_filename.c_str(),"stack");	
	    groundtruth.readData(&groundtruth_data);	
	    stackp->set_groundtruth(groundtruth_data);
	}	
	    
	std::map<unsigned int, int> mito_list;

	cout<<"Building RAG ..."; 	
	stackp->build_rag();
	cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	
	if (REMOVE_INCLUSION){
	    cout<<"Inclusion removal ..."; 
	    stackp->remove_inclusions();
	}
	cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

// 	stackp->compute_vi();  	
// 	stackp->compute_groundtruth_assignment();
	
	
	/*C* debug
	FILE* mfp= fopen(mitolist_name.c_str(),"wt");
	fprintf(mfp,"%u\n",mito_list.size());
	std::map<unsigned int, int>::iterator mit = mito_list.begin();
	for(; mit!=mito_list.end(); mit++){
	    if (mit->second ==2)
	    fprintf(mfp,"%u\n",mit->first);
	}
	fclose(mfp);
	// */
	
	if (merge_mito && (mito_classifier_filename.size()>0)) {
	    cout << "using mito clfr\n";
	    MitoSpDetector<Label> mdet;
	    mdet.set_var(stackp->get_rag(), stackp->get_feature_mgr());
	    mdet.predict(mito_classifier_filename, mito_thd);
	    Label * temp_label_volume1D = stackp->get_label_volume();
	    
// 	    size_t ofn = imagee.find_last_of(".");
// 	    size_t bfn = imagename.find_last_of("/");
// 	    size_t nchar = ofn-(bfn+1);
// 	    string imagename2 = imagename.substr(bfn+1,nchar);
	    string output_filename = image_dir + "/"+imagename+"_mitolbl.h5";
	    
	    mdet.save(temp_label_volume1D, depth, height,width, output_filename);
	    delete[] temp_label_volume1D;
	}
	

	if (agglo_type==0){	
	    cout<<"Agglomerating (flat) upto threshold "<< threshold<< " ...\n"; 
	    stackp->agglomerate_rag_flat(threshold);	
	}
	else if (agglo_type==1){
	    cout<<"Agglomerating (agglo) upto threshold "<< threshold<< " ...\n"; 
	    stackp->agglomerate_rag(threshold, false);	
	}
// 	else if (agglo_type == 2){		
// 	    cout<<"Agglomerating (mrf) upto threshold "<< threshold<< " ...\n"; 
// 	    stackp->agglomerate_rag_mrf(threshold, read_off_rwts, output_filename, classifier_filename);	
// 	}		
	else if (agglo_type == 3){		
	    cout<<"Agglomerating (queue) upto threshold "<< threshold<< " ...\n"; 
	    stackp->agglomerate_rag_queue(threshold, false);	
	}		
	else if (agglo_type == 4){		
	    cout<<"Agglomerating (flat) upto threshold "<< threshold<< " ...\n"; 
	    stackp->agglomerate_rag_flat(threshold, false, global_output_filename, classifier_filename);	
	}		
	cout << "Done with "<< stackp->get_num_bodies()<< " regions\n";

	if (REMOVE_INCLUSION){
	    cout<<"Inclusion removal ..."; 
	    stackp->remove_inclusions();
	    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	
	}
	Label * temp_label_volume1D = stackp->get_label_volume();       	    
	if (!merge_mito && min_region_sz>0){
	  stackp->absorb_small_regions2(prediction_ch0, temp_label_volume1D, min_region_sz);
	}
	hsize_t dims_out[3];
	dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;   
	H5Write(output_filename_nomito.c_str(),"stack",3,dims_out, temp_label_volume1D);
	printf("Output-nomito written to %s, dataset %s\n",output_filename_nomito.c_str(),output_dataset_name.c_str());	
	delete[] temp_label_volume1D;	
	



	stackp->compute_vi();  	
	    

// 	hsize_t dims_out[3];
// 	dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;   
// 	H5Write(output_filename_nomito.c_str(),"stack",3,dims_out, temp_label_volume1D);
// 	printf("Output-nomito written to %s, dataset %s\n",output_filename_nomito.c_str(),output_dataset_name.c_str());	
// 	delete[] temp_label_volume1D;	


	if (merge_mito){
	    cout<<"Merge Mitochondria (border-len) ..."; 
	    stackp->merge_mitochondria_a();
	    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

	    if (REMOVE_INCLUSION){
		cout<<"Inclusion removal ..."; 
		stackp->remove_inclusions();
		cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	
	    }

	    stackp->compute_vi();  	
	    
	    temp_label_volume1D = stackp->get_label_volume();     
	    if (min_region_sz>0)
		stackp->absorb_small_regions2(prediction_ch0, temp_label_volume1D, min_region_sz);

	    dims_out[0]=depth; dims_out[1]= height; dims_out[2]= width;   
	    H5Write(output_filename.c_str(), "stack",3,dims_out, temp_label_volume1D);
	    printf("Output written to %s, dataset %s\n",output_filename.c_str(),output_dataset_name.c_str());	
	    delete[] temp_label_volume1D;	
	} 	
	//stackp->determine_edge_locations();
	//stackp->write_graph(output_filename);

	time(&end);	
	printf("Time elapsed: %.2f\n", (difftime(end,start))*1.0/60);

	delete stackp;

	if (watershed_data)  	
	    delete[] watershed_data;
	delete[] zp_watershed_data;
	    
	if (prediction_data)  	
	    delete[] prediction_data;
	delete[] prediction_single_ch;

	if (groundtruth_data)  	
	    delete[] groundtruth_data;
    
    }


    return 0;
}
