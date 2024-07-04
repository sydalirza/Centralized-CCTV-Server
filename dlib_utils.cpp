#include "dlib_utils.h"
#include <dlib/opencv.h>
#include <dlib/image_io.h>

// Define the network and shape predictor
anet_type net;
dlib::shape_predictor sp;

void initialize_network()
{
    dlib::deserialize("dlib_face_recognition_resnet_model_v1.dat") >> net;
}

void initialize_shape_predictor()
{
    dlib::deserialize("shape_predictor_5_face_landmarks.dat") >> sp;
}
