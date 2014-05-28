#version 120

//
//  shadow_map.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 3/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

varying vec4 shadowColor;

void main(void) {
    // the shadow color includes only the ambient terms
    shadowColor = gl_Color * (gl_LightModel.ambient + gl_LightSource[0].ambient);
    
    // the normal color includes diffuse
    vec4 normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    gl_FrontColor = shadowColor + gl_Color * (gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));
    
    // generate the shadow texture coordinates using the eye position
    vec4 eyePosition = gl_ModelViewMatrix * gl_Vertex;
    gl_TexCoord[0] = vec4(dot(gl_EyePlaneS[0], eyePosition), dot(gl_EyePlaneT[0], eyePosition),
        dot(gl_EyePlaneR[0], eyePosition), 1.0);
    gl_TexCoord[1] = vec4(dot(gl_EyePlaneS[1], eyePosition), dot(gl_EyePlaneT[1], eyePosition),
        dot(gl_EyePlaneR[1], eyePosition), 1.0);
    gl_TexCoord[2] = vec4(dot(gl_EyePlaneS[2], eyePosition), dot(gl_EyePlaneT[2], eyePosition),
        dot(gl_EyePlaneR[2], eyePosition), 1.0);
    gl_TexCoord[3] = vec4(dot(gl_EyePlaneS[3], eyePosition), dot(gl_EyePlaneT[3], eyePosition),
        dot(gl_EyePlaneR[3], eyePosition), 1.0);
    
    // use the fixed function transform
    gl_Position = ftransform();
}
