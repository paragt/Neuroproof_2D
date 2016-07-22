
# -*- coding: utf-8 -*-

#!/usr/bin/env python

#from gala import imio, agglo, morpho, classify, features, evaluate
import h5py
from skimage import morphology as skmorph
from skimage import feature as skfeature
from scipy import ndimage as ndi
import Image

#from scipy.ndimage import label
from numpy import unique, array, expand_dims, bincount, ones, zeros, gradient, amax, sqrt
import sys

import pdb


def remove_small_connected_components(a, min_size=64, in_place=False):
    original_dtype = a.dtype
    if a.dtype == bool:
        a = label(a)[0]
    elif not in_place:
        a = a.copy()
    if min_size == 0: # shortcut for efficiency
        return a
    component_sizes = bincount(a.ravel())
    too_small = component_sizes < min_size
    too_small_locations = too_small[a]
    a[too_small_locations] = 0
    return a.astype(original_dtype)

def read_h5(filename, dataset_name):
    fp = h5py.File(filename)
    mat = array(fp["stack"])
    fp.close()
    return mat
    
def write_h5(datamat, filename, dataset_name):
    fp = h5py.File(filename,'w')
    fp.create_dataset(dataset_name, data=datamat)
    fp.close()

def normalize_val(input_data, maxval):    
    normalize_data = input_data
    larger_than_max = normalize_data > maxval
    normalize_data[larger_than_max] = maxval
    normalize_data = normalize_data / maxval
    return normalize_data
    
    
if __name__ == "__main__" :
  
    
    n_cmd_args =  len(sys.argv)

    prediction_file=sys.argv[1]
    spfeature_file= sys.argv[3]
    watershed_file= sys.argv[2]

    pred_thd = float(sys.argv[4])
    size_threshold = int(sys.argv[5])
    
    #pdb.set_trace()
    imagename = watershed_file[:-13] + '.png'

    p_all = read_h5(prediction_file, "stack")
    p = p_all[:,:,0]
    
    imwidth = p.shape[0]
    imheight = p.shape[1]
    #ws_in = read_h5(watershed_file, "stack")
    #seeds = ws_in
    #mask_im = read_h5(mask_file,"stack")
    
    #morph_struc = array([ [0,0,1,0,0],
			  #[0,1,1,1,0],
			  #[1,1,1,1,1],
			  #[0,1,1,1,0],
			  #[0,0,1,0,0]])
    morph_struc = array([ [0,1,0],
			  [1,1,1],
			  [0,1,0]])
    
    #pdb.set_trace()
    bpp = p.T > pred_thd
    mbpp1 = ndi.morphology.binary_closing(bpp, structure=morph_struc)
    mbpp2 = ndi.morphology.binary_opening(mbpp1, structure=morph_struc)
    
    dt = ndi.distance_transform_edt(mbpp2)
    #local_maxi = skfeature.peak_local_max(dt, indices = False, footprint = ones((3, 3)),labels = mask_im)
    local_max = skfeature.peak_local_max(dt, min_distance=3)
    local_maxi = zeros(dt.shape)
    local_maxi[local_max[:,0],local_max[:,1]]=1
    markers = ndi.label(local_maxi)[0]
    ws = skmorph.watershed(-dt, markers, mask=mbpp2)

    
    #mask_im = (mask_im>0)
    
    #print "Performing watershed"
    ##local_maxi = peak_local_max(p, indices=False, footprint=ones((3, 3)), lables=mask_im)    
    #ws = skmorph.watershed(-p, seeds, mask=mask_im)
    print "Watershed initially has "+ str(unique(ws).size) + " regions"
    ws_int = remove_small_connected_components(ws, min_size=size_threshold)
    
    ws_int1 = skmorph.watershed(-dt, ws_int, mask=mbpp2)
    
    ws_int2 =  ws_int1.astype('int32')
    if len(ws_int2.shape)<3:
	ws_int2 = expand_dims(ws_int2, axis=0)
    
    write_h5(ws_int2,watershed_file,'stack'); 	
    print "Watershed has "+ str(unique(ws_int2).size) + " regions"

    #pdb.set_trace()
    spfeat = zeros((imwidth, imheight, 3))
    image_mat = array(Image.open(imagename))	
    image_val = image_mat.astype('float32')
    image_val = image_val/255
       
    spfeat[:,:,0] = image_val
    
    spfeat[:,:,1] = p_all[:,:,2].T
    
    normalized_dt = normalize_val(dt, (imwidth*1.0/20.0))
    spfeat[:,:,2] = normalized_dt
    
    ###image_mat = array(Image.open(imagename))	
    ###image_val = image_mat.astype('float32')
    ###grad_vals = gradient(image_val)
    ###grad_mag = sqrt((grad_vals[0]**2 + grad_vals[1]**2))
    ###norm_grad_mag = normalize_val(grad_mag, 255.0/10.0)
    ###spfeat[:,:,3] = norm_grad_mag
    
    ##grad_vals = gradient(p)
    ##grad_mag = sqrt((grad_vals[0]**2 + grad_vals[1]**2))
    ##spfeat[:,:,3] = grad_mag.T

    spfeat = expand_dims(spfeat, axis=0).astype('float32')
    write_h5(spfeat,spfeature_file,'stack'); 	
    