#ifndef _FLODER_
#define _FOLDER_

#include<sys/types.h>
#include<dirent.h>
#include "string_functions.h"

#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QRegExp>


class DataElem{
public:
    string imagename;
    string groundtruth;
    string watershed;
    string prediction;
};

class Folder{
    std::map <string, DataElem> _datalist;  
    std::map <string, DataElem>::iterator lit;
    string _dirname;
    int _nFiles;
    int _fitr;
    QStringList _allFileNames;
  
public:
    Folder(string dirname){
      
	_dirname = dirname;
	QString qdirname=_dirname.c_str();
	QFileInfo fileInfo(qdirname);
	QDir directory= fileInfo.dir();
	QStringList nameFilter("*.png");
	_allFileNames = directory.entryList(nameFilter);
	
	_nFiles = _allFileNames.size();
	
	DIR *dir;
	struct dirent *ent;
	string fname, img_idx, img_type;  
	
	
	if ((dir = opendir (dirname.c_str())) != NULL) {
	  /* print all the files and directories within directory */
	  while ((ent = readdir (dir)) != NULL) {
	    fname.assign(ent->d_name);
// 	    printf ("%s\n", fname.c_str());
	    
	    if (endswith(fname,"png")){
		get_image_idx(fname, img_idx, img_type);
		insert_to_filelist(fname, img_idx, img_type);
	    }	    
	    else if (endswith(fname,"h5")){
		get_image_idx2(fname, img_idx, img_type);
		insert_to_filelist(fname, img_idx, img_type);
	    }
	  }
	  closedir (dir);
	} else {
	  /* could not open directory */
	  perror ("");
	  return;//EXIT_FAILURE;
	}
	
	lit = _datalist.begin();
	_fitr=0;
	    
    };
    void insert_to_filelist(string filename, string first_part, string img_type){
	
	std::map <string, DataElem>::iterator it;
	
	it = _datalist.find(first_part);
	if (it != _datalist.end()){
	    if (img_type.compare("prediction") == 0 )
	      (it->second).prediction = filename;
	    else if (img_type.compare("groundtruth") == 0 )
	      (it->second).groundtruth = filename;
	    else if (img_type.compare("watershed") == 0 )
	      (it->second).watershed = filename;
	    else if (img_type.compare("feature") == 0 )
		;
	    else if (img_type.compare("mitolbl") == 0 )
		;
	    else
	       (it->second).imagename = filename;
	    
	}
	else{
	    DataElem de;
	    
	    if (img_type.compare("prediction") == 0 )
	      de.prediction = filename;
	    else if (img_type.compare("groundtruth") == 0 )
	      de.groundtruth = filename;
	    else if (img_type.compare("watershed") == 0 )
	      de.watershed = filename;
	    else if (img_type.compare("feature") == 0 )
		;
	    else if (img_type.compare("mitolbl") == 0 )
		;
	    else
	       de.imagename = filename;
	    
	    _datalist.insert(std::make_pair(first_part, de));
	}
      
    }
    
    bool get_next(string &ifname, string &pname, string &wname, string &gname){
      
	if (_fitr>=_nFiles)
// 	if (lit==_datalist.end())
	  return false;
	
        QString fileName = _allFileNames.at(_fitr);
        QString wsFileName = fileName.replace(QRegExp(".png"),"_watershed.h5");
        wname.assign(_dirname+ "/"+wsFileName.toStdString());
	
	fileName = _allFileNames.at(_fitr);
        QString ppFileName = fileName.replace(QRegExp(".png"),"_spfeature.h5");
        pname.assign(_dirname + "/" + ppFileName.toStdString());
	QFileInfo pfileinfo(pname.c_str());
	if (!(pfileinfo.exists())){
	    fileName = _allFileNames.at(_fitr);
	    ppFileName = fileName.replace(QRegExp(".png"),"_prediction.h5");
	    pname.assign(_dirname + "/" + ppFileName.toStdString());	  
	}
	
	fileName = _allFileNames.at(_fitr);
        QString gtFileName = fileName.replace(QRegExp(".png"),"_groundtruth.h5");
        gname.assign(_dirname + "/" + gtFileName.toStdString());
	
	fileName = _allFileNames.at(_fitr);
	ifname.assign(_dirname+ "/"+ fileName.toStdString());
// 	ifname.assign(_dirname+ "/"+ (lit->second).imagename);
// 	pname.assign(_dirname+ "/"+ (lit->second).prediction);
// 	wname.assign(_dirname+ "/"+ (lit->second).watershed);
// 	gname.assign(_dirname+ "/"+ (lit->second).groundtruth);
	
	lit++;
	_fitr++;
	return true;
	
    };
    bool get_next(string &ifname, string &pname, string &wname){
      
	if (_fitr>=_nFiles)
// 	if (lit==_datalist.end())
	  return false;
	
	
        QString fileName = _allFileNames.at(_fitr);
        QString wsFileName = fileName.replace(QRegExp(".png"),"_watershed.h5");
        wname.assign(_dirname+ "/"+wsFileName.toStdString());
	
	fileName = _allFileNames.at(_fitr);
        QString ppFileName = fileName.replace(QRegExp(".png"),"_spfeature.h5");
        pname.assign(_dirname + "/" + ppFileName.toStdString());
	QFileInfo pfileinfo(pname.c_str());
	if (!(pfileinfo.exists())){
	    fileName = _allFileNames.at(_fitr);
	    ppFileName = fileName.replace(QRegExp(".png"),"_prediction.h5");
	    pname.assign(_dirname + "/" + ppFileName.toStdString());	  
	}
	
	
	fileName = _allFileNames.at(_fitr);
	ifname.assign(_dirname+ "/"+ fileName.toStdString());
// 	ifname.assign(_dirname+ "/"+ (lit->second).imagename);
// 	pname.assign(_dirname+ "/"+ (lit->second).prediction);
// 	wname.assign(_dirname+ "/"+ (lit->second).watershed);
	
	lit++;
	_fitr++;
	
	return true;
	
    };
    
    size_t ndata(){return _datalist.size();};
};

#endif 

