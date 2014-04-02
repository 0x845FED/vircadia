#version 120

//
//  shadow_map.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 3/27/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

varying vec4 shadowColor;

void main(void) {
    // the shadow color includes only the ambient terms
    shadowColor = gl_Color * (gl_LightModel.ambient + gl_LightSource[0].ambient);
    
    // the normal color includes diffuse
    vec4 normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    gl_FrontColor = shadowColor + gl_Color * (gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));
    
    // generate the shadow texture coordinate using the eye position
    vec4 eyePosition = gl_ModelViewMatrix * gl_Vertex;
    gl_TexCoord[0] = vec4(dot(gl_EyePlaneS[0], eyePosition), dot(gl_EyePlaneT[0], eyePosition),
        dot(gl_EyePlaneR[0], eyePosition), 1.0); 
    
    // use the fixed function transform
    gl_Position = ftransform();
}
