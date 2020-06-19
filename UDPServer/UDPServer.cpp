#include "Client.h";
#include "MySDLEngine.h";

void main()
{
	MySDLEngine engine = MySDLEngine::MySDLEngine();
	Client client = Client::Client();
	client.setEngine(&engine);
	engine.setClient(&client);

	//Load media
	if (!engine.loadMedia())
	{
		printf("Failed to load media!\n");
	}
	else
	{
		client.runrecvtask();
		engine.run();
	}
}

