#include <cstring>
#include <thread>
#include <sys/mman.h>
#include <unistd.h>
#include "HardwareBridge.h"
#include "rt/rt_vectornav.h"

/*!
 * If an error occurs during initialization, before motors are enabled, print error and exit.
 * @param reason Error message string
 * @param printErrno If true, also print C errno
 */
void HardwareBridge::initError(const char *reason, bool printErrno) {
  printf("FAILED TO INITIALIZE HARDWARE: %s\n", reason);

  if(printErrno) {
    printf("Error: %s\n", strerror(errno));
  }

  exit(-1);
}

/*!
 * All initialization code that is common between Cheetah 3 and Mini Cheetah
 */
void HardwareBridge::initCommon() {
  printf("[HardwareBridge] Init stack\n");
  prefaultStack();
  printf("[HardwareBridge] Init scheduler\n");
  setupScheduler();
  if(!_interfaceLCM.good()) {
    initError("_interfaceLCM failed to initialize\n", false);
  }

  printf("[HardwareBridge] Subscribe LCM\n");
  _interfaceLCM.subscribe("interface", &HardwareBridge::handleGamepadLCM, this);
  _interfaceLCM.subscribe("interface-request", &HardwareBridge::handleControlParameter, this);

  printf("[HardwareBridge] Start interface LCM handler\n");
  _interfaceLcmThread = std::thread(&HardwareBridge::handleInterfaceLCM, this);
}

void HardwareBridge::handleInterfaceLCM() {
  while(!_interfaceLcmQuit)
    _interfaceLCM.handle();
}

/*!
 * Writes to a 16 KB buffer on the stack. If we are using 4K pages for our stack, this will make
 * sure that we won't have a page fault when the stack grows.  Also mlock's all pages associated with the current
 * process, which prevents the cheetah software from being swapped out.  If we do run out of memory, the robot
 * program will be killed by the OOM process killer (and leaves a log) instead of just becoming unresponsive.
 */
void HardwareBridge::prefaultStack() {
  printf("[Init] Prefault stack...\n");
  volatile  char stack[MAX_STACK_SIZE];
  memset(const_cast<char*>(stack), 0, MAX_STACK_SIZE);
  if(mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
    initError("mlockall failed.  This is likely because you didn't run robot as root.\n", true);
  }
}

/*!
 * Configures the
 */
void HardwareBridge::setupScheduler() {
  printf("[Init] Setup RT Scheduler...\n");
  struct sched_param params;
  params.sched_priority = TASK_PRIORITY;
  if(sched_setscheduler(0, SCHED_FIFO, &params) == -1) {
    initError("sched_setscheduler failed.\n", true);
  }
}

void HardwareBridge::handleGamepadLCM(const lcm::ReceiveBuffer *rbuf, const std::string &chan,
                                      const gamepad_lcmt *msg) {
  (void)rbuf;
  (void)chan;
  _gamepadCommand.set(msg);
}

void HardwareBridge::handleControlParameter(const lcm::ReceiveBuffer* rbuf,
                                            const std::string& chan,
                                            const control_parameter_request_lcmt* msg) {
  (void)rbuf;
  (void)chan;
  if (msg->requestNumber <= _parameter_response_lcmt.requestNumber) {
    // nothing to do!
    printf(
        "[HardwareBridge] Warning: the interface has run a ControlParameter iteration, but there is no new request!\n");
    //return;
  }

  // sanity check
  s64 nRequests = msg->requestNumber - _parameter_response_lcmt.requestNumber;
  if(nRequests != 1) {
    printf("[ERROR] Hardware bridge: we've missed %ld requests\n", nRequests - 1);
  }

  switch (msg->requestKind) {
    case (s8)ControlParameterRequestKind::SET_PARAM_BY_NAME: {
      std::string name((char*)msg->name);
      ControlParameter &param = _robotParams.collection.lookup(name);

      // type check
      if ((s8)param._kind != msg->parameterKind) {
        throw std::runtime_error("type mismatch for parameter " + name + ", robot thinks it is "
                                 + controlParameterValueKindToString(param._kind) +
                                 " but received a command to set it to " +
                                 controlParameterValueKindToString((ControlParameterValueKind)msg->parameterKind));
      }

      // do the actual set
      ControlParameterValue v;
      memcpy(&v, msg->value, sizeof(v));
      param.set(v, (ControlParameterValueKind)msg->parameterKind);

      // respond:
      _parameter_response_lcmt.requestNumber = msg->requestNumber; // acknowledge that the set has happened
      _parameter_response_lcmt.parameterKind = msg->parameterKind; // just for debugging print statements
      memcpy(_parameter_response_lcmt.value, msg->value, 64);
      //_parameter_response_lcmt.value = _parameter_request_lcmt.value;                 // just for debugging print statements
      strcpy((char*)_parameter_response_lcmt.name, name.c_str());            // just for debugging print statements
      _parameter_response_lcmt.requestKind = msg->requestKind;


      printf("[Robot Control Parameter] set %s to %s\n", name.c_str(),
          controlParameterValueToString(v, (ControlParameterValueKind)msg->parameterKind).c_str());

    }
      break;


    case (s8)ControlParameterRequestKind::GET_PARAM_BY_NAME: {
      printf("[ERROR] Robot doesn't support get param currently\n");
    }
//      std::string name(request.name);
//      ControlParameter &param = _robotParams.collection.lookup(name);
//
//      // type check
//      if (param._kind != request.parameterKind) {
//        throw std::runtime_error("type mismatch for parameter " + name + ", robot thinks it is "
//                                 + controlParameterValueKindToString(param._kind) +
//                                 " but received a command to set it to " +
//                                 controlParameterValueKindToString(request.parameterKind));
//      }
//
//      // respond
//      response.value = param.get(request.parameterKind);
//      response.requestNumber = request.requestNumber;   // acknowledge
//      response.parameterKind = request.parameterKind;   // just for debugging print statements
//      strcpy(response.name, name.c_str());              // just for debugging print statements
//      response.requestKind = request.requestKind;       // just for debugging print statements
//
//
//      printf("%s\n", response.toString().c_str());
//    }
      break;
  }
  _interfaceLCM.publish("interface-response", &_parameter_response_lcmt);
}

MiniCheetahHardwareBridge::MiniCheetahHardwareBridge()
{

}

void MiniCheetahHardwareBridge::run() {
  initCommon();
  initHardware();

  _robotController = new RobotController;

  _robotController->driverCommand = &_gamepadCommand;
//  _robotController->spiData       = &_sharedMemory().simToRobot.spiData;

  _robotController->robotType     = RobotType::MINI_CHEETAH;
  _robotController->vectorNavData = &_vectorNavData;

//  _robotController->spiCommand    = &_sharedMemory().robotToSim.spiCommand;

  _robotController->controlParameters = &_robotParams;
  _robotController->visualizationData = &_visualizationData;
  _robotController->cheetahMainVisualization = &_mainCheetahVisualization;

  _robotController->initialize();
  _firstRun = false;

  // init control thread

  statusTask.start();

  for(;;) {
    usleep(1000000);
    printf("joy %f\n", _robotController->driverCommand->leftStickAnalog[0]);
  }
}

void MiniCheetahHardwareBridge::initHardware() {
  printf("[MiniCheetahHardware] Init vectornav\n");
//  if(!init_vectornav(&_vectorNavData)) {
//    initError("failed to initialize vectornav!\n", false);
//  }


  // init spi
  // init sbus
  // init lidarlite

  // init LCM hardware logging thread


  //
}

