
#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>
//#include <CAM/CAMAll.h>
#include <vector>
#include <windows.h>


using namespace adsk::core;
using namespace adsk::fusion;
//using namespace adsk::cam;

Ptr<Application> app;
Ptr<UserInterface> ui;

// Global command input declarations.
Ptr<ImageCommandInput> imgInput;
Ptr<DropDownCommandInput> dropDownRendezvousPts;
Ptr<TextBoxCommandInput> _errMessage;
Ptr<ButtonRowCommandInput> inputPointsButtons;

class CameraManager;
CameraManager* gCamManager = nullptr;

// define ID strings.
#define DROPDOWN_RENDEZVOUS_POINTS "dropDownRendezvousPts"
#define INPUT_LAYOUT_TABLE "inputLayoutTable"
#define INPUT_BUTTONS_ROW "inputButtonRow"
#define ADD_POINT "Add Point"
#define DELETE_POINT "Delete Point"
#define PLAY_REEL "Play"
#define STOP_REEL "Stop Player"

template<typename T>
void ShowValue(T value)
{
	std::string valStr = "The value is ";
	ui->messageBox(valStr);
}

bool checkReturn(Ptr<Base> returnObj)
{
	if (returnObj)
		return true;
	else
		if (app && ui)
		{
			std::string errDesc;
			app->getLastError(&errDesc);
			ui->messageBox(errDesc);
			return false;
		}
		else
			return false;
}

class RendezvousPoint
{
public:
	int m_nIndex;
	Ptr<Camera> camera;
	std::string name;
};

class CameraManager
{
	std::vector<RendezvousPoint> mapRendezvousPts;
	int m_nFrameIndex = 0;
	bool m_bContinuous = true;
	UINT_PTR m_nTimerId;
public:
	static void CALLBACK TimerCallBack(HWND hwnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
	{
		gCamManager->NextFrame();
	}

	bool NextFrame()
	{
		if (m_nFrameIndex < mapRendezvousPts.size())
		{
			RendezvousPoint& rPoint = mapRendezvousPts.at(m_nFrameIndex);
			auto actViewPort = app->activeViewport();
			actViewPort->camera(rPoint.camera);
			m_nFrameIndex++;
		}
		else
		{
			if (m_bContinuous)
				m_nFrameIndex = 0;
		}
		return true;
	}

	void Play()
	{
		m_nTimerId = SetTimer(NULL, m_nTimerId, 1000, CameraManager::TimerCallBack);
	}

	void Stop()
	{
		KillTimer(NULL, m_nTimerId);
		m_nFrameIndex = 0;
	}

	void AddRendezvousPoint()
	{
		RendezvousPoint currentPt;
		auto actViewPort = app->activeViewport();
		currentPt.camera = actViewPort->camera();
		//camera->upVector(Vector3D::create(0.0, 0.0, 1.0));
		// actViewPort->camera(camera);

		mapRendezvousPts.insert(mapRendezvousPts.end(), currentPt);		
	}

	void applyCamera()
	{
		auto actViewPort = app->activeViewport();
		int count = 0;
		// char message[256];
		// Get the camera from list.
		for (const auto& rendezvousPt : mapRendezvousPts)
		{
			//ShowValue(count++);
			/*count++;
			sprintf(message, "Rendezvous point %d of %d total points", count, mapRendezvousPts.size());
			ui->messageBox(message);*/

			actViewPort->camera(rendezvousPt.camera);
			Sleep(1000);
		}
	}

	~CameraManager()
	{
		Stop();
	}
};

class CamWizardInputCommandChangedHandeler : public adsk::core::InputChangedEventHandler
{
public:
	void notify(const Ptr < InputChangedEventArgs>& eventArgs) override
	{
		Ptr<CommandInput> changedInput = eventArgs->input();

		if (changedInput->name() == ADD_POINT)
		{
			gCamManager->AddRendezvousPoint();
			ui->messageBox(changedInput->name());
		}
		else if (changedInput->name() == DELETE_POINT)
		{
			gCamManager->applyCamera();
		}
		else if (changedInput->name() == PLAY_REEL)
		{
			gCamManager->Play();
		}
		else if(changedInput->name() == STOP_REEL)
		{
			gCamManager->Stop();
		}
	}
}_camWizardInputChanged;

class CamWizardCommandCreatedEventHandler : public adsk::core::CommandCreatedEventHandler
{
public:
	void notify(const Ptr<CommandCreatedEventArgs>& eventArgs) override
	{
		// Verify that a Fusion design is active.
		Ptr<Design> des = app->activeProduct();
		if (!checkReturn(des))
		{
			ui->messageBox("A Fusion design must be active when invoking this command.");
			return;
		}

		Ptr<Command> cmd = eventArgs->command();
		cmd->isExecutedWhenPreEmpted(false);
		Ptr<CommandInputs> inputs = cmd->commandInputs();
		if (!checkReturn(inputs))
			return;

		// Define the command dialog.
		imgInput = inputs->addImageCommandInput("CamWizardImage", "", "Resources/CamWizardImage.png");
		if (!checkReturn(imgInput))
			return;
		imgInput->isFullWidth(true);

		// Rendezvous Points dropdown.
		dropDownRendezvousPts = inputs->addDropDownCommandInput(DROPDOWN_RENDEZVOUS_POINTS, "Rendezvous Points", TextListDropDownStyle);
		if (!checkReturn(dropDownRendezvousPts))
			return;

		// Buuton row
		Ptr<ButtonRowCommandInput> buttonsRowInput = inputs->addButtonRowCommandInput(INPUT_BUTTONS_ROW, "Input Button Row", false);
		buttonsRowInput->commandInputs()->addBoolValueInput(ADD_POINT, ADD_POINT, false, "resources/AddPoint");
		buttonsRowInput->commandInputs()->addBoolValueInput(PLAY_REEL, PLAY_REEL, false, "resources/DeletePoint");
		/*buttonsRowInput->listItems()->add(ADD_POINT, false, "resources/AddPoint");
		buttonsRowInput->listItems()->add(PLAY_REEL, false, "resources/DeletePoint");
		buttonsRowInput->listItems()->add(STOP_REEL, false, "resources/StopPlaying");*/

		// Connect to the command related events.
		Ptr<InputChangedEvent> inputChnagedEvent = cmd->inputChanged();
		if (!checkReturn(inputChnagedEvent))
			return;
		bool isOk = inputChnagedEvent->add(&_camWizardInputChanged);
		if (!isOk)
			return;

	}
} _camWizardCreated;

//class CamWizardCommandCreatedEventHandler : public adsk::core::CommandCreatedEventHandler
//{
//public:
//	void notify(const Ptr<CommandCreatedEventArgs>& eventArgs) override
//	{
//		// Verify that a Fusion design is active.
//		Ptr<Design> des = app->activeProduct();
//		if (!checkReturn(des))
//		{
//			ui->messageBox("A Fusion design must be active when invoking this command.");
//			return;
//		}
//
//		Ptr<Command> cmd = eventArgs->command();
//		cmd->isExecutedWhenPreEmpted(false);
//		Ptr<CommandInputs> inputs = cmd->commandInputs();
//		if (!checkReturn(inputs))
//			return;
//
//		// Define the command dialog.
//		imgInput = inputs->addImageCommandInput("CamWizardImage", "", "Resources/CamWizardImage.png");
//		if (!checkReturn(imgInput))
//			return;
//		imgInput->isFullWidth(true);
//
//		// Rendezvous Points dropdown.
//		dropDownRendezvousPts = inputs->addDropDownCommandInput(DROPDOWN_RENDEZVOUS_POINTS, "Rendezvous Points", TextListDropDownStyle);
//		if (!checkReturn(dropDownRendezvousPts))
//			return;
//
//		// Table Input
//		// Create table input
//		Ptr<TableCommandInput> tableInput = inputs->addTableCommandInput(INPUT_LAYOUT_TABLE, "Table", 3, "1:1:1");
//		tableInput->tablePresentationStyle(adsk::core::TablePresentationStyles::transparentBackgroundTablePresentationStyle);
//
//		// Add point delete point buttons.
//		Ptr<CommandInput> addButtonInput = inputs->addBoolValueInput(tableInput->id() + "_add", ADD_POINT, false, "resources/AddPoint", true);
//		tableInput->addToolbarCommandInput(addButtonInput);
//		//Ptr<CommandInput> deleteButtonInput = inputs->addBoolValueInput(tableInput->id() + "_delete", DELETE_POINT, false, "resources/DeletePoint", true);
//		//tableInput->addToolbarCommandInput(deleteButtonInput);
//
//		// Play
//		Ptr<CommandInput> playButtonInput = inputs->addBoolValueInput(tableInput->id() + "_play", PLAY_REEL, false, "resources/DeletePoint", true);
//		tableInput->addToolbarCommandInput(playButtonInput);
//		// Stop
//		Ptr<CommandInput> stopButtonInput = inputs->addBoolValueInput(tableInput->id() + "_stop", STOP_REEL, false, "resources/StopPlaying", true);
//		tableInput->addToolbarCommandInput(stopButtonInput);
//
//		// Connect to the command related events.
//		Ptr<InputChangedEvent> inputChnagedEvent = cmd->inputChanged();
//		if (!checkReturn(inputChnagedEvent))
//			return;
//		bool isOk = inputChnagedEvent->add(&_camWizardInputChanged);
//		if (!isOk)
//			return;
//
//	}
//} _camWizardCreated;

extern "C" XI_EXPORT bool run(const char* context)
{
	app = Application::get();
	if (!app)
		return false;

	ui = app->userInterface();
	if (!ui)
		return false;

	// Create instance of camera manager.
	gCamManager = new CameraManager();

	// Create a command definition and add a button to the CREATE panel.
	Ptr<CommandDefinition> cmdDef = ui->commandDefinitions()->itemById("adskCamWizardCPP");
	if (!cmdDef)
	{
		cmdDef = ui->commandDefinitions()->addButtonDefinition("adskCamWizardCPP", "Cam Wizard", "Creates a rendezvous point on locations in view", "resources/AddPoint");
		if (!checkReturn(cmdDef))
			return false;
	}

	// Connect to the command created event.
	Ptr<CommandCreatedEvent> commandCreatedEvent = cmdDef->commandCreated();
	if (!checkReturn(commandCreatedEvent))
		return false;
	bool isOk = commandCreatedEvent->add(&_camWizardCreated);
	if (!isOk)
		return false;

	isOk = cmdDef->execute();
	if (!isOk)
		return false;

	// Prevent this module from terminating so that the command can continue to run until
	// the user completes the command.
	adsk::autoTerminate(false);

	//ui->messageBox("Hello addin");

	return true;
}

extern "C" XI_EXPORT bool stop(const char* context)
{
	if (ui)
	{
		ui->messageBox("Stop addin");
		ui = nullptr;
	}

	if (gCamManager)
	{
		delete gCamManager;
		gCamManager = nullptr;
	}

	return true;
}


#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // XI_WIN
