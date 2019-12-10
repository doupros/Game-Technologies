#include "../../Common/Window.h"

#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"

#include "../CSC8503Common/GameServer.h"
#include "../CSC8503Common/GameClient.h"

#include "../CSC8503Common/NavigationGrid.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

using namespace NCL;
using namespace CSC8503;

class TestPacketReceiver : public PacketReceiver {
public:
	TestPacketReceiver(string name) {
		this -> name = name;
	}

	void ReceivePacket(int type, GamePacket* payload, int source) {
		if (type == String_Message) {
			StringPacket* realPacket = (StringPacket*)payload;


			string msg = realPacket -> GetStringFromData();

			std::cout << name << " received message : " << msg << std::endl;
		}
	}
protected:
	string name;
};


void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();

	int someData = 0;

	StateFunc AFunc = [](void* data) {
		int* realData = (int*)data;
		(*realData)++;
		std::cout << "In State A!" << std::endl;
	};
	StateFunc BFunc = [](void* data) {
		int* realData = (int*)data;
		(*realData)--;
		std::cout << "In State B!" << std::endl;
	};

	GenericState* stateA = new GenericState(AFunc, (void*)&someData);
	GenericState* stateB = new GenericState(BFunc, (void*)&someData);
	testMachine->AddState(stateA);
	testMachine->AddState(stateB);

	GenericTransition <int&, int >* transitionA =
		new GenericTransition <int&, int >(
			GenericTransition <int&, int >::GreaterThanTransition,
			someData, 10, stateA, stateB); // if greater than 10 , A to B

	GenericTransition <int&, int >* transitionB =
		new GenericTransition <int&, int >(
			GenericTransition <int&, int >::EqualsTransition,
			someData, 0, stateB, stateA); // if equals 0 , B to A

	testMachine->AddTransition(transitionA);
	testMachine->AddTransition(transitionB);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(); // run the state machine !
	}
	delete testMachine;
}// end of TestStateMachine function !

void TestNetworking() {
	NetworkBase::Initialise();

	TestPacketReceiver serverReceiver(" Server ");
	TestPacketReceiver clientReceiver(" Client ");

	int port = NetworkBase::GetDefaultPort();

	GameServer* server = new GameServer(port, 1);
	GameClient* client = new GameClient();

	server -> RegisterPacketHandler(String_Message, &serverReceiver);
	client -> RegisterPacketHandler(String_Message, &clientReceiver);

	bool canConnect = client -> Connect(127, 0, 0, 1, port);

	for (int i = 0; i < 100; ++i) {
		server -> SendGlobalPacket(
			StringPacket(" Server says hello ! " + std::to_string(i)));

		client -> SendPacket(
			StringPacket(" Client says hello ! " + std::to_string(i)));

		server -> UpdateServer();
		client -> UpdateClient();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	NetworkBase::Destroy();
}

vector<Vector3> testNodes;

void TestPathfinding() {
	NavigationGrid grid("MapFile20.txt");
	
	NavigationPath outPath;

	Vector3 startPos(6, 6, 6);
	Vector3 endPos(120, 6, 120);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}
void DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];
		//std::cout <<"A:"<< a.x << a.y << a.z << std::endl;
		//std::cout <<"B:"<< b.x << b.y << b.z << std::endl;
		//Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
		Debug::DrawLine(Vector3(a.x+6,a.y,a.z+6), Vector3(b.x + 6, b.y, b.z + 6), Vector4(0, 1, 0, 1));
	}
}



/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1366, 768);

	if (!w->HasInitialised()) {
		return -1;
	}	

	TestStateMachine();
	//TestNetworking();
	//TestPathfinding();
	
	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	TutorialGame* g = new TutorialGame();

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();

		if (dt > 1.0f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}

		//DisplayPathfinding();

		w->SetTitle("Gametech frame time:" + std::to_string(1000 / (1000.0f * dt)));
		
		g->UpdateGame(dt);
		
		/*for (int ver = -100; ver < 100; ver += 4) {
			Debug::DrawLine(Vector3(ver, 1, -100), Vector3(ver, 1, 100), Vector4(0, 1, 0, 1));
		}
		for (int ver = -100; ver < 100; ver += 4) {
			Debug::DrawLine(Vector3(-100, 1, ver), Vector3(100, 1, ver), Vector4(0.5, 0, 0, 1));
		}
			Debug::DrawLine(Vector3(0, 0, 0), Vector3(0, 100, 0), Vector4(0, 0, 0.5, 1));
		*/
	}
	Window::DestroyGameWindow();
}