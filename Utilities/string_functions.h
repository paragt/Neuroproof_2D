#ifndef _STRING_FUNCTIONS
#define _STRING_FUNCTIONS

#include <string>

bool endswith(std::string filename, std::string extn){
    unsigned found = filename.find_last_of(".");
    std::string fextn = filename.substr(found+1);	
    if (fextn.compare(extn) == 0 )
	return true;
    else return false;	  
}

void get_image_idx(std::string& filename, std::string& first_part, std::string& img_type){
    unsigned found = filename.find_last_of(".");
    std::string part1 = filename.substr(0,found);	
    
    unsigned found2 = part1.find_last_of("_");
    first_part.assign(part1);
    img_type.assign(filename.substr(found+1));
}

void get_image_idx2(std::string& filename, std::string& first_part, std::string& img_type){
    unsigned found = filename.find_last_of(".");
    std::string part1 = filename.substr(0,found);	
    
    unsigned found2 = part1.find_last_of("_");
    first_part.assign(part1.substr(0,found2));
    img_type.assign(part1.substr(found2+1));
}

void get_output_filename(std::string imagepath, std::string threshold_str, std::string& output_filename, std::string& output_filename_nomito){
// 	output_filename = image_dir+"/output/";
// 	imagename = imagepath;
	size_t ofn = imagepath.find_last_of(".");
	size_t bfn = imagepath.find_last_of("/");
	size_t nchar = ofn-(bfn+1);
	std::string imagename = imagepath.substr(bfn+1,nchar);
	std::string image_dir = imagepath.substr(0,bfn);
	output_filename = image_dir + "/output/"+imagename+"_result_thd"+threshold_str+"_m4.h5";
// 	std::string global_output_filename = image_dir + "/output/"+imagename+"_global_result_thd"+threshold_str+"_m4.h5";
	std::string mitolist_name = image_dir + "/output/"+imagename+"_mitolist.txt";
// 	output_filename += "_result_m4.h5";
// 	if (agglo_type ==4)
// 	    output_filename = global_output_filename;
	output_filename_nomito = output_filename;
	ofn = output_filename_nomito.find_last_of(".");
	output_filename_nomito.erase(ofn-1,1);			
}


#endif