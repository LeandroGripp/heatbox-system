
void createOpcClient();
void createSocketServer();

void mainOrch(void) {

	// Initialize the data structures used for communication between threads.

	// Create thread that runs the createSocketServer function.

	// Create thread that runs the createOpcClient function.

	// Wait for both threads to finish.

	// Free memory resources.
}

void createSocketServer() {
	while (1) {
		// Create the thread that runs the socket server

		// Wait for the thread to exit. 
		// When it does, it loops over and creates a new one, making sure we recover from failure.
	}
}

void createOpcClient() {
	while (1) {
		// Create the thread that runs the OPC Client

		// Wait for the thread to exit. 
		// When it does, it loops over and creates a new one, making sure we recover from failure.
	}
}