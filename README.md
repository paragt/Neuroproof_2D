# Neuroproof_2D 

Neuroproof tweaked to work on 2D data, i.e., images.



# Build
Linux: Install miniconda on your workstation. Create and activate the conda environment using the following commands:

  conda create -n my_conda_env -c flyem vigra opencv qt

  source activate my_conda_env

Then follow the usual procedure of building:

  mkdir build
 
  cd build

  cmake -DCMAKE_PREFIX_PATH=[CONDA_ENV_PATH]/my_conda_env ..

# Example

Generate the prediction and oversegmentation using necessary tools. For example, we have produced the pixel prediction using ilastik (ilastik.org) and oversegmentation by watershed. Provided with an ilastik project (ilp), execute the following commands 

pixel prediction: run_ilastik.sh --headless  --project=Training_merged_1.ilp  --output_format=hdf5 --output_filename_format=[pred_dir]/{nickname}_prediction.h5  --output_internal_path=stack  [imfile]

python convert_wshed2.py  [pixel_prediction_filename] [watershed_filename] [pixel_prediction_filename]  [Thd_prediction] [MinRegionSz]


Create a folder called 'output' within Example/nissl.

build/NeuroProof_stack -image_dir Example/nissl/ -classifier Example/sp_clfr_805.xml -nomito -threshold 0.45 -algorithm 1 -min_region-sz 0

The min_region_sz in Neuroproof_stack must be 0 (due to a sub-optimal code design). The code will automatically find all the filenames with _prediction.h5 and _watershed.h5 at the end within the image_dir (modify the folder.h file if you wish to change this).


