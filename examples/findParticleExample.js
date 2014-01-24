//
//  editParticleExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates creating and editing a particle
//

var iteration = 0;

var particleA = Particles.addParticle(
                    {
                        position: { x: 2, y: 0, z: 2 },
                        velocity: { x: 0, y: 0, z: 0 },
                        gravity: { x: 0, y: 0, z: 0 },
                        radius : 0.1,
                        color: { red: 0, green: 255, blue: 0 }
                    });

var particleB = Particles.addParticle(
                    {
                        position: { x: 5, y: 0, z: 5 },
                        velocity: { x: 0, y: 0, z: 0 },
                        gravity: { x: 0, y: 0, z: 0 },
                        radius : 0.1,
                        color: { red: 0, green: 255, blue: 255 }
                    });

var searchAt = { x: 0, y: 0, z: 0};
var moveSearch = { x: 0.1, y: 0, z: 0.1};
var searchRadius = 1;
var searchRadiusChange = 0;

print("particleA.creatorTokenID = " + particleA.creatorTokenID);
print("particleB.creatorTokenID = " + particleB.creatorTokenID);


function scriptEnding() {
    print("calling Particles.deleteParticle()");
    Particles.deleteParticle(particleA);
    Particles.deleteParticle(particleB);
}

function printProperties(properties) {
    for (var property in properties) {
        if (properties.hasOwnProperty(property)) { 
            if (property == "position" || 
                property == "gravity" || 
                property == "velocity") {
                print(property +": " + properties[property].x + ", " + properties[property].y + ", " + properties[property].z);
            } else if (property == "color") {
                print(property +": " + properties[property].red + ", " 
                            + properties[property].green + ", " + properties[property].blue);
            } else {
                print(property +": " + properties[property])
            }
        }
    }
}

function findParticles() {

    // run for a while, then clean up
    // stop it...
    if (iteration >= 100) {
        print("calling Agent.stop()");
        Agent.stop();
    }

    print("--------------------------");
    print("iteration =" + iteration);
    iteration++;

    // Check to see if we've been notified of the actual identity of the particles we created
    if (!particleA.isKnownID) {
        var identifyA = Particles.identifyParticle(particleA);
        if (identifyA.isKnownID) {
            particleA = identifyA;
            print(">>>> identified particleA.id = " + particleA.id);
        }
    }
    if (!particleB.isKnownID) {
        var identifyB = Particles.identifyParticle(particleB);
        if (identifyB.isKnownID) {
            particleB = identifyB;
            print(">>>> identified particleB.id = " + particleB.id);
        }
    }
    
    // also check to see if we can "find" particles...
    print("searching for particles at:" + searchAt.x + ", " + searchAt.y + ", " + searchAt.z + " radius:" + searchRadius);
    var foundParticles = Particles.findParticles(searchAt, searchRadius);
    print("found this many particles: "+ foundParticles.length);
    for (var i = 0; i < foundParticles.length; i++) {
        print("   particle[" + i + "].id:" + foundParticles[i].id);
        if (foundParticles[i].id == particleA.id) {
            print(">>>> found particleA!!");
            var propertiesA = Particles.getParticleProperties(particleA);
            printProperties(propertiesA);
        }
        if (foundParticles[i].id == particleB.id) {
            print(">>>> found particleB!!");
        }
    }
    // move search
    searchAt.x += moveSearch.x;
    searchAt.y += moveSearch.y;
    searchAt.z += moveSearch.z;
    searchRadius += searchRadiusChange;

    // after we've searched for 80 iterations, change our search mechanism to be from the center with expanding radius    
    if (iteration == 80) {
        searchAt = { x: 3.5, y: 0, z: 3.5};
        moveSearch = { x: 0, y: 0, z: 0};
        searchRadius = 0.5;
        searchRadiusChange = 0.5;
    }
    
}


// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(findParticles);

// register our scriptEnding callback
Agent.scriptEnding.connect(scriptEnding);
