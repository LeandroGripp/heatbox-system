

void mainSocket() {
	// Setup the server to listen.
	
	// Bind the server to the right port.

	// Create the accept connections loop.

	// Accept each new connection by running a new thread. The function to be run is described below:

		// Infinite loop receiving messages.

		// When message received, create a new thread to deal with the request and continue the loop.
		// The function to be run in this new thread is described below:

			// Check if it's read or write request

			// If it's read, send request to OPC Client and wait for data.

			// If it's write, send request to OPC Client and wait for ACK.
}