//
//  This sample script loads a sound file and plays it at the 'fingertip' of the 
//

//  First, load the clap sound from a URL 
var clap = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/bushtit_1.raw");	

function maybePlaySound() {
	if (Math.random() < 0.01) {
		//  Set the location and other info for the sound to play
		var options = new AudioInjectionOptions(); 
		var palmPosition = Controller.getSpatialControlPosition(0);
		options.position = palmPosition;
		options.volume = 0.5;
		Audio.playSound(clap, options);
	}
}

// Connect a call back that happens every frame
Agent.willSendVisualDataCallback.connect(maybePlaySound);