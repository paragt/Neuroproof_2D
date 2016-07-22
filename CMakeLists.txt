CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project (Neuroproof_2D)

#  ask to use -DCMAKE_PREFIX_PATH=/groups/scheffer/home/paragt/miniconda/envs/tst_my/

#   include_directories (/groups/scheffer/home/paragt/miniconda/envs/tst_my/include/)
   include_directories (${CMAKE_PREFIX_PATH}/include)

   include_directories (Algorithms)
   include_directories (Utilities)
   include_directories (Watershed)
   include_directories (Classifier)
   include_directories (DataStructures)
   include_directories (FeatureManager)
   include_directories (${CMAKE_PREFIX_PATH}/include/qt/QtCore)

#    link_directories (/groups/scheffer/home/paragt/miniconda/envs/tst_my/lib) 
   link_directories (${CMAKE_PREFIX_PATH}/lib)

   add_subdirectory (Algorithms)
   add_subdirectory (Classifier)
   add_subdirectory (DataStructures)
   add_subdirectory (FeatureManager)

    
   set(CMAKE_CXX_FLAGS "-std=c++0x")

   add_executable (NeuroProof_stack NeuroProof_stack.cpp)  
   add_executable (NeuroProof_stack_learn NeuroProof_stack_learn.cpp)  


   target_link_libraries (NeuroProof_stack Algorithms Classifier DataStructures FeatureManager vigraimpex opencv_ml opencv_core boost_python boost_thread boost_system boost_chrono python2.7 png hdf5 hdf5_hl QtCore)  
   target_link_libraries (NeuroProof_stack_learn Algorithms Classifier DataStructures FeatureManager vigraimpex opencv_ml opencv_core boost_python boost_thread boost_system boost_chrono python2.7 png hdf5 hdf5_hl QtCore)  


############################################################################
# endif()
