/*! @file SimulatorDriver.cpp
 *  @brief  The SimulatorDriver runs a RobotController and connects it to a Simulator, using shared memory.
 *
 */

#include "SimulationBridge.h"
#include "Controllers/LegController.h"


void SimulationBridge::run() {
  // init shared memory:
  _sharedMemory.attach(DEVELOPMENT_SIMULATOR_SHARED_MEMORY_NAME);
  _sharedMemory().init();

  // init Quadruped Controller


  printf("[Simulation Driver] Starting main loop...\n");
  bool firstRun = true;
  for(;;) {
    // wait for our turn to access the shared memory
    // on the first loop, this gives the simulator a chance to put stuff in shared memory before we start
    _sharedMemory().waitForSimulator();

    if(firstRun) {
      firstRun = false;
      // check that the robot type is correct:
      if(_robot != _sharedMemory().simToRobot.robotType) {
        printf("simulator and simulatorDriver don't agree on which robot we are simulating (robot %d, sim %d)\n", (int)_robot, (int)_sharedMemory().simToRobot.robotType);
        throw std::runtime_error("robot mismatch!");
      }
    }

    // the simulator tells us which mode to run in
    _simMode = _sharedMemory().simToRobot.mode;
    switch(_simMode) {
      case SimulatorMode::RUN_CONTROL_PARAMETERS: // there is a new control parameter request
        handleControlParameters();
        break;
      case SimulatorMode::RUN_CONTROLLER:  // the simulator is ready for the next robot controller run
        _iterations++;
        runRobotControl();
        break;
      case SimulatorMode::DO_NOTHING: // the simulator is just checking to see if we are alive yet
        break;
      case SimulatorMode::EXIT: // the simulator is done with us
        printf("[Simulation Driver] Transitioned to exit mode\n");
        return;
        break;
      default:
        throw std::runtime_error("unknown simulator mode");
    }

    // tell the simulator we are done
    _sharedMemory().robotIsDone();
  }
}

/*!
 * This function handles a a control parameter message from the simulator
 */
void SimulationBridge::handleControlParameters() {
  ControlParameterRequest& request = _sharedMemory().simToRobot.controlParameterRequest;
  ControlParameterResponse& response = _sharedMemory().robotToSim.controlParameterResponse;
  if(request.requestNumber <= response.requestNumber) {
    // nothing to do!
    printf("[SimulationBridge] Warning: the simulator has run a ControlParameter iteration, but there is no new request!\n");
    return;
  }

  // sanity check
  u64 nRequests = request.requestNumber - response.requestNumber;
  assert(nRequests == 1);

  response.nParameters = _robotParams.collection._map.size(); // todo don't do this every single time?

  switch(request.requestKind) {
    case ControlParameterRequestKind::SET_PARAM_BY_NAME:
    {
      std::string name(request.name);
      ControlParameter& param = _robotParams.collection.lookup(name);

      // type check
      if(param._kind != request.parameterKind) {
        throw std::runtime_error("type mismatch for parameter " + name + ", robot thinks it is "
        + controlParameterValueKindToString(param._kind) + " but received a command to set it to " +
        controlParameterValueKindToString(request.parameterKind));
      }

      // do the actual set
      param.set(request.value, request.parameterKind);

      // respond:
      response.requestNumber = request.requestNumber; // acknowledge that the set has happened
      response.parameterKind = request.parameterKind; // just for debugging print statements
      response.value = request.value;                 // just for debugging print statements
      strcpy(response.name, name.c_str());            // just for debugging print statements
      response.requestKind = request.requestKind;


      printf("%s\n", response.toString().c_str());

    }
    break;


    case ControlParameterRequestKind::GET_PARAM_BY_NAME:
    {
      std::string name(request.name);
      ControlParameter& param = _robotParams.collection.lookup(name);

      // type check
      if(param._kind != request.parameterKind) {
        throw std::runtime_error("type mismatch for parameter " + name + ", robot thinks it is "
                                 + controlParameterValueKindToString(param._kind) + " but received a command to set it to " +
                                 controlParameterValueKindToString(request.parameterKind));
      }

      // respond
      response.value = param.get(request.parameterKind);
      response.requestNumber = request.requestNumber;   // acknowledge
      response.parameterKind = request.parameterKind;   // just for debugging print statements
      strcpy(response.name, name.c_str());              // just for debugging print statements
      response.requestKind = request.requestKind;       // just for debugging print statements


      printf("%s\n", response.toString().c_str());
    }
    break;
  }
}

void SimulationBridge::runRobotControl() {
  if(_firstControllerRun) {
    printf("[Simulator Driver] First run of robot controller...\n");
    if(_robotParams.isFullyInitialized()) {
      printf("\tAll %ld control parameters are initialized\n", _robotParams.collection._map.size());
      _simMode = SimulatorMode::RUN_CONTROLLER;
    } else {
      printf("\tbut not all control parameters were initialized. Missing:\n%s\n", _robotParams.generateUnitializedList().c_str());
      throw std::runtime_error("not all parameters initialized when going into RUN_CONTROLLER");
    }

    _robotController = new RobotController;

    _robotController->driverCommand = &_sharedMemory().simToRobot.gamepadCommand;
    _robotController->spiData       = &_sharedMemory().simToRobot.spiData;
    _robotController->robotType     = _robot;
    _robotController->kvhImuData    = &_sharedMemory().simToRobot.kvh;
    _robotController->vectorNavData = &_sharedMemory().simToRobot.vectorNav;
    _robotController->cheaterState  = &_sharedMemory().simToRobot.cheaterState;
    _robotController->spiCommand    = &_sharedMemory().robotToSim.spiCommand;
    _robotController->controlParameters = &_robotParams;

    _robotController->initialize();
    _firstControllerRun = false;
  }

  for(int i = 0; i < 4; i++) {
    _robotController->spiCommand->flags[i] = 1;
  }
  _robotController->step();
}