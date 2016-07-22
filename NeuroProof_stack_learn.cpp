#include "DataStructures/Stack.h"
#include "DataStructures/MitoSpDetector.h"
// #include "Priority/GPR.h"
// #include "Priority/LocalEdgePriority.h"
// #include "Utilities/ScopeTime.h"
// #include "ImportsExports/ImportExportRagPriority.h"
#include "Utilities/folder.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <memory>
// #include <json/json.h>
// #include <json/value.h>

#include "Utilities/h5read.h"
#include "Utilities/h5write.h"

#include <time.h>

using std::cerr; using std::cout; using std::endl;
using std::ifstream;
using std::string;
using std::stringstream;
using namespace NeuroProof;
using namespace std;



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



int main(int argc, char** argv) 
{

    cout<< "Reading data ..." <<endl;
    size_t          i, j, k;



    if (argc<8){
	printf("format: NeuroProof_stack_learn -train_dir training_directory \
					 -iteration num_iteration \
					 -strategy learning_strategy \
					 -classifier classifier_file \n");
	return 0;
    }	

    int argc_itr=1;	
    string watershed_filename="";
    string watershed_dataset_name="";		
    string prediction_filename="";
    string prediction_dataset_name="";		
    string groundtruth_filename="";
    string groundtruth_dataset_name="";		
    string classifier_filename;
    string mito_classifier_filename="";
    	
    int maxIter = 1; 	
    int strategy = 1; 	
    bool merge_mito = true;
    double mito_thd=0.3;
    
    string training_dir;
    string mito_det_dir;
    
    while(argc_itr<argc){
	if (!(strcmp(argv[argc_itr],"-train_dir"))){
	    training_dir = argv[++argc_itr];
	}

	if (!(strcmp(argv[argc_itr],"-mito_det_dir"))){
	    mito_det_dir = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-mito_classifier"))){
	    mito_classifier_filename = argv[++argc_itr];
	}
	
	if (!(strcmp(argv[argc_itr],"-classifier"))){
	    classifier_filename = argv[++argc_itr];
	}
	if (!(strcmp(argv[argc_itr],"-nomito"))){
	    merge_mito = false;
	}
	if (!(strcmp(argv[argc_itr],"-iteration"))){
	    maxIter = atoi(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-strategy"))){
	    strategy = atoi(argv[++argc_itr]);
	}
	if (!(strcmp(argv[argc_itr],"-mito_thd"))){
	    mito_thd = atof(argv[++argc_itr]);
	}
        ++argc_itr;
    } 	

    
    
    time_t start, end;
    time(&start);	
    
    size_t element_size;
    string imagename;
    UniqueRowFeature_Label	all_features;
    
    double threshold=0.2;

    EdgeClassifier* eclfr;
    if (endswith(classifier_filename, "h5"))
	eclfr = new VigraRFclassifier();	
    else if (endswith(classifier_filename, "xml")) 	
	eclfr = new OpencvRFclassifier();	

    for(int itr=0; itr < maxIter ;itr++){
	printf("\n iteration %d \n", itr);
	Folder datadir(training_dir);
	size_t img_count=0;  
	while (datadir.get_next(imagename, prediction_filename, watershed_filename, groundtruth_filename))
	{
	//clock_t start = clock();
	    printf("image %d: %s\nwatershed: %s\nprediction :%s\ngroundtruth :%s\n", ++img_count, imagename.c_str(),
	      watershed_filename.c_str(), prediction_filename.c_str(), groundtruth_filename.c_str());
	    
	    H5Read watershed(watershed_filename.c_str(), "stack", true);	
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
	    
		

	    H5Read groundtruth(groundtruth_filename.c_str(), "stack", true);	
	    Label* groundtruth_data=NULL;
	    groundtruth.readData(&groundtruth_data);	
	    
	    Label* px_groundtruth_data=NULL;
	    Label *zp_px_groundtruth_data=NULL;
	    int pad_len=1;
	    
	    if (mito_det_dir.size()>0){
		string imgname_noextn = imagename;
		imgname_noextn.erase(0, training_dir.size());
		imgname_noextn.erase(imgname_noextn.size()-4, 4);
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

	    // * not necessary, compute_contingency_table() and compute_groundtruth_assignment() does not
	    // * use margin
// 	    Label *zp_groundtruth_data=NULL;
// 	    padZero(groundtruth_data, data_dim,pad_len,&zp_groundtruth_data);	
		

	    H5Read prediction(prediction_filename.c_str(), "stack", true);	
	    float* prediction_data=NULL;
	    prediction.readData(&prediction_data);	
	    double* prediction_ch1 = new double[depth*height*width];


    // 	for(int itr=0 ; itr < maxIter ; itr++){

    // 	    printf("\n ** Learning iteration %d  **\n\n",itr+1);
		
	    StackLearn* stackp = new StackLearn(zp_watershed_data, depth+2*pad_len, height+2*pad_len, width+2*pad_len, pad_len);
	    stackp->set_feature_mgr(new FeatureMgr());
	    stackp->set_merge_mito(merge_mito, mito_thd);

	    size_t cube_size, plane_size;
	    element_size=prediction.dim()[prediction.total_dim()-1];
	    
	    size_t position, count;		    	
	    for (int ch=0; ch < element_size; ch++){
		count = 0;
		for(i=0; i<depth; i++){
		    cube_size = height*width*element_size;	
		    for(j=0; j<height; j++){
			plane_size = width*element_size;
			for(k=0; k<width; k++){
			    position = i*cube_size + j*plane_size + k*element_size + ch;
			    prediction_ch1[count] = prediction_data[position];				
			    count++;
			}		
		    }	
		}

		
		double* zp_prediction_ch1 = NULL;
		padZero(prediction_ch1, data_dim ,pad_len,&zp_prediction_ch1);
		stackp->add_prediction_channel(zp_prediction_ch1);
	    }

	    stackp->set_basic_features();  	


	    stackp->set_groundtruth(groundtruth_data); //compute_groundtruth_assignment() does not use padding
	    if (px_groundtruth_data)
		stackp->set_pixel_groundtruth(zp_px_groundtruth_data);

	    printf("Building RAG ..."); 	
	    stackp->build_rag();
	    printf("done with %d regions\n", stackp->get_num_bodies());	

	    if (merge_mito && (mito_classifier_filename.size()>0)) {
		MitoSpDetector<Label> mdet;
		mdet.set_var(stackp->get_rag(), stackp->get_feature_mgr());
		mdet.predict(mito_classifier_filename,mito_thd);
		Label * temp_label_volume1D = stackp->get_label_volume();
		
		size_t ofn = imagename.find_last_of(".");
		size_t bfn = imagename.find_last_of("/");
		size_t nchar = ofn-(bfn+1);
		string imagename2 = imagename.substr(bfn+1,nchar);
		string output_filename = training_dir + "/"+imagename2+"_mitolbl.h5";
		
		mdet.save(temp_label_volume1D, depth, height,width, output_filename);
		delete[] temp_label_volume1D;
	    }
	    

	    cout<<"Generating features ..."; 
	    if(itr<1)
	      stackp->generate_edge_classifier_features(threshold,all_features); 
	    else{
	      stackp->get_feature_mgr()->set_classifier(eclfr);
	      stackp->generate_edge_classifier_features_queue(threshold,all_features); // # iteration, threshold, clfr_filename	
	    }

    // 	    if (itr<1){
    // 		stackp->learn_edge_classifier_flat(threshold,all_features,all_labels); // # iteration, threshold, clfr_filename
    // 	    }
    // 	    else{
    // 		if (strategy == 1){ //accumulate only misclassified 
    // 		    printf("cumulative learning, only misclassified\n");
    // 		    stackp->learn_edge_classifier_queue(threshold,all_features,all_labels, false); // # iteration, threshold, clfr_filename	
    // 		}
    // 		else if (strategy == 2){ //accumulate all 
    // 		    printf("cumulative learning, all\n");
    // 		    stackp->learn_edge_classifier_queue(threshold,all_features,all_labels, true); // # iteration, threshold, clfr_filename	
    // 		}	
    // 		else if (strategy == 3){ // lash	
    // 		    printf("learning by LASH\n");
    // 		    stackp->learn_edge_classifier_lash(threshold,all_features,all_labels); // # iteration, threshold, clfr_filename	
    // 		}
    // 	    }
    // 
    // 	    cout<<"done with "<< stackp->get_num_bodies()<< " regions\n";	

	    delete stackp;

    // 	}
	// end for


	
	//    printf("Time elapsed: %.2f\n", ((double)clock() - start) / CLOCKS_PER_SEC);


	    if (watershed_data) 
		delete[] watershed_data;
	    delete[] zp_watershed_data;

	    if (prediction_data)  	
		delete[] prediction_data;
	    delete[] prediction_ch1;	

	    if (groundtruth_data)  	
		delete[] groundtruth_data;
	    if (px_groundtruth_data)  	
		delete[] px_groundtruth_data;
	    if (zp_px_groundtruth_data)  	
		delete[] zp_px_groundtruth_data;
// 	    delete[] zp_groundtruth_data;	

	    printf("total number of features: %u\n", all_features.nrows());
	
	}
	
    //     stackp-> get_feature_mgr()->set_classifier(eclfr);	 
	
    //     stackp->fit_edge_classifier(threshold, all_features);
	
	std::vector< std::vector<double> > all_feat;
	std::vector<int> all_labels;
	
	all_features.get_feature_label(all_feat, all_labels); 
	
	//*for interactive learning
	
	FILE* fp = fopen("all_feature_labels_TEM.txt","wt");
	
	fprintf(fp, "%u %u %u\n", all_feat.size(), all_feat[0].size(),element_size);
	for(size_t irow=0; irow<all_feat.size(); irow++){
	    for(size_t icol=0; icol<all_feat[0].size(); icol++){
		fprintf(fp,"%lf ",all_feat[irow][icol]);
	    }
	    fprintf(fp, "%d\n", all_labels[irow]);
	}
	    
	fclose(fp);
	/**/
	
// 	all_feat.erase(all_feat.begin()+500, all_feat.end());
// 	all_labels.erase(all_labels.begin()+500, all_labels.end());
	
	
	eclfr->learn(all_feat, all_labels); // number of trees
	printf("Classifier learned \n");
	
	eclfr->save_classifier(classifier_filename.c_str());  	
	printf("Classifier saved to %s\n",classifier_filename.c_str());

    }
     
    time(&end);	
    printf("Time elapsed: %.2f\n", (difftime(end,start))*1.0/60);
     	

    return 0;
}
