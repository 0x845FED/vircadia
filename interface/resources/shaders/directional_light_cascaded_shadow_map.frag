#version 120

//
//  directional_light.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal texture
uniform sampler2D normalMap;

// the specular texture
uniform sampler2D specularMap;

// the depth texture
uniform sampler2D depthMap;

// the shadow texture
uniform sampler2DShadow shadowMap;

// the distances to the cascade sections
uniform vec3 shadowDistances;

// the inverse of the size of the shadow map
uniform float shadowScale;

// the distance to the near clip plane
uniform float near;

// scale factor for depth: (far - near) / far
uniform float depthScale;

// offset for depth texture coordinates
uniform vec2 depthTexCoordOffset;

// scale for depth texture coordinates
uniform vec2 depthTexCoordScale;

void main(void) {
    float depthVal = texture2D(depthMap, gl_TexCoord[0].st).r;
    vec4 normalVal = texture2D(normalMap, gl_TexCoord[0].st);
    vec4 diffuseVal = texture2D(diffuseMap, gl_TexCoord[0].st);
    vec4 specularVal = texture2D(specularMap, gl_TexCoord[0].st);

    // compute the view space position using the depth
    float z = near / (depthVal * depthScale - 1.0);
    vec4 position = vec4((depthTexCoordOffset + gl_TexCoord[0].st * depthTexCoordScale) * z, z, 1.0);

    // get the normal from the map
    vec4 normal = normalVal;
    if ((normalVal.a >= 0.45) && (normalVal.a <= 0.55)) {
        normal.a = 1.0;
        normalVal.a = 0.0;
        gl_FragColor = vec4(diffuseVal.rgb * specularVal.rgb, 1.0);
    } else {
     
        // compute the index of the cascade to use and the corresponding texture coordinates
        int shadowIndex = int(dot(step(vec3(position.z), shadowDistances), vec3(1.0, 1.0, 1.0)));
        vec3 shadowTexCoord = vec3(dot(gl_EyePlaneS[shadowIndex], position), dot(gl_EyePlaneT[shadowIndex], position),
            dot(gl_EyePlaneR[shadowIndex], position));
        
        // get the normal from the map
        vec3 normalizedNormal = normalize(normal.xyz * 2.0 - vec3(1.0));
        
        // average values from the shadow map
        float diffuse = dot(normalizedNormal, gl_LightSource[0].position.xyz);
        float facingLight = step(0.0, diffuse) * 0.25 *
            (shadow2D(shadowMap, shadowTexCoord + vec3(-shadowScale, -shadowScale, 0.0)).r +
            shadow2D(shadowMap, shadowTexCoord + vec3(-shadowScale, shadowScale, 0.0)).r +
            shadow2D(shadowMap, shadowTexCoord + vec3(shadowScale, -shadowScale, 0.0)).r +
            shadow2D(shadowMap, shadowTexCoord + vec3(shadowScale, shadowScale, 0.0)).r);
        
        // compute the base color based on OpenGL lighting model
        vec3 baseColor = diffuseVal.rgb * (gl_FrontLightModelProduct.sceneColor.rgb +
            gl_FrontLightProduct[0].ambient.rgb + gl_FrontLightProduct[0].diffuse.rgb * (diffuse * facingLight));
        
        // compute the specular multiplier (sans exponent)
        float specular = facingLight * max(0.0, dot(normalize(gl_LightSource[0].position.xyz - normalize(position.xyz)),
            normalizedNormal));    
        
        // add specular contribution
        vec4 specularColor = specularVal;
        gl_FragColor = vec4(baseColor.rgb + pow(specular, specularColor.a * 128.0) * specularColor.rgb, normal.a);
    }
}
